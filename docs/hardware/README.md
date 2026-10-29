# Hardware · from signal to rails

**Revision A — design proposal, 29 October 2026.** Only the Pico-to-analyzer
connection is built and measured. The powered stage and trip circuit below are
specified for review and bench validation; they are not a tested construction
release. Firmware 0.5.0 still drives **GP2 only** and never arms a power stage.

## What is already verified

Connect analyzer D0 to **GP2 / physical pin 4**, and analyzer GND to **physical
pin 3**. Do not connect a logic analyzer directly to powered rails. Captures of
F0 off and on decoded correctly, with 58–58.125 µs and 100–100.125 µs half-bits.
See [the analyzer guide](../../DCC-ANALYZER.md).

## Documents

- [01 · USB-C power and ground](power.svg)
- [02 · Pico, level shifter and shield](control.svg)
- [03 · Proposed hardware current trip and arm latch](protection.svg)
- [Offline print pack](print.html) — three vector sheets, A4 landscape
- [Bill of materials](BOM.md) / [CSV](bom.csv)
- [Pin-by-pin connections](connections.csv)
- [Primary sources](sources.md)

The SVG sheets are printable and editable text. A named net on one sheet is the
same electrical connection on another sheet. Crossings without a junction dot
are not connections. Diagrams do not show the physical position of module pins.
Always use the labels on the received module and the specified package pinout.

## One USB-C input

Use a USB-C PD source offering **15 V / 3 A**, a 3 A USB-C cable and the selected
trigger set to 15 V. The trigger asks for a profile; it does not boost 5 V.
Confirm the negotiated voltage under load before connecting modules.

After the input fuse, split into two branches:

1. **Logic branch:** MP1584 adjusted to **5.00 V** supplies the shield's `5V`
   header, the external logic ICs, and the anode of D1. D1's cathode supplies
   Pico **VSYS / pin 39**. Pico GND joins the common return.
2. **Track branch:** S1's normally closed, DC-rated power contact supplies the
   shield's motor `VIN` screw terminal. A local 100 µF / 25 V capacitor supports
   the bridge. The shield's A/B terminals each connect to both rails of one track.

The MP1584 board is an existing module, not a new regulator design. Do not
assume its headline 3 A rating is continuous in an enclosure; logic load is
much smaller. Check its output at idle and with Wi-Fi transmitting. Its input
and output share ground. The L298P loses voltage and dissipates heat, so a 15 V
source does not mean a 15 V track waveform. Measure under representative load.

D1 is an external power-OR diode into VSYS, following Raspberry Pi's dual-supply
recommendation. It prevents USB-powered VSYS from feeding the external 5 V rail
backwards. Do **not** connect external 5 V directly to Pico VBUS, USB D+/D−,
3V3, or GPIO. The Pico's own micro-USB remains available for recovery.

On the standalone shield, the `VIN` header may carry motor voltage through its
VIN-CONNECT link. Leave that header unconnected to the Pico. No Arduino is used,
so cutting VIN-CONNECT is not inherently required. Leave IOREF, 3V3, RESET and
unused shield headers unconnected in this proposal. Verify the actual clone's
power and current-sense circuit before applying motor power.

## Why the level shifter is included

The L298's 2.3 V minimum high level alone would accept Pico outputs. However,
the Arduino reference shield also has **CD4077B logic** powered at 5 V, whose
specified high-level minimum is **3.5 V**. A Pico's 3.3 V is insufficient for a
guaranteed high level at that gate. The user's Deek-Robot photo shows a CD4077
family device; check the actual marking when it arrives.

U1, **SN74AHCT125N**, runs at 5 V and accepts TTL-level inputs. Four channels
translate DCC-A, DCC-B and the two output enables. **Do not substitute a plain
74HC125** without checking its input thresholds. The buffer's active-low OE
pins share `BUFFER_DISABLE`, the complement of the hardware arm latch.

Shield A uses D12/D3/D9 (direction/enable/brake); B uses D13/D11/D8. Brake inputs
are tied low so each direction signal produces complementary bridge inputs.
ENA and ENB have 10 kΩ pull-downs so they stay low when the buffers are disabled.
Each U1 input also has a 10 kΩ pull-down. Both rails are bridge outputs: **neither
rail is logic ground**. Keep both tracks electrically separate if their outputs
can differ. Do not let a wheel bridge between independent A/B outputs.

## Proposed firmware pin allocation

