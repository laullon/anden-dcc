# Local HTTP API

This local prototype has no authentication. Examples use `http://anden-dcc.local`;
the assigned IP also works. There is no cloud service.

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/` | English/Spanish panel; Setup closed by default |
| GET | `/icon.svg` | SVG app icon |
| GET | `/api/status` | Version, network and DCC state; no LED pulse |
| POST | `/dcc/f0` | Form: `address=1..127`, `on=0|1` |
| POST | `/wifi/save` | Form: `ssid`, `password`; stored on Pico |
| POST | `/wifi/forget` | Erase network and restore setup AP |
| GET | `/update` | 303 to `/?setup=firmware` |
| POST | `/update` | Multipart `firmware` field with compatible `.bin` |

Status includes version, connection, mDNS, storage, SSID, IP, `trackPower`, and:

```json
{"ready":true,"running":true,"pin":2,"address":3,"f0":false,"bytes":"03 80 83"}
```

That object is under `dcc`. `trackPower` is always false in 0.4.0. DCC state is
the last queued request, not decoder readback. Queued packets precede the latest
command. F1–F4 are off. No speed, programming or second refreshed address exists.

F0: 200 with state, 400 invalid fields, 503 unavailable/full queue.
Wi-Fi: 400 invalid data, 500 persistence failure. Password empty means an open
network, not "keep password". SSID: 1–32 bytes; password empty or 8–63 bytes.

The updater can return **HTTP 200 for an error**. Check `Update Success` in the
body, then verify the version after reboot. POST stops GP2 before upload; if it
fails, a new F0 command restarts the signal. Use `.bin`, not UF2 or filesystem
images, with the unchanged 1 MB sketch / 1 MB filesystem layout.

The future power stage must also physically disarm its bridge during updates;
this release only stops the logic signal.
