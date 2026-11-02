# ESP13 · web setup + OTA experiment

This is our own ESP8266 firmware for the ESP-WROOM-02/ESP13 shield. It provides
Wi-Fi setup and browser-based firmware updates. It is **not AT modem firmware**,
and does not provide a train-control protocol, DCC generation, or Uno updates.
Version 0.1.1 adds [Uno stack bench tests](../uno-stack-test/README.md): checked
serial ping/status, enable-pin readback, raw ADC values and a guarded L LED test.
The existing Pico W controller stays separate.

## What it does

- Starts an open **anden-esp13** setup network at **http://192.168.13.1/**.
- Joins a configured 2.4 GHz router network and reconnects automatically.
- Keeps the setup network available for recovery, even after joining the router.
- Advertises **anden-esp13.local**, with direct IP access as a fallback.
- Saves explicit Wi-Fi changes in the ESP8266 SDK settings, outside the sketch.
- Accepts firmware updates from the collapsed **Setup → firmware** panel.
- Provides `/api/status` with board identity, version, IPs, update space, boot ID
  and the Uno bench-test state.
- Adds web buttons for Uno ping and its L LED; both bridge enables stay LOW.
- Uses no application login or access-point password, as requested.

The app sends only checked protocol frames on the Uno UART, with no debug
logging. The ESP ROM still sends its usual boot message, which the Uno parser ignores. Leave the track/motor supply disconnected during
this experiment; the ESP firmware cannot guarantee the Uno's output state.

## One first installation, then OTA

**OTA support must already be running before a wireless update is possible.**
We have verified the ESP13 ROM bootloader's UART connection, but have not
identified a working OTA service in its existing firmware. This `.bin` is ready
for an initial serial installation; it cannot install itself over Wi-Fi.

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
adapter signals before returning the shield switches ON. These are installation
instructions, not a report of a completed hardware deployment.

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
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.1.1 --dry-run
python3 experiments/esp13-ota/ota.py --host ESP_IP \
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.1.1
```

OTA replaces the ESP application, including its update code. Always use a build
that retains OTA support. A failed boot or interrupted update may require serial
recovery; this experiment does not provide automatic rollback.

## Verification status

Version **0.1.1** compiles successfully: **361,232 bytes** (below the 448 KiB
image limit), with 29,520 bytes of static RAM. Host checks cover credential
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

References: [ESP8266 Arduino OTA documentation](https://arduino-esp8266.readthedocs.io/en/3.1.1/ota_updates/readme.html),
[Espressif esptool commands](https://docs.espressif.com/projects/esptool/en/latest/esp8266/esptool/basic-commands.html).
