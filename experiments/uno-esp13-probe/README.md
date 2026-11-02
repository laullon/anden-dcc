# Uno + ESP13 diagnostic experiment

This is a separate hardware experiment, not a replacement for the working Pico W
firmware. It checks the UART connection between an ATmega328P Uno and a DOIT-style
ESP13 Wi-Fi shield. It does not generate DCC, configure Wi-Fi, or flash the ESP.

Keep the motor/track supply disconnected during this experiment. The sketch sets
the standard L298P shield enable pins D3 and D11 LOW and its direction/brake pins
LOW. Software cannot keep these outputs driven during an Uno reset or upload.

The ESP13 shares Uno D0/D1 with the USB serial interface. On the board under test,
both blue DIP switches must be OFF for an Uno upload and ON for the UART test.
Other shield revisions may route these switches differently.

## Upload and run

Use separate build and source directories; do not set Arduino CLI's export
directory to the build directory.

```sh
arduino-cli compile --fqbn arduino:avr:uno \
  --build-path /tmp/anden-uno-probe-build experiments/uno-esp13-probe
# Both ESP13 switches OFF before uploading:
arduino-cli upload --fqbn arduino:avr:uno --port /dev/cu.usbserial-110 \
  --input-dir /tmp/anden-uno-probe-build
python3 experiments/uno-esp13-probe/read_probe.py \
  --port /dev/cu.usbserial-110 --monitor --seconds 180
```

The Python reader uses macOS/POSIX serial APIs, needs no third-party packages,
and resets the Uno when opening its port. Choose the port belonging to your Uno;
the example port is specific to the development Mac.

Once `BOOT_MONITOR_READY` appears, set both ESP13 switches ON and press the
**ESP13's own RST button**, not the Uno RESET. Press twice about a second apart
to reduce the chance of missing the brief reporting window.

The sketch first sends read-only `AT` probes at 115200, 9600, 57600, 38400,
19200 and 74880 baud, requesting `AT+GMR` only after an `OK`. It then passively
listens for the ESP8266 ROM startup message at 74880 baud in eight-second windows.
Each window reports a byte count and the first 64 bytes in hex at 115200 baud.
The shared UART is not used as a continuous echo bridge, which could feed ESP
responses back into its command input.

Version 3 also sends one ESP ROM `SYNC` packet at 115200 baud after each listen
window. To test this independently of the installed firmware, hold the ESP13
**KEY** button, press and release its **RST**, wait one second, then release KEY.
A startup message with first boot-mode value `1` confirms UART bootloader mode.
`ROM_SYNC status=OK` requires a correctly framed, successful SYNC response;
ordinary boot text, packet echoes, and error replies do not count.

This experiment sends no erase, flash/RAM download, or register-write commands.
The packet follows [Espressif’s serial protocol](https://docs.espressif.com/projects/esptool/en/latest/esp8266/advanced-topics/serial-protocol.html).
After testing, press ESP13 RST with KEY released to return to its installed
firmware.

The host prints only structured diagnostic lines. Its raw log stays locally in
`/tmp/anden-uno-esp13-probe.log` (override with `--log`); review it before sharing
because arbitrary pre-existing ESP firmware may emit private data. Without
`--monitor`, the reader stops after the AT scan.

## Initial observations

On 2026-11-02 the Uno bootloader identified an ATmega328P and the diagnostic was
uploaded successfully with both switches OFF. With both switches ON, all six
AT probes received zero bytes. The ESP13 power LED is lit and the user confirmed
soldered, stacked headers. This does **not** establish whether the ESP has AT
firmware or whether both UART directions work.

The startup-log test subsequently captured 630 bytes after each of two ESP13
resets. The decoded prefix was:

```text
ets Jan  8 2013,rst cause:2, boot mode:(3,6)
load 0x401000
```

This confirms the ESP ROM is running and the ESP-to-Uno receive path works. It
does not verify the Uno-to-ESP transmit path or identify the installed firmware.
The first boot-mode value of 3 denotes booting from flash; see the
[Espressif SDK API guide](https://www.espressif.com/sites/default/files/2c-esp8266_non_os_sdk_api_guide_en_v1.5.4.pdf).

Repeating the six AT probes after those resets still received zero bytes.
