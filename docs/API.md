# Local HTTP API

This local prototype has no authentication. Examples use `http://anden-dcc.local`;
the assigned IP also works. There is no cloud service.

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/` | English/Spanish roster panel; Setup closed by default |
| GET | `/icon.svg` | SVG app icon |
| GET | `/api/status` | Version, network and DCC state; no LED pulse |
| POST | `/programming/address/read` | Prepared boundary; always 501 |
| POST | `/programming/address/write` | `address=1..127`; 400 invalid / otherwise 501 |
| POST | `/programming/cv/read` | `cv=1..1024`; 400 invalid / otherwise 501 |
| POST | `/programming/cv/write` | `cv=1..1024`, `value=0..255`; 400 invalid / otherwise 501 |
| POST | `/dcc/f0` | Form: existing roster `address=1..127`, `on=0|1` |
| POST | `/locos/save` | Form: optional `id`, `address=1..127`, optional `name` |
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
    {"id": 1, "name": "Evening express", "address": 3, "f0": true, "bytes": "03 90 93"},
    {"id": 2, "name": "Shunter", "address": 4, "f0": false, "bytes": "04 80 84"}
  ]
}
```

All three DCC/roster POST endpoints return that `dcc` object on success.
`trackPower` is always false. State is requested state, not decoder readback;
old complete packets can remain briefly in the DMA ring/FIFO before a change
reaches the GPIO. Every roster address is refreshed without browser polling.
F1–F4 remain off. No speed, programming or other functions exist yet.

## Roster

Two slots at addresses 3 and 4 are provided when no valid roster file exists.
Create by omitting `id` or leaving it empty; edit by sending an existing ID.
IDs are stable across edits and restarts, but may be reused after removal.
Addresses must be unique. Names are optional, at most 48 UTF-8 bytes, with no
ASCII control characters. An empty name gets a translated fallback in the UI.
Names are escaped in JSON and displayed as text, never HTML.

Names and addresses persist in `/locos.cfg` on Pico LittleFS. Writes use a
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

## Programming preparation (0.5.1)

`GET /api/status` also includes:

```json
{"programming":{"available":false,"state":"hardware-required","operations":[],"planned":["read-address","write-short-address","read-cv","write-cv"],"missing":["isolated-powered-output","calibrated-ack-input","service-mode-transmitter"]}}
```

Valid programming POST requests return **HTTP 501** with
`error: "programming-unavailable"`. There is no enable flag or simulated success.
They never send packets, stop/restart F0, alter CVs, or update the roster.
Malformed numeric parameters return 400. Zero is valid for a CV value, not a
CV number or short address. The UI keeps these operations disabled.
See the [programming status and hardware plan](PROGRAMMING.md).

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
