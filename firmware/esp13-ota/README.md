# ESP13 web controller

**ESP13 0.6.4 / Uno 0.6.0.** This application hosts the panel, Wi-Fi setup,
persistent roster and ESP OTA updates. The Uno owns DCC and output timing.
See the [main guide](../../README.md), [wiring](../../docs/hardware/README.md),
[HTTP API](../../docs/API.md) and [stationary test](../uno-stack-test/FIRST-LOCO-TEST.md).

## Build

Tested core: **esp8266:esp8266@3.1.2**. Use **Generic ESP8266, 1 MB / 64 KB
LittleFS, DOUT, 80 MHz**. Keep this layout for OTA compatibility. The board must
have at least 1 MB of flash; identify its actual flash size before first upload.

```sh
tools/build-esp13.sh
```

The build embeds `web/index.html` and the original SVGs from `../../web/locos/`
into `SetupPage.h`, runs host checks, compiles with the local AndenStackLink
library, and validates `build/esp13-ota/esp13-ota.ino.bin`. The manifest records
version, board profile, size and SHA-256. Maximum OTA image: **448 KiB**.

## First serial installation

OTA needs this application to be running first. For an unmodified ESP13:

1. Disconnect motor VIN and track. Isolate the Uno UART with both shield switches OFF.
2. Power the ESP shield adequately and connect common GND to a **3.3 V logic**
   USB-UART adapter: adapter TX → ESP RX0, RX → ESP TX0. Use the raw ESP header;
   do not apply 5 V logic there. Avoid conflicting supply feeds.
3. Enter ROM bootloader mode: hold KEY, press/release ESP RST, then release KEY.
4. Identify the flash and save a full backup before writing. Do not guess its size.

```sh
python -m esptool --chip esp8266 --port ADAPTER_PORT --before no-reset --after no-reset flash-id
python -m esptool --chip esp8266 --port ADAPTER_PORT --before no-reset --after no-reset \
  read-flash 0x0 ACTUAL_FLASH_SIZE_BYTES esp13-before.bin
python -m esptool --chip esp8266 --port ADAPTER_PORT --before no-reset --after no-reset \
  write-flash --flash-mode dout --flash-size 1MB 0x0 build/esp13-ota/esp13-ota.ino.bin
```

Disconnect adapter DTR/RTS from boot pins, release KEY and press ESP RST to boot
normally. Remove the adapter before enabling the stacked UART. Both switches ON
connect Uno and ESP for normal operation; OFF permits Uno USB upload.

## Wi-Fi and OTA

Join the open **anden-esp13** Wi-Fi and browse **http://192.168.13.1/**. Setup
saves your 2.4 GHz router details. Return to your network and browse
**http://anden-esp13.local/** or the router IP. The recovery AP stays available.
There is deliberately no app login or AP password. The SDK stores router
credentials outside the sketch; do not commit them or full flash backups.

Turn track power off and finish any address search before an update. The browser
checks image identity and size, shows upload progress and verifies the reboot.
The command-line helper checks the app identity, flash layout, expected version
and a new boot ID:

```sh
python3 firmware/esp13-ota/ota.py --host anden-esp13.local \
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.6.4
```

OTA replaces the **ESP application only**; it cannot update the Uno. Keep OTA
support in replacement builds. Interrupted/bad updates may require serial recovery;
there is no automatic rollback. Normal OTA preserves roster/favorites and router
settings with this layout; flash erasure or layout changes can remove them.

## Panel behavior

System / Light / Dark buttons remember appearance in each browser. Wi-Fi and
firmware controls stay under the collapsed Setup section. A compact power bar
appears after the main power panel scrolls above the viewport. Both use the same
acknowledged output state and browser-owned lease; scrolling never changes power.
Unknown output state still offers Power off, while Power on remains guarded.

The DOIT blue LED uses **GPIO16, active LOW**: 500 ms blink while connecting,
steady when joined, 120 ms dip on user requests. Automatic status polling is quiet.
The Uno L LED is a separate test, blocked whenever an output is enabled.

## Function explorer and favorites (0.6.0)

Each loco card has a collapsed **Explore functions** panel. Open it, choose F0–F28
and use its single button to toggle On/Off, just like the buttons on the card.
F0 always appears on every card. Track power can stay OFF while you set any
function or direction; those settings take effect on the rails when power is on.
Function meanings come from your decoder: F1 might be sound on one loco and something else on another.
Selecting a function alone sends no command. These controls latch On/Off; there is
no momentary horn button yet.

Enter a short name and choose **Pin favorite**. Up to eight favorites per loco
stay visible as quick toggle buttons above the collapsed panel. Select a pinned
function to rename it or remove its pin. Pinning/removing does not change its
current On/Off state. A named F0 changes its permanent button label rather than
adding a duplicate. Reset name restores Headlights; the F0 button is never removed.
Naming/pinning works with track power off.

Names are limited to 24 UTF-8 bytes. Pins/names live on the ESP, shared across
browsers and preserved by normal OTA with the same filesystem layout. Unsaved
name drafts, selection and expanded state stay in the current page through polling;
they reset on reload. Active F0–F28 states live in RAM and start OFF after ESP
restart. Address changes reset them; renaming preserves them. Removing a loco
also removes its pins before its ID can be reused.

Storage accepts old `anden-roster-2` files and upgrades on the next save to
`anden-roster-3`: `L\tid\taddress\ttype\tname` records followed by
`F\tid\tfunction\tname` records. The temporary-file/rename save contains both
roster and pins. Invalid existing files disable saves rather than silently replacing
them with defaults. Filesystem erase/layout changes can still remove this data.

New endpoints (URL-encoded POST; success returns the `dcc` object):

| Path | Fields | Effect |
| --- | --- | --- |
| `/dcc/function` | `address`, `function` (0–28), `on` (0/1) | Acknowledged requested state |
| `/locos/favorite` | `id`, `function`, `name` | Pin or rename, without switching |
| `/locos/favorite/remove` | `id`, `function` | Remove pin, without switching |

`dcc.functionsSupported` gates F1–F28 using the Uno's capability reply. Each loco
returns a numeric 29-bit `functions` mask and a `favorites` array of `{number,name}`.
F0 remains compatible with `/dcc/f0`. Finder activity locks function changes and
favorite edits. HTTP 400 means invalid fields or a full pin list; 409 means a search
is active; 500 means persistence failed; 501 means the Uno needs upgrading; 503
means the new snapshot was not acknowledged. A failed request leaves saved data
and acknowledged state unchanged. Upgrade **both boards** for actual F1–F28 output.

