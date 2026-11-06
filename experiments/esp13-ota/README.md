# ESP13 · web setup + OTA experiment

**ESP/Uno 0.6.0** adds independent **F0–F28** and named favorite functions to the
[stationary track controller](../uno-stack-test/FIRST-LOCO-TEST.md). Speed stays
zero. Older bench-release notes below remain useful for wiring and first upload.

This is our own ESP8266 firmware for the ESP-WROOM-02/ESP13 shield. It provides
Wi-Fi setup, browser-based firmware updates and a locomotive roster. Version
**0.4.0** controls independent F0 packets on the Uno through a checked UART link.
The [Uno bench firmware](../uno-stack-test/README.md) emits logic on **D10**, with
continuous track operation disabled. Setup also provides no-load DC and timed
DCC scope tests for bridge A. This is our own application, not AT modem firmware;
it cannot update the Uno. The existing Pico W controller stays separate.

## What it does

- Starts an open **anden-esp13** setup network at **http://192.168.13.1/**.
- Joins a configured 2.4 GHz router network and reconnects automatically.
- Keeps the setup network available for recovery, even after joining the router.
- Advertises **anden-esp13.local**, with direct IP access as a fallback.
- Saves explicit Wi-Fi changes in the ESP8266 SDK settings, outside the sketch.
- Accepts firmware updates from the collapsed **Setup → firmware** panel.
- Provides `/api/status` with board identity, version, IPs, update space, boot ID
  and the Uno bench-test state.
- Displays two loco cards initially, with room for eight short addresses (1–127).
- Saves names/addresses and Electric, Diesel or Steam types in LittleFS.
- Sends acknowledged F0 changes through the Uno; shows packet bytes and counters.
- Adds Uno ping and L LED controls; the LED test requires both enables LOW.
- Offers System / Light / Dark appearance, remembered by each browser.
- Uses the blue LED for Wi-Fi status and feedback on user requests.
- Adds three-second DC / DCC bridge-A tests and Stop under Setup, with no loads.
- Uses no application login or access-point password, as requested.

The app sends only checked protocol frames on the Uno UART, with no debug
logging. The ESP ROM still sends its usual boot message, which the Uno parser ignores. Leave the track/motor supply disconnected during
this experiment; the ESP firmware cannot guarantee the Uno's output state.

## Appearance

Use the **System**, **Light** and **Dark** buttons in the header. The current
choice is highlighted. System is the default and follows device appearance
changes while the page is open.
Explicit choices are stored in this browser for this origin, survive reloads,
and are applied before the page paints. Returning to System clears the override.
If storage is unavailable, the buttons still work for the current page.

The light palette uses warm cream, pale green and dark text; dark keeps the
existing slate and amber colors. Cards, dialogs, form fields and status/error
messages use the selected palette. The buttons remain usable while offline
or during an update. Changing appearance sends no request to the ESP and does
not change loco settings or trigger its activity LED. Each browser remembers
its own choice; accessing the same board through another hostname/IP creates a
separate browser origin and therefore a separate preference.

## Blue LED feedback

ESP **0.2.1** uses the DOIT shield's blue LED on **GPIO16, active LOW**:

- Blinks every 500 ms while disconnected from the configured router (including
  when only the setup access point is running).
- Stays ON when the router connection is established.
- Dips OFF for 120 ms on page loads and user requests, then returns to Wi-Fi status.
  New requests extend the dip; merely keeping the page open does not flash it.
- Automatic `GET`/`HEAD /api/status` polls and background Uno UART polls are quiet.

The server hook covers all routes, including Setup, OTA, roster commands and
errors. Feedback uses timestamps and GPIO writes, with no added delay or UART
logging. A long request can hold the LED off until the handler returns. This
GPIO belongs to the ESP; the Uno L LED test remains separate. Generic ESP8266
`LED_BUILTIN` may point at another pin and is deliberately not used.

`GET /api/status` includes `led.pin`, `activeLow`, `on`, `pinHigh` (GPIO readback)
and `activityRequests` (since boot). Readback confirms the driven pin state;
physical LED operation still needs visual confirmation on each shield revision.

