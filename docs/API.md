# HTTP API

The ESP13 hosts the app at `http://anden-esp13.local/`, its router IP or the open
setup network `http://192.168.13.1/`. Requests have no application authentication.
POST fields are URL-encoded unless noted. Commands wait for checked Uno replies.

| Method | Path | Fields / result |
| --- | --- | --- |
| GET | `/` | Embedded web panel |
| GET | `/api/status` | ESP identity, version, boot ID, network, storage, LED, Uno and `dcc` |
| POST | `/api/track/on` | Explicit start; returns browser `lease` |
| POST | `/api/track/keep` | `lease`; renew existing session only |
| POST | `/api/track/off` | Optional `lease`; absent means manual global Stop |
| POST | `/dcc/f0` | `address`, `on` (0/1) |
| POST | `/dcc/function` | `address`, `function` (0–28), `on` (0/1) |
| POST | `/dcc/direction` | `address`, `forward` (0/1); speed stays zero |
| POST | `/locos/save` | Optional `id`, `address`, `name`, `type` (`electric`, `diesel`, `steam`) |
| POST | `/locos/remove` | `id` |
| POST | `/locos/favorite` | `id`, `function`, `name`; pin or rename |
| POST | `/locos/favorite/remove` | `id`, `function`; remove pin only |
| POST | `/finder/start` | `first`, `last`; requires active track session |
| POST | `/finder/answer` | `step`, `answer` (`on`, `off`, `retry`) |
| POST | `/finder/cancel` | `step`; cancel/restore or dismiss result |
| POST | `/api/uno/ping` | Checked Uno link status |
| POST | `/api/uno/led` | Blink L LED; both output enables must be LOW |
| POST | `/api/uno/bridge` | `direction` (0/1); no-load three-second DC pulse |
| POST | `/api/uno/dcc-test` | No-load three-second DCC burst |
| POST | `/api/uno/bench-stop` | Stop bench/track output |
| POST | `/setup/wifi` | `ssid`, `password`; accepted changes apply asynchronously |
| POST | `/setup/wifi/forget` | Forget router credentials; keep setup network |
| POST | `/update` | Exactly one multipart `firmware` `.bin`; checked size/profile/identity/CRC; ESP only |

## Status and persistence

`dcc` includes `ready`, `running`, packet/fault counters, `functionsSupported`,
`maxFunction=28`, `maxFavorites=8`, `finder` and `locos`. Each loco includes `id`,
`address`, `name`, `type`, `forward`, `speed=0`, `f0`, numeric 29-bit `functions`,
`favorites: [{number,name}]`, and debug F0-group `bytes`. Read actual output
state from `uno.trackActive`, `uno.enableA` and `uno.enableB`.

The roster supports eight unique short addresses 1–127. Names/types/addresses and
favorite metadata live in ESP LittleFS `/locos.cfg`, saved using a temporary file
and rename. Up to eight favorite names of 1–24 UTF-8 bytes belong to each loco ID.
Pinning/removing does not activate/deactivate a function. Active function states
are RAM-only and start OFF on ESP boot; changing an address resets them.

Legacy `anden-roster-2` files are read and upgraded on save to `anden-roster-3`.
Loco rows are `L\tid\taddress\ttype\tname`; favorite rows are
`F\tid\tfunction\tname`. OTA with the same layout preserves this filesystem;
erasing flash or changing layouts can remove it.

## Power and errors

The browser normally renews its lease every 500 ms. The ESP gives a browser six
seconds to reconnect while keeping the Uno's independent two-second lease alive.
A stale lease cannot stop a newer session or restart an expired one. Power always
needs explicit activation. Manual off without a lease is a global Stop.

Metadata address edits require output OFF. Finder activity locks function, roster
and favorite edits; Wi-Fi/OTA require all track/bench output off and no active search. Search
uses headlights plus repeated zero-speed packets; it does not program CVs.

Typical errors: 400 invalid input/full favorite list, 404 missing loco, 409 active
search/output conflict or stale session, 500 storage failure, 501 unsupported Uno
capability, 503 missing acknowledgment. Success returns `dcc` for loco/function/
search requests, Uno status for diagnostics, or the result noted above. UI state
must follow acknowledged responses rather than guessing from a click.

[Protocol and timing](../firmware/uno-stack-test/FIRST-LOCO-TEST.md)
