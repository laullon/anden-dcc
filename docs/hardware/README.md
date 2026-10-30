# Hardware: choose your build

Both versions stay in this repository. Choose one interface; they share the
Pico W, Deek-Robot L298P shield, USB-C PD supply, MP1584 buck and two-track layout.

| | Option A: keep the shield intact | Option B: modify the shield |
| --- | --- | --- |
| CD4077 on the shield | Leave installed | Remove |
| External logic | SN74AHCT125N level shifter | None |
| DCC signals from the Pico | GP2; shield creates the inverse | GP2 + GP3, complementary outputs |
| Track enable | GP6 through the buffer | GP6 directly to D3 / ENA |
| Board rework | None | Remove CD4077; wire GP3 to its former PCB pad 3 |
| Build guide | [Buffered shield guide](buffered-shield.md) | [Modification guide](shield-modification.md) |
| Printable document | [Rev C schematic](schematic.pdf) | [Illustrated modification guide](shield-modification.pdf) |
| Detailed design files | [KiCad](../../hardware/kicad/anden-dcc.kicad_pro), [BOM](BOM.md), [CSV](bom.csv), [connections](connections.csv) | Wiring and changes to the buffered assembly are described in the modification guide; no separate KiCad schematic or BOM yet |

**Option A** suits builders who prefer to leave the shield untouched.
**Option B** avoids the added buffer for builders comfortable with SMD rework.
Both keep the shield's logic supply at **5 V**. Neither runs the entire shield
from 3.3 V.

## Shared layout

Use bridge A to feed **both tracks in parallel**. Locomotives have independent
DCC addresses; two locos do not require two bridge outputs. Bridge B stays
disabled, with B+ and B- unconnected. Decoder programming is out of scope.

The supply plan is USB-C PD at 15 V for the motor stage, with the MP1584 set to
5.00 V for logic and the protected Pico VSYS feed. Current sensing uses A0 through
the existing divider/filter to GP26. Follow the chosen guide for the actual
connections; neither rail is logic ground.

## Firmware and validation status

These are **two documented hardware options, not two released firmware variants**.
Firmware **0.5.3 is still a GP2-only logic-output prototype**. Option B needs the
complementary GP3 output. Both need default-off track enable, calibrated current
cutoff and startup speed-zero handling before powering a train. The assemblies
are not built or electrically validated, and there is no hardware-option setting
in the current firmware.

The existing **Revision C schematic, BOM, connections CSV and KiCad exports cover
Option A only**. Do not combine its buffer wiring with Option B's direct wiring.
Both guides remain available on the main branch so builders can choose without
switching branches.

[Manufacturer references](sources.md) · [Schematic SVG, Option A](schematic.svg) ·
[Offline print page, Option A](print.html)
