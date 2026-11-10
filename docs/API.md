# HTTP API

The ESP13 hosts the app at `http://anden-esp13.local/`, its assigned LAN IP or
the open setup network at `http://192.168.13.1/`. Requests have no application
authentication. Send POST fields as URL-encoded form data unless noted below.
Function and direction updates verify a checked Uno reply before reporting
success. Starting a search and answering a question also require a checked reply.

| Method | Path | Fields / result |
| --- | --- | --- |
| GET | `/` | Embedded web panel |
| GET | `/api/status` | ESP identity, version, boot ID, network, storage, LED, Uno and `dcc` |
| POST | `/api/track/on` | Explicit start; returns browser `lease` |
| POST | `/api/track/keep` | `lease`; renew existing session only |
| POST | `/api/track/off` | Optional `lease`; absent means manual global Stop |
| POST | `/dcc/f0` | `address`, `on` (0/1) |
| POST | `/dcc/function` | `address`, `function` (0–28), `on` (0/1) |
| POST | `/dcc/direction` | `address`, `forward` (0/1). Stops cab motion before changing direction. |
| POST | `/locos/save` | Required `address`. Optional `id` (omit to add), `name` `type` (`electric`, `diesel`, `steam`) and `onDiorama` (0/1). |
| POST | `/locos/remove` | `id` |
| POST | `/locos/favorite` | `id`, `function`, `name`; pin or rename |
| POST | `/locos/favorite/remove` | `id`, `function`; remove pin only |
| POST | `/finder/start` | `first`, `last`; requires active track session |
| POST | `/finder/answer` | `step`, `answer` (`on`, `off`, `retry`) |
| POST | `/finder/cancel` | `step`. Start cleanup or dismiss the result. |
| POST | `/api/uno/ping` | Checked Uno link status |
| POST | `/api/uno/led` | Blink L LED; both output enables must be LOW |
| POST | `/api/uno/bridge` | `direction` (0/1); no-load three-second DC pulse |
| POST | `/api/uno/dcc-test` | No-load three-second DCC burst |
| POST | `/api/uno/bench-stop` | Stop bench/track output |
| POST | `/setup/wifi` | `ssid`, optional `password`. Also accepts multipart form data. Returns 202 before applying settings. |
| POST | `/setup/wifi/forget` | Returns 202 before forgetting router credentials. The setup network stays available. |
| GET | `/update` | Standalone firmware upload form |
| POST | `/update` | One multipart `firmware` file. Validates headers, size, app ID and CRC. Success returns HTTP 200 text followed by an ESP restart. |

Read `otaMaxBytes` from status before uploading. To confirm the restart, poll
`/api/status` for app ID `ANDEN_ESP13_OTA_V1` and a new `bootId`.

## Status and persistence

`dcc` includes `ready`, `running`, packet/fault counters, `functionsSupported`,
`cabFilterSupported`, `maxFunction=28`, `maxFavorites=8`, `finder` and `locos`. Each loco includes `id`,
`address`, `name`, `type`, `onDiorama`, `forward`, `speed=0`, `f0`, numeric 29-bit `functions`,
`favorites: [{number,name}]`, and debug F0-group `bytes`. Read reported output
state from `uno.online`, `uno.enableA`, `uno.enableB` and `uno.trackActive`.
The `speed` field reports the web request, which is always zero. Read the
commanded local step on the [cab display](../firmware/uno-stack-test/CAB.md).
`uno.trackActive` identifies normal track operation. The top-level and
`dcc.trackPower` flags also include a timed DCC burst.

The roster supports eight unique short addresses 1–127 and optional names up to
48 UTF-8 bytes. Omitting the type preserves an existing card's type or defaults
a new card to electric. The ESP saves names, types, addresses and favorite labels to
LittleFS `/locos.cfg` using a temporary file and rename. Up to eight favorite names of 1–24 UTF-8 bytes belong to each loco ID.
Pinning/removing does not activate/deactivate a function. Active function states
are RAM-only and start OFF on ESP boot; changing an address resets them.

Legacy `anden-roster-2` and `anden-roster-3` files load with all locos unmarked
and upgrade on save to `anden-roster-4`.
Loco rows are `L\tid\taddress\ttype\tonDiorama\tname`; favorite rows are
`F\tid\tfunction\tname`. OTA with the same layout preserves this filesystem.
On a failed mount, only an entirely erased filesystem region is initialized.
Invalid stored records leave the default roster available with saves disabled.

## Power and errors

The browser normally renews its lease every 500 ms. The ESP gives a browser six
seconds to reconnect while keeping the Uno's independent two-second lease alive.
A stale lease cannot stop a newer session or restart an expired one. Power always
needs explicit activation. Manual off without a lease is a global Stop.

Adding, changing or removing roster entries requires both outputs off. Function
and direction requests can update the schedule with track power off or during
normal track operation. Finder activity locks those requests and roster/favorite
edits. Wi-Fi and OTA require track and bench output off and no active search.
Search uses headlights and repeated zero-speed packets.

Typical errors include 400 for invalid input or a full favorite list, 404 for a
missing loco, 409 for an output conflict, stale session or duplicate address,
500 for storage or flash failure, 501 for unsupported Uno capability and 503 for
a missing acknowledgment. Refresh status after a missing acknowledgment before
retrying. Successful loco, function and search requests return the `dcc` object.
Diagnostics return Uno status. See the table for other responses.

[Protocol and timing](../firmware/uno-stack-test/FIRST-LOCO-TEST.md)