| Pico GPIO | Physical pin | Purpose | Firmware 0.5.0 |
| --- | ---: | --- | --- |
| GP2 | 4 | DCC-A → U1 → shield D12 | Active, measured logic signal |
| GP3 | 5 | DCC-B → U1 → shield D13 | Reserved, not driven |
| GP4 | 6 | Rising-edge ARM clock to U2 | Reserved; external pull-down |
| GP5 | 7 | DISARM → Q1 → latch `/CLR` | Reserved; external pull-down |
| GP6 | 9 | Enable A → U1 → shield D3 | Reserved; external pull-down |
| GP7 | 10 | Enable B → U1 → shield D11 | Reserved; external pull-down |
| GP26 / ADC0 | 31 | Divided A current sense | Reserved |
| GP27 / ADC1 | 32 | Divided B current sense | Reserved |
| GP28 / ADC2 | 34 | Divided latch-disabled feedback | Reserved |
| VSYS | 39 | D1 cathode, external supply | Power input |
| GND | 3 / other GND | Common signal return | Required |

This is a proposed future pin assignment, not an instruction to power rails with
0.5.0. GP2 changing while the bridge is disabled is intentional.

## Current measurement and trip latch

Each shield sense output (A0/A1) passes through **10 kΩ over 10 kΩ**, with **1 nF**
from the midpoint to GND. This halves a possible 5 V output before the Pico ADC
and the comparator. The divider ratio remains below 3.3 V for a 5.5 V source,
including 1% resistor tolerance. It must never be connected to a rail output.

Assuming the Arduino shield's **nominal 1.65 V/A** sense gain, the divided signal
is **0.825 V/A**. This is a reference value, not a calibration of the Deek-Robot.
With U3's comparator reference made by 40.2 kΩ from 5 V and 10 kΩ to GND:

```
VREF = 5 × 10 / (40.2 + 10) = 0.996 V
Nominal current trip = 0.996 / 0.825 = 1.21 A per channel
```

A common LM393 compares each divided sense signal at its negative input with
VREF at its positive input. Either open-collector output pulls `/CLR` low on an
overcurrent. That asynchronously clears U2 (74HCT74), making `/Q` high and
tri-stating all U1 outputs. ENA/ENB then fall low through their resistors.
The reset remains latched after current disappears; a fresh ARM clock edge is
required to re-enable. This avoids uncontrolled automatic retry/chopping.

U4, **MCP120-485** (open drain, no internal pull-up), clears the latch during
power-up and brownout. The -485 option has a nominal 4.725 V threshold; check
the suffix rather than substituting a lower-threshold part. Its active-low reset
output, both comparator outputs, S1's NO contact and Q1's collector share `/CLR`.
A 4.7 kΩ pull-up holds `/CLR` high when all release. S1's NO contact grounds
`/CLR` when pressed; its NC contact independently disconnects the track supply.
Logic remains powered so future firmware can report the stop condition.

U2 D and `/PRE` are high, so only a deliberate low-to-high ARM edge sets Q.
The complement `/Q` drives U1 OE pins and, through a third 10 kΩ/10 kΩ divider,
GP28. Q1 is a 2N3904 open-collector disarm path controlled through a 10 kΩ base
resistor from GP5, with 100 kΩ base-to-emitter. Do not substitute a transistor
by package shape alone; verify E/B/C against its manufacturer's datasheet.

The unused second flip-flop has D and CLK tied to GND, `/PRE` and `/CLR` tied to
5 V, with Q and `/Q` unconnected. Fit 100 nF at each IC supply. No logic input
should float. A hardware watchdog or power-stage deadman remains a future
improvement: this latch trips on current, stop or brownout, not on every possible
firmware failure. The planned firmware must disarm before OTA, reset or changes
to the DCC engine. The present firmware does not control any of these power pins.

## Bench acceptance sequence before connecting a train

1. Inspect the received shield and continuity of its power links; compare
   D12/D13, D3/D11, D9/D8 and A0/A1 with the reference. Check for clone differences.
2. With boards disconnected, confirm USB PD voltage and set MP1584 to 5.00 V.
3. Power logic only. Confirm VSYS voltage and no external-rail backfeed when the
   Pico is powered by its USB. Confirm the arm latch starts cleared.
4. Verify each buffer output at 5 V, the ADC divider voltage at known inputs,
   and the trip threshold using a current-limited bench supply and dummy load.
5. Measure latch clearing, bridge disable delay, transient overshoot, thermal
   behavior and emergency-stop operation on the actual assembly. They have not
   been measured by the project's digital signal capture.
6. Implement and test the reserved enable/arm/disarm/ADC firmware, including
   speed-zero startup packets before enabling rails and an emergency-stop path.
   This function-only firmware does not order a decoder motor to stop. Start below
   the nominal 1.21 A threshold and check decoder sound/lighting startup current.
7. Only then validate powered DCC on a suitable isolated measurement interface
   and a single decoder. Increase loads after checking cooling and source limits.

The input fuse protects wiring; it is not the fast per-track trip mechanism.
Two 1.2 A track limits plus logic leave useful margin within a 15 V / 3 A source.
DCC-EX describes about 1.3–1.5 A as a realistic shield output with cooling; that
is not a guarantee for a particular clone or enclosure. Sound decoders and
capacitor-equipped stock may need inrush handling after measurements.
