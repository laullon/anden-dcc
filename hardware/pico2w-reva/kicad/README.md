# EXPERIMENT · Pico 2 W KiCad project

**EXPERIMENTAL / DESIGN ONLY — not for fabrication.**

Open `anden-dcc-pico2w-reva.kicad_pro` in KiCad 10. The project contains a
root overview and six hierarchical sheets, a two-layer PCB with a nominal
1.6 mm thickness, and project-local symbol/footprint library tables.

## Schematic organization

| Sheet | Scope |
| --- | --- |
| 01-usb-pd | USB-C connector, PD negotiation, input protection |
| 02-logic-power | 5 V buck, source isolation, Pico USB coexistence |
| 03-pico | Pico 2 W sockets, GPIO, debug and antenna constraints |
| 04-track-output | DCC bridge, support network, parallel track connectors |
| 05-protection | Watchdog, enable gating, current and voltage sensing |
| 06-cab | Encoder, selection button, STOP, LED and OLED |

Sheet 01 now contains the [PD negotiation core for review](review/01a-usb-pd.md).
The remaining sheets contain design notes and outstanding tasks. Input protection,
inter-sheet electrical interfaces and footprints have not been captured.
The local libraries are intentionally empty until pin assignments and land
patterns are checked against manufacturer drawings. The PCB contains a status
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
user lays out the PCB, and the assistant reviews it. Review 01A covers the PD
negotiation core; 01B will finish input protection and the load path before
moving to the logic supply. The board was not modified during capture of 01A.

Read [PROJECT.md](../PROJECT.md) for requirements, candidate parts, assumptions
and open work. Review [01A](review/01a-usb-pd.md) before completing the rest of the USB-C input.
