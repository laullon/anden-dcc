#!/usr/bin/env python3
"""Build the offline print page from the checked-in, KiCad-exported SVG."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
folder=root/'docs/hardware'
assert (folder/'schematic.svg').exists()
html='''<!doctype html><html lang="en"><meta charset="utf-8">
<title>anden-dcc · simple wiring · Rev C proposal</title>
<style>body{margin:0;background:#e8e8e1}.sheet{max-width:1600px;margin:20px auto;background:white}.sheet img{display:block;width:100%;height:auto}@media print{@page{size:A3 landscape;margin:5mm}body{background:white}.sheet{margin:0;width:410mm;height:287mm}.sheet img{width:100%;height:100%}}</style><body><article class="sheet">'''+'<img src="schematic.svg" alt="anden-dcc simple circuit - Rev C proposal"></article></body></html>'
(folder/'print.html').write_text(html)
print('Generated the single-sheet Rev C print page from schematic.svg.')
