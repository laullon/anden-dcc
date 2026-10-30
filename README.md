<p align="center"><img src="assets/banner.svg" alt="anden-dcc — a small controller for trains on display" width="100%"></p>

<p align="center"><strong>Pico W · hardware-timed DCC · Wi-Fi · OTA · English / Español</strong></p>

## The magic smoke guarantee

**Use at your own discretion and risk.** This is a hobby project, provided
as-is, with no warranty. If it liberates the magic smoke from your Pico, motor
shield, decoder, or prized locomotive, congratulations: you have discovered the
least useful special effect in railway modelling. The authors accept no
responsibility for damaged hardware, derailed afternoons, or smoke that refuses
to go back inside the chip. Bring your own judgement; replacement electrons are
not included.

---

**anden-dcc** is a small controller for an H0 railway diorama: give your trains
some light, sound and, eventually, a little movement—without a full-size command
station. A Raspberry Pi Pico W hosts the panel and generates DCC in its PIO
hardware, independent of browser and Wi-Fi activity.

**Current release: 0.5.3.** A shared locomotive roster, independent F0/headlights
and short addresses are implemented. This is a **logic-output prototype**. Track
power, motor control and current protection are not enabled in firmware. The
Deek-Robot power assembly is a documented proposal for the next stage.

[Quick start](#quick-start) · [Hardware & BOM](docs/hardware/README.md) ·
[Signal verification](DCC-ANALYZER.md) · [Release validation](docs/VALIDATION.md) · [API](docs/API.md) ·
[Build](#build-the-firmware) · [Español](README.es.md)

![English control panel preview](assets/ui-desktop.png)

## A place for your trains

- **A shared locomotive roster**: two cards to start, room for up to **eight**.
- Name each train; edit its address and control **F0/headlights independently**.
- Names and short addresses **1–127** are saved on the Pico, shared by browsers.
- Every roster address keeps receiving its own F0 packet with the page closed.
- **English by default**; an EN/ES switch is remembered by your browser.
- **Setup, closed by default**, contains Wi-Fi and firmware updates.
- Saved 2.4 GHz Wi-Fi settings, reconnection and a setup access point.
- `http://anden-dcc.local`, with direct IP access as fallback.
- OTA updates through the panel; first installation and recovery use USB.
- LED connected/disconnected feedback, with a brief dip on user requests.
  Automatic status polls do not trigger it.
- Self-contained UI: no external fonts, CDN, cloud account or internet required.

The waveform illustration is not a live oscilloscope. Diagnostics show each
locomotive's requested packet and nominal timings. Measurements come from an external analyzer.

## Quick start

1. Power the flashed Pico and connect to its open **`anden-dcc`** Wi-Fi.
2. Open **`http://192.168.42.1`**, expand **Setup**, and enter your 2.4 GHz network.
3. Return to your usual Wi-Fi when the Pico's setup network disappears.
4. Open **`http://anden-dcc.local`** or the address assigned by your router.
5. Use **Edit** on each locomotive to set its name and decoder address.
6. Switch its **Headlights / F0** independently; **Add locomotive** expands the roster.

A new controller starts with **two slots at addresses 3 and 4**; change them to
match your decoders. Names and addresses survive restarts and normal OTA updates.
**All F0 states start off after reboot**; changing a card to a different address
also starts its F0 off. Renaming keeps its current F0 state. Removing a card or
changing its address stops refreshing the previous decoder; it keeps its last
state, so turn its lights off first if desired. Each function-group packet also
sets F1–F4 off. An empty roster sends DCC idle packets.

The network and updater are intentionally open for this local prototype. Your
router password is entered in Setup and stored on the Pico, not in source.

### Logic analyzer hookup

| Analyzer lead | Pico W |
| --- | --- |
| D0 / CH0 | **GP2 — physical pin 4** |
| GND | **GND — physical pin 3** |

Capture at **8 MS/s** for **100 ms**. Connect only to GPIO for this stage;
powered DCC rails require a different measurement interface.

| Address 3 | Packet | Nominal half-bit widths |
| --- | --- | --- |
| F0 off | `03 80 83` | 58 µs for 1, 100 µs for 0 |
| F0 on | `03 90 93` | 58 µs for 1, 100 µs for 0 |

Earlier single-loco Logic 2 captures decoded 14 complete packets per state
with no XOR errors. The new multi-loco DMA scheduler still needs a physical capture.
Half-bits measured 58.000–58.125 µs and 100.000–100.125 µs at 8 MS/s.
[Full signal guide](DCC-ANALYZER.md).

## Hardware & BOM

**One DCC output feeds both tracks in parallel.** Each loco is still controlled
independently by its address. The simple design keeps the **Pico W**, selected
**Deek-Robot L298P**, **15 V USB-C PD trigger**, and **MP1584 set to 5.00 V**.
There are two documented interfaces: **Option A** keeps the shield intact and
adds an SN74AHCT125N buffer; **Option B** removes the shield's CD4077 and uses
complementary Pico outputs directly. Power, sensing and default-off enable
components are still required. Bridge B is unused.

Decoder addresses are identified/set by another method.

| Document | Contents |
| --- | --- |
| [Choose a hardware version](docs/hardware/README.md) | Both build options and firmware status |
| [Option A: unmodified shield](docs/hardware/buffered-shield.md) | External AHCT buffer; no shield rework |
| [Option B: modified shield](docs/hardware/shield-modification.md) / [PDF guide](docs/hardware/shield-modification.pdf) | Remove CD4077; direct complementary Pico signals |
| [Schematic PDF](docs/hardware/schematic.pdf) / [SVG](docs/hardware/schematic.svg) | Option A: one electrical sheet, Rev C proposal |
| [Editable KiCad project](hardware/kicad/README.md) | Native schematic, ERC report and exported netlist |
| [BOM](docs/hardware/BOM.md) / [CSV](docs/hardware/bom.csv) | Option A: modules and additional parts |
| [Connections](docs/hardware/connections.csv) | Option A: exact circuit nets and pins |
| [Sources](docs/hardware/sources.md) | Manufacturer references |

**The power assembly is not built or electrically validated.** Track enable,
software current cutoff and startup stop packets still need implementation and
bench tests before powering trains. Current firmware only emits GP2 logic.

## Build the firmware

Use Arduino CLI 1.x and the **Earle Philhower Arduino-Pico core 6.3.1**.
Select **Raspberry Pi Pico W**, **1 MB sketch + 1 MB LittleFS**. OTA stages the
image in LittleFS; changing the layout can break updates.

```sh
arduino-cli core update-index --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core install rp2040:rp2040@6.3.1 --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
python3 tools/embed_web.py
arduino-cli compile --fqbn rp2040:rp2040:rpipicow:flash=2097152_1048576 --output-dir build firmware/anden-dcc
```

Or run `sh tools/build.sh`. `ARDUINO_CLI` can point to a bundled executable.
First USB install: hold BOOTSEL while connecting, then copy
`build/anden-dcc.ino.uf2` to RPI-RP2, or use picotool.

Next updates: **Setup → Firmware update**, choose **`build/anden-dcc.ino.bin`**.
The server stops GP2 before installing; the panel shows progress and reloads
after restart. **Do not upload UF2 over OTA.**

The CLI helper verifies success and the running version:

```sh
python3 tools/ota.py --host anden-dcc.local --file build/anden-dcc.ino.bin --expect-version 0.5.3
```

Use the Pico IP for `--host` if mDNS is slow. Normal firmware OTA preserves saved
Wi-Fi settings. BOOTSEL remains the recovery path.

## Tests & development

```sh
python3 tools/embed_web.py --check
python3 tests/check_dcc.py
python3 tests/check_hardware.py
python3 tools/decode_dcc_csv.py path/to/digital.csv --expected '03 80 83'
```

The host check executes the C++ encoder and roster, and models **assembled PIO
opcodes** for all 254 address/F0 combinations, pulse lengths, repeated frames,
complete packet updates, 2/3/8-loco schedules and empty-roster idle packets. The CSV decoder independently checks real analyzer exports.

Browser checks use a mocked controller, never your Wi-Fi settings or trains:

```sh
npm ci --prefix tools/ui-tests
npx --prefix tools/ui-tests playwright install chromium
npm test --prefix tools/ui-tests
```

Set `CHROME_PATH` to an installed Chrome to skip the browser download.
Screenshots go to `build/ui/` or `UI_ARTIFACTS`. Checks cover language, collapsed
Setup, independent commands, roster edits, duplicates, capacity, stale status
responses, mobile layout, offline state and failed save/upload recovery. GitHub Actions runs host checks, compilation and UI checks when hosted.

Edit `web/index.html` and `web/icon.svg`, then run `tools/embed_web.py`.
The browser tab uses `web/favicon.svg` plus a conventional ICO fallback. After
changing that SVG, run `node tools/build_favicon.cjs` with the UI dependencies
and Chromium installed, then embed the updated assets. `CHROME_PATH` is supported.
Generated `WebAssets.h` is checked in for direct Arduino IDE builds. If PIO
changes, regenerate its header with the core's `pioasm`, then run the timing
checks. `tools/export_hardware.py` exports and checks the current KiCad schematic;
`tools/draw_hardware.py` refreshes its offline print page.

```text
firmware/anden-dcc/   Pico sketch, packet encoder and PIO generator
web/                 self-contained bilingual UI and app icon
assets/              GitHub banner, icon and UI previews
docs/hardware/       wiring proposal, BOM, connections and sources
tests/               host packet/timing check
tools/               web embedding, OTA, capture decoding and UI checks
```

## Next stops

- [x] Wi-Fi, web control and OTA without a USB connection.
- [x] PIO DCC timing and F0 packets measured on hardware.
- [x] English/Spanish UI with hidden-by-default Setup.
- [ ] Build and validate the simple, single-output Deek-Robot wiring.
- [ ] Implement default-off track enable, software current cutoff and startup stop packets.
- [x] Shared multi-locomotive roster and automatic F0 refresh.
- [ ] Add sound functions and restricted movement.

## License

[MIT licensed](LICENSE). Build it, change it, share it, sell it, or teach your
model railway to run a tiny empire. Keep the license notice with your copies.

The formal warranty and liability terms are in the [MIT license](LICENSE).
This smoke-themed explanation adds no extra license restrictions.

[Changelog](CHANGELOG.md) · [Contributing](CONTRIBUTING.md) · [License](LICENSE)
