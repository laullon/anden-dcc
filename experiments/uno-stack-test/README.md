# Uno + ESP13 + L298P: bench firmware

**Current: ESP/Uno 0.6.0** adds F0–F28 and named function favorites. See the
[stationary controller guide](FIRST-LOCO-TEST.md) and
[function explorer/API notes](../esp13-ota/README.md#function-explorer-and-favorites-060).
Historical bench-only descriptions below predate track mode.

**New: ESP/Uno 0.5.0** adds [stationary track power, direction and guided address discovery](FIRST-LOCO-TEST.md). Speed stays zero. The sections below describe the historical 0.4.0 bench release; use the linked guide for current track controls and protocol.

Version **0.4.0** combines the eight-loco F0 generator on **D10 / OC1B** with
separate **three-second, no-load DC and DCC tests on bridge A**. The ESP hosts the web
controls; the Uno owns pulse timing and cutoff. Both enables are LOW at startup
and idle. **No trains or loads:** there is no continuous track mode.

Start USB-only as below. For motor VIN and the meter test, follow the
[powered bench guide](POWER-TEST.md), including VIN isolation before 15 V.
The [timed DCC scope guide](DCC-SCOPE.md) covers the new D12 route; no jumper is needed.

## Setup

Power the stacked boards from the Uno USB. Remove any separate USB-UART adapter
signal wires; the ESP uses its normal stacked UART connection and level conversion.

1. Build both sketches with the repository's local serial library:

   ```sh
   tools/build-uno-stack.sh
   tools/build-esp13.sh
   ```

2. Turn both ESP13 switches **OFF** for an Uno USB upload:

   ```sh
   arduino-cli upload --fqbn arduino:avr:uno --port UNO_PORT \
     --input-dir build/uno-stack-test
   ```

3. Update the already-installed ESP firmware over Wi-Fi:

   ```sh
   python3 experiments/esp13-ota/ota.py --host ESP_IP \
     --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.4.0
   ```

4. Turn both switches **ON** to connect Uno and ESP. No reset is required.
5. Open the ESP page. Two locomotive cards start at addresses **3 and 4**;
   edit these to match your decoders. Each has independent F0 controls and packet
   bytes. You can add up to eight, with Electric, Diesel or Steam illustrations.
6. The **Uno + motor shield** card still provides **Ping Uno**, **Blink Uno L LED**,
   enable readback and raw ADC counts. Setup stays collapsed by default.

Names, types and addresses are stored on the ESP and survive ordinary OTA.
F0 starts OFF after an ESP reboot; renaming keeps its state, changing the address
resets it. Removing/changing an address stops refreshing the old decoder; it
would retain its last function state on a powered track. F1–F4 are OFF in these
function-group packets. An empty roster sends idle packets. Speed, other
functions, CV programming and the Pico address finder are not implemented here.

The build scripts supply `--libraries libraries`. To use Arduino IDE directly,
install/copy `libraries/AndenStackLink` into its sketchbook libraries directory.
The Uno sketch uses AVR core 1.8.7; the ESP uses ESP8266 core 3.1.2.

## Pin states

| Function | Uno pin | Test behavior |
| --- | --- | --- |
| DCC logic test output | **D10 / OC1B** | Repeating 5 V logic waveform |
| Shield enable A | D3 | LOW except during the fixed bench pulse |
| Shield enable B | D11 | Held LOW |
| Shield brake A / B | D9 / D8 | LOW |
| Shield direction A | D12 | LOW at idle; DC polarity or DCC during a bench test |
| Shield direction B / Uno L LED | D13 | Eight short flashes on request, then LOW |
| Current-sense A / B | A0 / A1 | Raw ADC readings; not converted to amperes |
| Uno RX / TX | D0 / D1 | Checked UART messages to/from ESP |

**Do not jumper D10 to D12 in this build:** D12 is driven as an output, so that would
connect two outputs together. D10 is reserved for an isolated timing test.
Timer1 is reserved too; its OC1A output is disconnected, so D9 remains an
ordinary GPIO. Do not add a Servo library or another Timer1 user to this sketch.

D13 is also the bridge B direction input. The web LED endpoint first requests
fresh pin states and rejects the action if either enable is HIGH. The Uno keeps
B LOW and A LOW except during an explicit no-load pulse. During reset/upload the
MCU cannot hold outputs, which is why motor VIN stays disconnected for uploads.
Power LEDs stay on. ESP firmware 0.2.1 also uses its blue GPIO16 LED for
[Wi-Fi/request feedback](../esp13-ota/README.md#blue-led-feedback). Powered
motor-output indicators remain untested.

## Timing and analyzer check

Timer1 runs at 2 MHz (16 MHz CPU / 8). OC1B toggles D10 in hardware; the interrupt
schedules the next compare using the previous deadline, with **116 ticks / 58 µs**
for a one and **200 ticks / 100 µs** for a zero. Both halves use the same duration.
Packets have 16 preamble ones, separators, address, F0 instruction, XOR checksum
and an end bit. Every roster address repeats in turn, even with the page closed.

A complete roster is packed into an inactive buffer and published only at a
whole-packet boundary. The interrupt stops the test output and records a fault
if it cannot schedule the next edge in time. Packet/fault counters are firmware
telemetry; they do not measure the physical pulse widths.

When an analyzer is available, first confirm that its inputs tolerate **5 V**
(or use a suitable input interface). Then connect CH0 to **D10** and analyzer GND
to **Uno GND**, with motor VIN/tracks still disconnected. Capture at 8 MS/s for
100 ms and check both states for each address:

| Address | F0 off | F0 on |
| --- | --- | --- |
| 3 | `03 80 83` | `03 90 93` |
| 4 | `04 80 84` | `04 90 94` |

Expected half-bit widths are 58 / 100 µs. Decode with
`tools/decode_dcc_csv.py`; an eight-loco roster should refresh all eight addresses.
Do not connect an analyzer directly to powered DCC rails. **Physical waveform
measurement on the Uno is pending**; the analyzer was unavailable for this stage.

Timer reference: [Microchip ATmega328P datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf).

## Wire protocol and API

The shared `AndenStackLink` library frames bounded ASCII messages as
`@BODY*CRC16\n`, with a 144-character body limit (96 before 0.6.0), CRC16-CCITT (initial 0xFFFF), and
resynchronization at the next `@`. It rejects corrupt, overlong, empty-field and
malformed messages. ROM startup noise is ignored. Responses must carry the
current nonzero request ID; this is not a continuous echo bridge.

Requests:

```text
AD1,id,PING
AD1,id,STATUS
AD1,id,LED
AD1,id,PULSE,0|1
AD1,id,DCCTEST
AD1,id,BENCHSTOP
AD1,id,ROSTER,count,addr1,f01,addr2,f02,...
```

A roster request supplies all addresses and F0 states together (count 0–8;
addresses 1–127; F0 0/1). The Uno acknowledges after the snapshot is active.
The ESP reconciles its desired roster after reconnecting to the Uno.

Version 0.4.0 response:

```text
AD1,id,STATE,version,adcA,adcB,flags,ledEdges,dccRunning,dccCount,dccSignature,dccMask,packetCount,timingFaults,benchRemainingMs,benchState,benchMode
```

Flag bits 0..5 report enable A, enable B, brake A, brake B, direction A, direction B.
`dccSignature` is a CRC16 of count, mask and address list. The ESP also accepts the
old eight- and fourteen-field responses during staged upgrades. Bench controls
need a sixteen-field response; the DCC scope test needs seventeen fields. Roster controls wait until
a capable Uno is online and has acknowledged the current roster.

The ESP polls status every two seconds. Timeout counts include periods when the
switches were intentionally OFF. ADC values are uncalibrated and can vary with
USB/RF noise. The no-load pulse has a raw A0 cutoff, described in the powered
bench guide; calibrated current/overload protection remains future work.

- `GET /api/status` includes `uno` and `dcc`; `trackPower` reports an enabled
  DCC scope burst, not a measurement of supply voltage.
- `POST /dcc/f0` takes `address` and `on=0|1`; requires the Uno snapshot ACK.
- `POST /locos/save` takes optional `id`, `address`, `name`, and `type`.
- `POST /locos/remove` takes `id`. Metadata edits persist even if the Uno is offline;
  F0 controls wait until synchronization succeeds.
- `POST /api/uno/ping` returns a fresh checked Uno response, or HTTP 503.
- `POST /api/uno/led` requires a fresh reply with both enables LOW; HTTP 409 blocks
  an unsafe enable state. A successful reply starts the short L LED pattern.
- `POST /api/uno/bridge` takes `direction=0|1`; requires a fresh capable Uno
  response with both enables LOW. The pulse length is fixed on the Uno.
- `POST /api/uno/dcc-test` starts a fixed DCC burst with a ready roster.
- `POST /api/uno/bench-stop` requests immediate local bench cancellation.
- ESP OTA uses `/update`; it updates only the ESP, not the Uno.

The Uno UART is also connected to its USB converter. Switches OFF isolate the ESP
for Uno uploads/direct USB commands; switches ON connect the stack. With ON, use
the ESP API rather than writing from the Mac into that shared serial input.

## Validation, 2026-11-02–03

- Uno **0.2.0**: **7,122 bytes flash**, **539 bytes static RAM**; uploaded over USB.
- ESP **0.2.0**: **389,344-byte image**, **30,844 bytes static RAM**; deployed OTA.
- The earlier v0.1.0/v0.1.1 stack test verified serial communication and the L LED
  blink visually, with bridge enable readbacks LOW.
- Native C++ tests decode complete packets, equal half-bit durations, XOR,
  idle/eight-loco refresh and updates at every one of 88 half-bit phases. Shared
  roster/packet headers are checked against the original Pico implementation.
- Twenty eight-loco snapshot requests were acknowledged over Uno USB while the
  packet counter advanced; the deadline-fault counter stayed zero.
- Real web → ESP → Uno F0 on/off tests passed independently for addresses 3 and 4.
  All sampled enables were LOW, with no reported deadline faults.
- An additional same-version OTA confirmed a new ESP boot, preserved a temporary
  steam card's name/type/address, reset F0 OFF and recovered the Uno link. The
  temporary card was removed and the original two-card roster restored afterward.
- The real gzip-served page passed desktop/mobile (320 px) browser checks, with
  independent F0/packet updates and no JavaScript errors. Mocked browser tests
  also cover roster edits, duplicates, stale polls, failures and offline handling.

See the [powered bench guide](POWER-TEST.md) for the subsequent 0.3.0 checks.

**Not yet tested by the earlier logic checks:** physical D10 pulse widths, powered bridge operation, rail
waveforms, current/overload behavior under load, or control of an actual decoder.
