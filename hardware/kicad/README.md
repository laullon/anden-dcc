# Option A electrical schematic · Rev C

This KiCad project covers the **unmodified shield + AHCT buffer** option.
The [hardware comparison](../../docs/hardware/README.md) also preserves the
[modified-shield guide](../../docs/hardware/shield-modification.md) as Option B.

Open **anden-dcc.kicad_pro**, then the schematic, in KiCad 8 or later.
It is **one A3 sheet**, with the four modules, one complete 14-pin level shifter
and the small set of passives. Module pins are their connector labels; wire the
delivered PD/buck/shield by those labels, not by drawing position.

Matching net labels are connected. The two track connectors share RAIL_PLUS and
RAIL_MINUS from bridge A. Bridge B is disabled and has no connected rails.

The circuit is a proposal, not an electrically validated assembly. A clean ERC
checks schematic connectivity/rules; it does not prove thermal capability,
short-circuit response or actual module pinouts. Current cutoff remains future
firmware work; firmware 0.5.2 only drives GP2 logic.

After editing in KiCad:

```sh
python3 tools/export_hardware.py
```

That exports the current file, runs native ERC, refreshes the SVG/PDF/netlist
and their hashes, and checks the simple circuit's key nets. It does not
regenerate the schematic or overwrite edits made in the editor. If pin/net
assignments change, update the wiring CSV/BOM and their checks to match.

`tools/draw_kicad.py` is an optional deterministic generator for this revision.
Running it replaces the source schematic. It needs the standard KiCad symbol
libraries; `KICAD_SYMBOL_DIR` can select their directory.

`Anden.kicad_sym` contains project module/package interfaces. Stock passive,
switch, connector and power-flag graphics cached in the schematic are from
the KiCad community libraries, used under their
[CC-BY-SA 4.0 design exception](https://gitlab.com/kicad/libraries/kicad-symbols/-/blob/master/LICENSE.md).
The project's MIT license remains applicable to this design and its generated
files under that exception.