Sources: [DOIT manual, LED definition](https://pdf.direnc.net/upload/esp8266-esp13-serial-wifi-shield-datasheet.pdf#page=6),
[firsthand GPIO16/active-LOW identification on a DOIT ESP13](https://forum.arduino.cc/t/esp13-shield-by-doit-is-there-a-schematic-for-this-anywhere/479363/5).

## One first installation, then OTA

**OTA support must already be running before a wireless update is possible.**
An unmodified ESP13 needs one serial installation of this application. Its
factory firmware may not offer a compatible updater. Once our web page is
running, subsequent application updates use OTA.

Before replacing the current ESP firmware, identify its flash chip and save a
full flash backup. A direct UART adapter needs **3.3 V logic**. With such an
adapter, isolate the stacked Uno UART using both shield switches OFF and connect
adapter TX → ESP **RX0**, RX → ESP **TX0**, and GND → GND. Keep the shield powered
from the Uno USB; do not add a second power feed from the adapter. A Pico can
serve as a temporary adapter, but is not required.

### Using only the Uno as the serial adapter

A classic Uno with a separate USB-to-serial chip can act as the adapter without
changing its sketch. Temporarily **unstack** the ESP13 and hold the Uno ATmega328P
in reset. With USB unplugged, connect:

| Uno | Destination |
| --- | --- |
| RESET | Uno GND |
| 5V | Shield Arduino 5V |
| GND | Shield Arduino GND |
| D0 / RX | Shield Arduino D1 / TX |
| D1 / TX | Shield Arduino D0 / RX |

Use the shield's **Arduino stacking pins**, with its existing level conversion,
not the raw 3.3 V ESP RX0/TX0 header. Set both DIP switches ON, then reconnect the
Uno USB. Keep the motor supply disconnected. Its USB port now reaches the ESP
through the crossover, while the Uno processor stays inactive.

The pin names describe the Uno processor's viewpoint: its D0 is driven by the
USB chip's TX and D1 feeds the USB chip's RX. The shield's normal stacked routing
connects the processor to the ESP; its DIP switches simply connect/disconnect
those links. They do not supply this USB programming crossover. The earlier ROM
SYNC diagnostic worked because the Uno sketch generated the ESP commands and
reported their results to the Mac.

This wiring is prepared for physical testing, not yet recorded as successful.
A [software relay with the shield left stacked](../uno-esp13-proxy/README.md) was
also tested: USB packets passed with the switches OFF but failed with them ON,
while the Uno still reported its startup. No ESP flash was written.
After flashing and validating ESP OTA, unplug USB, remove all five jumpers, and
restack the shield. The Uno's diagnostic sketch remains installed until it is
explicitly replaced.

Sources: [DOIT manual, page 9](https://pdf.direnc.net/upload/esp8266-esp13-serial-wifi-shield-datasheet.pdf#page=9),
[USB serial pin directions](https://github.com/Jaycar-Electronics/usb-serial-converter),
and [ESP13 Arduino-side programming pins](https://wiki.keyestudio.com/Ks0366_Keyestudio_ESP13_Shield_Serial_Port_Module).

### Identify, back up, then upload

Using current Espressif `esptool` and the **direct adapter's port**, hold the
shield KEY button, press/release its RST, wait one second, then release KEY.
Use `flash-id` to determine the actual size, then back up **that full size**:

```sh
python -m esptool --chip esp8266 --port ADAPTER_PORT --before no-reset --after no-reset flash-id
python -m esptool --chip esp8266 --port ADAPTER_PORT --before no-reset --after no-reset \
  read-flash 0x0 ACTUAL_FLASH_SIZE_BYTES esp13-before.bin
```

This build uses a conservative **1 MB / 64 KB filesystem** flash layout and DOUT
mode. Confirm at least 1 MB of physical flash before installing. Larger chips
can use this smaller layout. Do not erase the whole flash during later updates.

```sh
python -m esptool --chip esp8266 --port ADAPTER_PORT --before no-reset --after no-reset \
  write-flash --flash-mode dout --flash-size 1MB 0x0 build/esp13-ota/esp13-ota.ino.bin
```

Press the ESP13 RST with KEY released after the first upload. Disconnect the
adapter signals before returning the shield switches ON. The direct CP210x
installation was verified on our board; the Uno-only crossover remains untested.

## Build

Tested with Arduino ESP8266 core **3.1.2**:

```sh
arduino-cli core update-index --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli core install esp8266:esp8266@3.1.2 --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
tools/build-esp13.sh
```

Output: `build/esp13-ota/esp13-ota.ino.bin`. The build also produces a manifest
with version, core, board profile, size and SHA-256. The image limit is **448 KiB**,
leaving room for repeated updates with this layout. The web page and update
helper enforce that conservative limit and check the ESP13 app marker and image
header. These checks prevent common accidental file mix-ups; they are not a
firmware signature. The underlying core updater also checks image compatibility.

## Use it

1. Join **anden-esp13** on the Mac or phone and open **http://192.168.13.1/**.
2. Open **Setup**, enter the router SSID and Wi-Fi key, and save. Use a blank key
   only for an open router network. No credentials are compiled into the binary.
3. Check the live status for the assigned router IP. The setup AP stays available.
   Router connection attempts may briefly interrupt an AP client while its radio
   changes channel; reconnect to the setup AP if needed.
4. For future updates, choose the new **ESP13** `.bin` in Setup and upload.
   The page waits for a different boot ID before reporting the board back online.

Wi-Fi settings survive ordinary sketch OTA updates made with this same layout.
Changing layouts, erasing flash, or uploading a filesystem/settings image can
remove them. Use the firmware field only. The core also exposes its stock upload
page at `/update`; the project page adds app identity and size checks.

The command-line alternative checks target/image identity before upload and
confirms both the expected version and a new boot ID:

```sh
python3 experiments/esp13-ota/ota.py --host ESP_IP \
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.4.0 --dry-run
python3 experiments/esp13-ota/ota.py --host ESP_IP \
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.4.0
```

OTA replaces the ESP application, including its update code. Always use a build
that retains OTA support. A failed boot or interrupted update may require serial
recovery; this experiment does not provide automatic rollback.

## Roster storage and limits

The ESP owns the roster, using the same model, packet encoder and SVG illustrations
as the Pico UI. New boards start with addresses **3 and 4**; these are placeholders,
not detected decoder addresses. Names, types and addresses survive ordinary sketch
OTA with this layout. All F0 states start OFF after an ESP restart. Changing an
address also resets all its requested functions; renaming keeps them. With both
boards on 0.6.0, all five groups covering F0–F28 are refreshed.

On first use, the app initializes the reserved 64 KB LittleFS partition if it
cannot mount it; this does not overwrite SDK Wi-Fi settings. The roster uses
`/locos.cfg` with a temporary-file/rename save. A filesystem image, erase or layout
change can remove it. If storage fails, metadata saves report an error.

The UI starts with two columns on desktop and one on small screens. F0 waits for
the Uno to acknowledge the complete roster; offline controls stay disabled while
saved metadata remains editable. The historical bench-only behavior described here has been extended with
stationary track power, direction, function favorites and the address finder.
Speed control and CV programming remain unavailable. Quantitative physical pulse timing
still needs measurement for each hardware revision. See the [stack guide](../uno-stack-test/README.md)
for pins, the UART protocol and API endpoints.

## Function explorer and favorites (0.6.0)

Each loco card has a collapsed **Explore functions** panel. Open it, choose F0–F28
and use On/Off to test that function while track power is on. Function meanings
come from your decoder: F1 might be sound on one loco and something else on another.
Selecting a function alone sends no command. These controls latch On/Off; there is
no momentary horn button yet.

Enter a short name and choose **Pin favorite**. Up to eight favorites per loco
stay visible as quick toggle buttons above the collapsed panel. Select a pinned
function to rename it or remove its pin. Pinning/removing does not change its
current On/Off state. A named F0 changes the existing headlight label rather than
adding a duplicate button. Naming/pinning works with track power off.

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

## No-load motor shield test

ESP/Uno **0.4.0** adds **DCC · 3 s** to the same panel. It routes the timed waveform
through the normal D12 shield input, with no jumper, starting at a packet boundary.
The new mode needs a 17-field Uno reply and a ready roster; legacy DC tests remain
available. See the [DCC scope guide](../uno-stack-test/DCC-SCOPE.md) for capture,
phase/cutoff behavior and validation limits. `trackPower` now reports an active
DCC test; it does not measure whether motor VIN is present.

ESP/Uno **0.3.0** adds **Setup → Motor shield bench test**. It provides A + / A −
three-second DC pulses and Stop, with live enable/status reporting. This is
separate from DCC and needs empty output terminals except for a voltmeter.
Follow the [power wiring and meter guide](../uno-stack-test/POWER-TEST.md), including
VIN isolation. It is not a locomotive or calibrated overload test.

The Uno starts each pulse only on request, times it locally, rejects extension,
and stops on its raw A0 limit or a timing fault. An interrupt countdown also
limits a stalled-main-loop pulse. There is no automatic restart after a fault or
reconnection. B stays off; roster/LED commands are blocked while A is enabled.
Legacy Uno replies remain supported, but the bench buttons stay disabled until
the new firmware is detected. An ESP update does not update the Uno.

## Verification status

ESP **0.4.0** compiles to **394,768 bytes**, with **30,972 bytes static RAM**.
The native ISR phase/guard model and browser DCC/legacy-mode tests passed. The
Uno also passed VIN-disconnected USB/UART DCC bursts and automatic Stop. Powered
waveform observations belong in the DCC scope guide.

ESP **0.3.0** compiles to **393,920 bytes** with **30,940 bytes static RAM**.
Native policy and browser checks cover the fixed pulse, stop/fault states,
capability detection, legacy replies and roster lock. See the powered bench guide
for hardware observations and remaining measurement limits. The powered no-load
meter test produced about +15 V / −15 V on A; both enables returned LOW.

ESP **0.2.3** replaces the appearance dropdown with three buttons. The image is
**391,424 bytes**, with **30,852 bytes static RAM**. Checks cover exactly one
pressed button, keyboard activation, saved choices, system following, both
palettes' contrast, 44 px mobile touch targets and existing roster/Setup flows.
OTA and the live button/reload/keyboard/mobile checks passed on the ESP, with
the Uno/DCC link ready and both bridge enables LOW.

ESP **0.2.2** compiles to **391,344 bytes** with **30,852 bytes static RAM**.
Theme checks cover rendered text contrast, system appearance changes, explicit
choices across reloads, invalid/blocked storage, preserved edit drafts, offline
selection, and 320 px page/dialog layout. Both palettes were visually reviewed;
existing roster, Wi-Fi and OTA browser checks also passed. OTA installation and
live light/dark selection/reload were then checked on the ESP. The roster metadata
was preserved, the Uno/DCC link recovered, and both bridge enables remained LOW.

ESP **0.2.1** compiles to **390,448 bytes** with **30,852 bytes static RAM**.
Host checks cover disconnected/connected LED states, quiet status polling,
request pulse extension and the millis rollover. On 2026-11-03, OTA confirmed a
new 0.2.1 boot and the Uno/DCC link recovered with both enables LOW and no reported
deadline faults. Live GPIO readback verified LOW while connected, HIGH after a
user ping, and LOW again afterward; repeated status polls did not increment the
activity counter. The GPIO16 mapping is based on the DOIT ESP13 identification
linked above; visual confirmation is tracked separately from GPIO readback.

Version **0.2.0** compiles successfully: **389,344 bytes** (below the 448 KiB
image limit), with **30,844 bytes** of static RAM. Host checks cover credential
boundaries, image/target checks, and confirmed-reboot handling. Browser tests
cover Wi-Fi save/forget, setup visibility, bad files, upload failures, simulated
restarts, offline handling, and desktop/mobile layouts down to 320 px.

On **2026-11-02**, serial installation was completed on the ESP8266EX with
**2 MB physical flash**, using a CP210x adapter connected to the raw ESP UART at
**115200 baud** and a separate capable 3.3 V supply. The full original flash was
backed up privately and validated before writing. Installation and a subsequent
serial reflash both passed the programmer's flash-hash verification.

The raw-UART connection requires both shield switches OFF to isolate the Uno.
Connected DTR/RTS wires can hold the device in download mode; leave them
disconnected for a manual normal reset, release KEY, then press RST. The programmer
was intentionally left in bootloader mode after the verified write.

Application HTTP access, Wi-Fi configuration through the setup page, saved
credentials and mDNS HTTP access were subsequently verified on hardware. Two
consecutive v0.1.0 OTA uploads passed the app's expected-version/new-boot-ID checks;
the device reconnected to the router after both. The v0.1.1 stack test was then
deployed over OTA and its web-to-Uno serial link and LED test passed. A further
OTA update with the UART connected also passed, and the Uno link recovered with
both enables LOW and zero timeout/bad-frame counts. See the
[stack test report](../uno-stack-test/README.md) for the measurements and limits.

Version 0.2.0 was then deployed OTA with Uno 0.2.0. Independent F0 commands were
acknowledged by the Uno, with bridge enables LOW and no reported timing-deadline
faults. A second 0.2.0 OTA confirmed roster name/type/address persistence, F0 OFF
after reboot and restored Uno synchronization. The real gzip-served page passed
browser control and 320 px layout checks; physical pulse widths and powered
motor-shield operation remain unmeasured.

References: [ESP8266 Arduino OTA documentation](https://arduino-esp8266.readthedocs.io/en/3.1.1/ota_updates/readme.html),
[Espressif esptool commands](https://docs.espressif.com/projects/esptool/en/latest/esp8266/esptool/basic-commands.html).
