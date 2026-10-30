# anden-dcc: L298P shield modification

30 October 2026 · Proposed interface · Bridge A only

**Status: not built or electrically validated. Firmware 0.5.3 still generates DCC on GP2 only. Do not power the tracks using this modification until complementary output, track enable and current cutoff are implemented and tested.**

## Decision

Remove the Deek-Robot shield's CD4077 and let the Pico W drive the two L298P bridge-A inputs directly. This eliminates the external 74AHCT125 or level-shifter module. Keep the shield's logic supply at 5 V and its motor supply at the planned 15 V. Bridge A feeds both tracks in parallel; bridge B stays disabled. Decoder programming is out of scope.

The unmodified shield remains an alternative if a level shifter is used. This guide describes the modified version only. The existing Revision C KiCad schematic and BOM still show the buffered version; do not combine those signal connections with this guide.

## Why it works

- The L298P accepts HIGH inputs from 2.3 V, compatible with Pico 3.3 V GPIO. Its logic supply must remain at least 4.5 V: 3.3 V signals do not imply a 3.3 V supply.
- The CD4077 needs a guaranteed HIGH of 3.5 V at a 5 V supply. It combines direction and braking; with braking low it inverts DIRA to produce the other bridge input.
- The Pico will produce complementary signals together in one PIO state machine: GP2/GP3 = 1/0, then 0/1. No software loop or second, independently timed waveform is needed.

## Final signal wiring

| Function | Pico W | Modified shield | L298P PowerSO20 |
| --- | --- | --- | --- |
| DCC | GP2, physical pin 4 | D12 / DIRA | IN1, pin 7 |
| Inverted DCC | GP3, physical pin 5 | Former CD4077 PCB pad 3 | IN2, pin 9 |
| Track enable | GP6, physical pin 9 | D3 / PWMA | ENA, pin 8 |
| Common ground | GND, physical pin 3 | GND | Ground net |

Use these **PowerSO20** pin numbers for the surface-mount L298P. The common upright L298N/Multiwatt package has different numbers. Confirm the actual clone by continuity before removing anything.

Keep a **10 kohm resistor from D3 / ENA to GND**, physically at the shield. Hold D11 / ENB low, leaving B+ and B- unconnected. Keep the previous D8, D9 and D13 ground ties; they no longer provide bridge-A braking after chip removal. Current sensing stays on A0 through the existing 10k/10k divider and 1 nF filter to GP26; its calibration/cutoff is still future work.

## Modification procedure (all power disconnected)

1. Unplug USB, PD power and every Pico/shield connection. Photograph the board and identify the CD4077 marking and pin-1 indicator. Do not confuse it with the LM358 current-sense amplifier.
2. Verify continuity: D12 to L298P pin 7; CD4077 pin 3 to L298P pin 9; D3 to L298P pin 8. If these do not match, stop and trace the actual clone before applying this pin map.
3. Remove the entire CD4077 with suitable SMD rework tools. Leave the L298P and current-sense amplifier in place. Clean the pads and inspect for lifted copper and solder bridges. Removing the whole chip avoids leaving an unused gate at undefined input levels.
4. Solder the GP3 lead to the **PCB pad formerly occupied by CD4077 pin 3**. It must connect to L298P IN2, not to another powered output. Add strain relief. Never attach GP3 while the CD4077 output remains connected.
5. Connect GP2 to D12, GP6 to D3 and common GND as listed. Fit/check the ENA pulldown. Omit the external AHCT buffer and its associated buffer-only wiring; do not remove unrelated power, sensing or enable components.

CD4077 SOIC-14, viewed from above with the notch at the top: left pins run 1 through 7 downward; right pins run 14 through 8 downward. Pad 3 is the third pad down on the left. Follow the chip's indicator and continuity, not the orientation of a product photo.

## Verification before connecting a locomotive

- With power off: confirm the GP3 solder point reaches L298P IN2 and there are no accidental shorts to adjacent pads, 5 V or ground. Confirm approximately 10 kohm from ENA to GND and ENB held low.
- With motor VIN disconnected: check the 5 V logic supply and that ENA remains low through startup, reset and OTA. After the firmware update, use the analyzer on GP2/GP3 to verify opposite levels and matching 58/100 us half-periods. Check the signals also reach the L298P input pins.
- Only after enable and current cutoff pass their checks: test bridge A with a current-limited motor supply and dummy load, then a locomotive. Neither rail is logic ground. Do not connect the generic logic analyzer directly to powered track outputs.

## Firmware still required

Generate GP2 and GP3 together with unchanged DCC timing; hold GP6 low until valid output is established; disable ENA before stopping PIO, during OTA and on faults; implement and calibrate current cutoff and startup speed-zero packets. The present GP2-only release does not implement these power-stage controls. No firmware is deployed by this document.

## Primary references

- [Arduino Motor Shield Rev3 schematic](https://www.arduino.cc/en/uploads/Main/arduino_MotorShield_Rev3-schematic.pdf): DIRA, inverted DIRA and PWMA wiring. This is a reference design, not proof of clone routing.
- [ST L298 datasheet](https://www.st.com/resource/en/datasheet/l298.pdf), pages 4-5: PowerSO20 pinout, supply minimum and input thresholds.
- [TI CD4077B datasheet](https://www.ti.com/lit/ds/symlink/cd4077b.pdf), pages 2-4: pinout and 5 V input threshold.

Use at your own discretion. The magic smoke is not a commissioning indicator.
