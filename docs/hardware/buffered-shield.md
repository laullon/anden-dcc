# Option A: unmodified shield + level shifter

[Compare both hardware options](README.md) · [Option B: modified shield](shield-modification.md)

**Revision C proposal.** Keep the shield's CD4077 in place and use the external
SN74AHCT125N buffer. This is the version covered by the existing KiCad schematic,
BOM and connections CSV. It does not require the GP3 connection used by Option B.

This assembly is not built or electrically validated. Firmware **0.5.3 produces
logic on GP2 only**; track enable and current cutoff remain unimplemented.

- [Single-sheet electrical schematic (PDF)](schematic.pdf)
- [Zoomable schematic (SVG)](schematic.svg) / [print page](print.html)
- [Editable KiCad project](../../hardware/kicad/anden-dcc.kicad_pro)
- [BOM](BOM.md) / [CSV](bom.csv)
- [Pin-by-pin wiring](connections.csv) / [primary sources](sources.md)

## What we are building

Use **bridge A** of the Deek-Robot L298P to feed **both tracks in parallel**.
Each locomotive keeps its own DCC address and independent controls. Two locos
do not require two power outputs. Bridge B is disabled and its rail terminals
are left unconnected. Decoder programming is out of scope.

```text
USB-C PD, 15 V ── fuse ──┬── track-power switch ── shield VIN
                       └── MP1584, 5.00 V ── logic + diode ── Pico VSYS

Pico GP2 ── 74AHCT125 ── shield D12 / direction A
Pico GP6 ── 74AHCT125 ── shield D3 / enable A
Shield A0 / sense ── 10k + 10k divider ── Pico GP26

Shield A+ ── one rail of each track
Shield A− ── the other rail of each track
```

The **only extra logic IC** is an SN74AHCT125N level shifter. It lets the Pico's
3.3 V outputs drive the shield's 5 V logic. Use AHCT, not HC. The shield's CD4077
direction logic is why we are keeping it.

## Power connections

1. Set the USB-C trigger to **15 V**, using its actual printed DIP table. The
   charger must offer 15 V at 3 A; the trigger does not boost a 5 V supply.
2. After the 3 A fuse, split the supply. S1 switches motor VIN; the other branch
   feeds the MP1584. The logic can stay on while track power is switched off.
3. Adjust the MP1584 to **5.00 V** before connection. Its output feeds the shield
   `5V` header and U1 pin 14. D1's anode goes to that 5 V rail; its band/cathode
   goes to Pico **VSYS, physical pin 39**.
4. Join PD−, buck IN−/OUT−, shield GND, Pico GND and U1 pin 7. Keep heavy motor
   returns away from the ADC return. Neither track rail goes to logic GND.

Leave the shield VIN header unconnected to the Pico. Do not connect external
5 V to Pico VBUS or 3V3_OUT. The Pico's USB connection remains usable for recovery.

## Signal wiring

| Function | Pico | U1 SN74AHCT125N | Shield |
| --- | --- | --- | --- |
| DCC waveform | GP2 / physical 4 | input 2 → output 3 | D12 / DIRA |
| Track enable, future firmware | GP6 / physical 9 | input 5 → output 6 | D3 / PWMA |
| Current measurement, future firmware | GP26 / physical 31 | 10k/10k divider and 1 nF; no buffer | A0 / SNSA |

U1 pins **1, 4, 7, 9 and 12 → GND**; pins **10, 13 and 14 → +5V**.
Outputs **8 and 11 remain unconnected**. These ties disable the unused gates.
R1/R2 hold the Pico-side control inputs low during reset; R3 holds shield ENA low.
Tie shield **D9, D8, D11 and D13 to GND**, disabling braking and unused bridge B.
Shield A1 and B+/B− remain unconnected.

## Basic current cutoff

Current protection is planned **in firmware**, using GP26 to switch GP6 off and
latch the output off until manually reset. The removed comparator/latch chips
are not part of this design. This is not an independent hardware overcurrent trip.

The shield's nominal 1.65 V/A sense gain becomes **0.825 V/A** after the divider.
An initial bench target of 0.8 A would correspond to 0.66 V, approximately 819
ideal 12-bit counts at 3.3 V. These are calculations, not calibration of the clone
or an implemented setting. The limit applies to both locos and all stock combined.
The fuse protects supply wiring; it is not the decoder-current cutoff.

Before enabling rails, the firmware needs a measured/calibrated cutoff that keeps
working during HTTP activity, default-off enable, startup speed-zero packets,
and disarm on OTA or faults. Verify those using a current-limited source and a
dummy load first. The L298P's thermal/current capability still needs measurement
and appropriate cooling; a product's “2 A” title is not a continuous guarantee.
