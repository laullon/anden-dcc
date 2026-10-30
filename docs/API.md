# Local HTTP API

This local prototype has no authentication. Examples use `http://anden-dcc.local`;
the assigned IP also works. There is no cloud service.

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/` | English/Spanish roster panel; Setup closed by default |
| GET | `/icon.svg` | SVG app icon |
| GET | `/favicon.svg` | Small SVG browser-tab icon |
| GET | `/favicon.ico` | Binary ICO fallback, 16/32px |
| GET | `/api/status` | Version, network and DCC state; no LED pulse |
| POST | `/dcc/f0` | Form: existing roster `address=1..127`, `on=0|1` |
| POST | `/locos/save` | Form: optional `id`, `address=1..127`, optional `name`, optional `type` (electric, diesel, steam) |
| POST | `/locos/remove` | Form: existing `id=1..8` |
| POST | `/wifi/save` | Form: `ssid`, `password`; stored on Pico |
| POST | `/wifi/forget` | Erase network and restore setup AP |
| GET | `/update` | 303 to `/?setup=firmware` |
| POST | `/update` | Multipart `firmware` field with compatible `.bin` |

Status includes version, connection, mDNS, storage, SSID, IP, `trackPower`, and
this object under `dcc` (schema changed in 0.5.0):

```json
{
  "ready": true,
  "running": true,
  "pin": 2,
  "maxLocos": 8,
  "locos": [
    {"id": 1, "name": "Evening express", "type": "electric", "address": 3, "f0": true, "bytes": "03 90 93"},
    {"id": 2, "name": "Shunter", "address": 4, "f0": false, "bytes": "04 80 84"}
  ]
}
```

All three DCC/roster POST endpoints return that `dcc` object on success.
`trackPower` is always false. State is requested state, not decoder readback;
old complete packets can remain briefly in the DMA ring/FIFO before a change
reaches the GPIO. Every roster address is refreshed without browser polling.
F1–F4 remain off. No speed, programming or other function groups exist yet.

## Roster

Two slots at addresses 3 and 4 are provided when no valid roster file exists.
Create by omitting `id` or leaving it empty; edit by sending an existing ID.
IDs are stable across edits and restarts, but may be reused after removal.
Addresses must be unique. Names are optional, at most 48 UTF-8 bytes, with no
ASCII control characters. An empty name gets a translated fallback in the UI.
Names are escaped in JSON and displayed as text, never HTML.

Names, addresses and illustration types persist in `/locos.cfg` on Pico LittleFS. Writes use a
temporary file followed by rename; failed saves do not change the RAM roster.
The valid empty roster persists and generates idle packets (`FF 00 FF`).
F0 lives in RAM and always starts off after reboot. Changing a loco's address
starts its F0 off; changing only its name preserves F0. Removing a loco or
changing its address stops refreshing the old decoder without sending an off
or stop command. Its last state may remain, so turn lights off first if desired.

```sh
curl --data-urlencode 'name=Evening express' --data 'id=1&address=3' http://anden-dcc.local/locos/save
curl --data 'address=3&on=1' http://anden-dcc.local/dcc/f0
curl --data 'id=2&address=4&name=Shunter' http://anden-dcc.local/locos/save
curl --data 'id=2' http://anden-dcc.local/locos/remove
```

Errors: 400 invalid input; 404 unknown ID/address; 409 duplicate address or
roster full; 500 persistence failure; 503 unavailable signal generator (F0).
Saving/removing entries does not restart a signal stopped by a failed OTA;
an explicit F0 command does. A successful config save can still report
`ready=false` if hardware resources were unavailable.

## Scope

Decoder programming was removed in 0.5.2. There is no programming capability
object in status; the former `/programming/...` routes now return 404.
Changing a roster address only changes the controller's roster, not the decoder.

## Wi-Fi and OTA

Wi-Fi: 400 invalid data, 500 persistence failure. Password empty means an open
network, not "keep password". SSID: 1–32 bytes; password empty or 8–63 bytes.

The updater can return **HTTP 200 for an error**. Check `Update Success` in the
body, then verify the version after reboot. POST stops DMA and PIO before upload;
if it fails, a new F0 command restarts the signal. Use `.bin`, not UF2 or filesystem
images, with the unchanged 1 MB sketch / 1 MB filesystem layout. Normal firmware
OTA preserves both the Wi-Fi configuration and the roster, resetting F0 off.

The future power stage must also physically disarm its bridge during updates;
this release only stops the logic signal.


## Guided F0 address finder (0.6.0)

POST `/finder/start` with `first=1..127` and `last=1..127`, ordered and inclusive.
Default UI range: 1-100. Address 0 is never transmitted. POST `/finder/answer`
with the current `step` and `answer=on|off|retry`. Answers describe the actual
lights, even when the question asks whether they are OFF. POST `/finder/cancel`
with the current `step`. If no search is active, cancel dismisses the last result
without changing the DCC output. All return the normal `dcc` object, including `finder`.

Example finder status (also under `dcc.finder` in GET `/api/status`):

```json
{"stage":"split","active":true,"step":3,"first":1,"last":100,
 "low":1,"high":100,"middle":50,"result":0,"waitMs":4000}
```

Stages: `idle`, `verifyOn`, `verifyOff`, `split`, `confirmOn`, `confirmOff`,
`cleanup`, `found`, `failed`. The finder is shared by all clients, kept in RAM
and reset on reboot. Each pattern has a 4000 ms settling interval; early answers,
old step tokens and conflicting starts return 409. Invalid input returns 400;
an unavailable generator returns 503. Retries retain the candidate range but
change the step token and restart the settling interval.

Each phase continuously sends function-group-one packets to **the entire original
range**, setting excluded addresses OFF. After ON/OFF preflight, ON selects the
lower half and OFF the upper half. A singleton skips splitting but still needs
ON/OFF confirmation. A failed preflight or confirmation yields no result. F1-F4
are off; no speed, broadcast, long-address or programming packets are generated.
This requires F0 lighting compatible with function-group-one (not 14-step FL).

Completion, cancellation or 180000 ms since the last accepted start/answer/retry
enters `cleanup`: send all original addresses OFF for 4000 ms, then resume the
unchanged roster and its requested F0 states. Unlisted locos remain OFF; functions
other than roster F0 are not restored. HTTP polling does not extend the timeout.
During an active search, including cleanup, `/dcc/f0`, `/locos/save`,
`/locos/remove`, POST `/update`, `/wifi/save` and `/wifi/forget` return 409.
The existing hardware limitations remain: **GP2 logic only; track power disabled**.


## Locomotive illustration type (0.6.1)

Each loco in `dcc.locos` includes `type`: `electric`, `diesel` or `steam`.
POST `/locos/save` accepts this optional field. Omitting it preserves the type
of an existing loco; new locos default to `electric`. An unsupported or empty
explicit type returns 400 without changing configuration. Type-only edits keep
F0 and address unchanged and do not alter the DCC packet contents.

The type is shared by browsers and stored with the roster in LittleFS.
`anden-roster-1` files still load, defaulting every type to electric. The next
successful roster save writes `anden-roster-2`, whose lines are
`id\taddress\ttype\tname`. F0 remains RAM-only and starts off after reboot.
Firmware older than 0.6.1 does not understand the v2 file: downgrade only with a
separate copy of the roster settings. Older API clients can still omit `type`.

Artwork sources and maintenance: [locomotive artwork](LOCO-ART.md).

OTA staging files are reclaimed after boot and before a new upload (0.6.1).
This preserves the same memory layout and all saved Wi-Fi/roster configuration.
