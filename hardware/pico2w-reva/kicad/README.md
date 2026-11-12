# EXPERIMENT · Pico 2 W KiCad project

**EXPERIMENTAL / DESIGN ONLY — not for fabrication.**

Open `anden-dcc-pico2w-reva.kicad_pro` in KiCad 10. The project contains a
root overview and seven hierarchical sheets, a two-layer PCB with a nominal
1.6 mm thickness, and project-local symbol/footprint library tables.

## Schematic organization

| Sheet | Scope |
| --- | --- |
| 01-usb-pd | USB-C connector, PD negotiation, input protection |
| 01b-power-entry | Proposed fuse, reverse-current diode, eFuse, voltage window and inrush |
| 02-logic-power | 5 V buck, source isolation, Pico USB coexistence |
| 03-pico | Pico 2 W sockets, GPIO, debug and antenna constraints |
| 04-track-output | DCC bridge, support network, parallel track connectors |
| 05-protection | Watchdog, enable gating, current and voltage sensing |
| 06-cab | Encoder, selection button, STOP, LED and OLED |

The [PD core](review/01a-usb-pd.md), protected input and 5 V buck
([power review](review/02-power.md)) are captured for user review. The remaining
sheets contain planning notes. Interfaces between captured sections use named
global labels with nearby explanations. Pico and bridge endpoints are pending.
The local symbol library contains LMR51420X with its pin assignment checked
against the English datasheet. Footprints remain unassigned and the local
footprint library is empty pending land-pattern review. The PCB contains a status
note on User.Drawings. Its outline, placement, routing, copper weight and
manufacturing clearances remain pending. Do not treat KiCad defaults as reviewed
power-routing constraints.

Keep native files editable and commit them together with the decision record.
Use `${KIPRJMOD}` for project-local libraries. The standard KiCad libraries remain
available through the user's global library tables. Do not add generated Gerbers
or purchasing outputs until the electrical and mechanical review is complete.

## Initial checks · 12 November 2026

- Loaded the native project in KiCad 10.0.8.
- KiCad CLI parsed and exported all seven schematic pages to SVG.
- ERC reported zero violations on the planning-only sheets. This provides no
  electrical validation because no circuit is captured yet.
- PCB DRC reported one expected violation: no Edge.Cuts outline. There are no
  footprints or routed nets yet. No exclusions were added to suppress this.

## Schematic and PCB review workflow

The assistant captures one schematic section at a time for user review. The
user lays out the PCB, and the assistant reviews it. Review 01A covers the PD core. Review 01B/02 now covers the proposed main
input branch and logic supply. Raw-input surge/CC ESD work remains open. The board was not modified during capture of 01A.

Read [PROJECT.md](../PROJECT.md) for requirements, candidate parts, assumptions
and open work. Review [01A](review/01a-usb-pd.md) and [01B/02](review/02-power.md) before accepting the power section.
