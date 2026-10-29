# DCC GPIO test

This version emits a continuous 3.3 V logic signal on GP2. It does not drive
rails, set motor speed, or enable a bridge. Connect only the logic analyzer
for this first measurement; the rails' bipolar power waveform needs a suitable
interface and must not be connected directly to these analyzer inputs.

## Wiring

| Analyzer lead | Raspberry Pi Pico W |
| --- | --- |
| GND | GND, physical pin 3 |
| CH0 / D0 | GP2, physical pin 4 |

These two Pico pins are adjacent. Physical pin numbers are different from GPIO
numbers. GP2 is GPIO2, not physical pin 2. No power wire from the analyzer is
needed; keep powering the Pico with its existing supply. Connect grounds first.

## Software and capture

For a generic 8-channel / 24 MHz analyzer, use PulseView/sigrok with the `fx2lafw`
driver if the device is FX2-based. For this user’s unit, Logic 2 2.4.48 detected a physical Logic and
its MCP server successfully captured real transitions; Logic 2 is verified
for this particular analyzer. Compatibility with other generic units varies.

- Enable D0, disable unused channels.
- Sample at 8 MHz, capture 1 million samples (125 ms), or at least 30 ms.
- A rising-edge trigger on D0 is optional; the signal repeats continuously.
- Capture raw digital timing first. UART/SPI decoding does not apply to DCC.
- Look for 58 us high + 58 us low (one), or 100 us high + 100 us low (zero).
- Each address-3 frame takes about 6.7–6.9 ms. With two locos the addresses alternate;
  per-address refresh periods depend on the full eight-word schedule.

Download: https://sigrok.org/wiki/Downloads
FX2 background: https://sigrok.org/wiki/Getting_started_with_a_logic_analyzer

The published PulseView macOS downloads are x86 builds and may use Rosetta on
Apple Silicon. This project does not install or modify capture software.

## One function: F0 / lights

Open http://anden-dcc.local (or the Pico's assigned IP). Use **Edit** on each
locomotive card to set a short address from 1 to 127, then press F0 on or off.
New controllers start with addresses 3 and 4. The roster is persisted on the
Pico; all F0 states start off after boot. Each loco is refreshed independently.

| Address 3 | Address byte | Function byte | XOR byte | Frame period |
| --- | --- | --- | --- | --- |
| F0 off | 03 | 80 | 83 | 6868 us |
| F0 on | 03 | 90 | 93 | 6700 us |

Framing, MSB first:

```text
16 × 1 | 0 | address[8] | 0 | function[8] | 0 | XOR[8] | 1
```

Every bit contains two equal-duration halves. There are no gaps between packets;
the end bit is followed by the next preamble. The XOR of all three bytes is zero.
F1–F4 are off in the function-group packet. This is intended for the usual
28/128-step decoder configuration; 14-step FL semantics are outside this test.
Changing the address stops refreshing the previous address; it does not send
an off command to it. There are no speed/stop packets in this prototype.

## Implementation

`dcc_tx.pio` runs at 500 kHz, 2 us/instruction cycle, including instruction delays.
It constructs the preamble, start/end bits and pulse lengths. A 26-bit payload
contains the address, two separators, function byte and XOR byte, MSB aligned.
`pull noblock` repeats the previous complete payload via X when the FIFO is
empty. A new command is consumed only at the next packet boundary.
The 0.5.0 scheduler feeds it with a 32-byte-aligned, eight-word DMA read ring.
A second DMA channel reloads the first at the end of each ring, without CPU
callbacks. Schedule slots repeat roster entries modulo the roster size, so
two locos alternate and every loco occurs at least once per ring (up to about
56 ms at eight locos). Counts that do not divide eight get unequal repetitions
within the ring. Empty rosters send `FF 00 FF` idle packets.

Aligned 32-bit stores replace complete packet words; the PIO still consumes
only at frame boundaries. During a roster change, old and new complete frames
can coexist briefly in the ring/FIFO. This is requested state, not readback.

