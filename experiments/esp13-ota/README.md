# ESP13 · web setup + OTA experiment

This is our own ESP8266 firmware for the ESP-WROOM-02/ESP13 shield. It provides
Wi-Fi setup and browser-based firmware updates. It is **not AT modem firmware**,
and does not yet provide a train-control protocol, DCC generation, or Uno updates.
The existing Pico W controller stays separate.

## What it does

- Starts an open **anden-esp13** setup network at **http://192.168.13.1/**.
- Joins a configured 2.4 GHz router network and reconnects automatically.
- Keeps the setup network available for recovery, even after joining the router.
- Advertises **anden-esp13.local**, with direct IP access as a fallback.
- Saves explicit Wi-Fi changes in the ESP8266 SDK settings, outside the sketch.
- Accepts firmware updates from the collapsed **Setup → firmware** panel.
- Provides `/api/status` with board identity, version, IPs, update space and boot ID.
- Uses no application login or access-point password, as requested.

The app does not print to the shared Uno UART or drive its pins. The ESP ROM still
sends its usual boot message. Leave the track/motor supply disconnected during
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

The Mac's Uno USB connection is not a direct ESP programming port while stacked.
The Uno has its own firmware and shares D0/D1 with the USB serial chip; do not run
an ESP flashing command against that port assuming it reaches the ESP directly.

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
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.1.0 --dry-run
python3 experiments/esp13-ota/ota.py --host ESP_IP \
  --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.1.0
```

OTA replaces the ESP application, including its update code. Always use a build
that retains OTA support. A failed boot or interrupted update may require serial
recovery; this experiment does not provide automatic rollback.

## Verification status

Version **0.1.0** compiles successfully: **353,952 bytes** (below the 448 KiB
image limit), with 29,276 bytes of static RAM. Host checks cover credential
boundaries, image/target checks, and confirmed-reboot handling. Browser tests
cover Wi-Fi save/forget, setup visibility, bad files, upload failures, simulated
restarts, offline handling, and desktop/mobile layouts down to 320 px.

These are software checks. First boot, saved credentials, mDNS on the actual
network, and consecutive OTA updates still require an initial installation and
physical validation.

References: [ESP8266 Arduino OTA documentation](https://arduino-esp8266.readthedocs.io/en/3.1.1/ota_updates/readme.html),
[Espressif esptool commands](https://docs.espressif.com/projects/esptool/en/latest/esp8266/esptool/basic-commands.html).