PIO and two DMA channels are claimed after Wi-Fi initialization. Failure is
reported as `dcc.ready=false`; resources are not forcibly reassigned.
An OTA POST disables/aborts both DMA channels, stops PIO and holds GP2 low
before uploading. After boot it starts again with F0 off. If OTA fails, pressing an F0 button resumes the signal.
There is no motor-driver enable pin or current protection in this version.

Regenerate the checked-in PIO header after editing the assembly:

```sh
pioasm -o c-sdk firmware/anden-dcc/dcc_tx.pio firmware/anden-dcc/dcc_tx.pio.h
python3 tests/check_dcc.py
```

The Arduino-Pico tools package supplies `pioasm`. Compile with the same core,
board and 1 MB LittleFS layout documented in README.md.

API example (form POST):

```sh
curl --data 'address=3&on=1' http://anden-dcc.local/dcc/f0
curl --data 'address=3&on=0' http://anden-dcc.local/dcc/f0
```

The status endpoint reports the requested address/F0 and packet bytes for each
roster entry, plus running state and GPIO. There may be a few queued packets
before the latest command appears.
`trackPower` remains false: this GPIO signal is not an electrical track output.

## Original 0.3.0 waveform validation

- C++ encoder checked for all 254 short-address/F0 combinations.
- Simulator executes the assembled RP2040 opcodes, then checks decoded frames
  and all half-bit lengths, including frame boundaries and FIFO fallback.
- Queued off/on update checked for a complete packet boundary switch.
- Firmware compiled and deployed over OTA; Pico reports PIO ready and running.
- Real HTTP F0 on/off returned the expected bytes; invalid inputs were rejected.
- Physical digital timing now verified via Logic 2 MCP: 14 full packets each
  for F0 off/on, expected bytes, no XOR errors and no invalid bit pairs.
- One half-bits measured 58.000–58.125 us; zero half-bits 100.000–100.125 us;
  high/low asymmetry at most 0.125 us at 8 MS/s.
- Voltage, rise time and powered-track electrical characteristics are not measured.

References:

- NMRA S-9.1 timing: https://www.nmra.com/sites/default/files/standards/sandrp/DCC/S/s-9.1_electrical_standards_for_digital_command_control_2021.pdf
- NMRA S-9.2 framing: https://www.nmra.org/sites/default/files/standards/sandrp/DCC/S/s-92-2004-07.pdf
- NMRA S-9.2.1 function group one: https://www.nmra.org/sites/default/files/standards/sandrp/DCC/S/s-9.2.1_dcc_extended_packet_formats.pdf

## Decode a real CSV capture

```sh
python3 tools/decode_dcc_csv.py path/to/digital.csv --expected '03 80 83'
```

With multiple locos, omit `--expected` to accept their different packets.
The decoder still checks frame XOR and timing.

This reads the actual Saleae transition export, ignores incomplete boundary
half-bits, reconstructs the three-byte frames and checks their XOR. Measured
half-bit windows, packet counts and periods are emitted as JSON. The tool exits
nonzero on timing errors, checksum errors, no complete packets or an unexpected
packet. It is deliberately limited to this prototype's symmetric bits and
three-byte packets; it does not decode the whole DCC protocol.

The GPIO waveform validation used the actual device through Logic 2 MCP.
The original address/F0 setting was restored after capturing both states.

## UI release 0.4.0 follow-up

The PIO engine and packet encoder were unchanged for the bilingual UI release.
Host timing checks and real HTTP F0 control passed. A new capture attempt showed
constant input on every analyzer channel. A controlled comparison with the
previously measured 0.3.0 firmware also showed no transitions, after which 0.4.0
was restored. This does not establish a fresh physical timing pass; the earlier
real 0.3.0 captures remain the measurement reference. Probe/ground/input setup
should be rechecked at the bench when available.

## Multi-locomotive release 0.5.0

Host checks execute the actual roster and feed 2/3/8-loco schedules and idle
packets into the assembled-PIO simulator. The PIO program is unchanged; its
feeder now uses DMA. Physical multi-address capture remains a separate check:
capture at least 150 ms, verify both addresses and their individual F0 bytes,
and repeat while using the web UI. The earlier single-address captures do not
validate the new DMA scheduler on hardware.

DMA API reference: [Raspberry Pi Pico SDK](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_dma).
