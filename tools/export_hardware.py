#!/usr/bin/env python3
"""Export the current KiCad schematic; never overwrite edits made in KiCad."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET

root=Path(__file__).resolve().parents[1]
folder=root/'hardware/kicad'
cli=os.environ.get('KICAD_CLI') or shutil.which('kicad-cli')
if not cli:
    candidate=Path('/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
    if candidate.exists(): cli=str(candidate)
if not cli: raise SystemExit('Install KiCad 8 or set KICAD_CLI.')
source=folder/'anden-dcc.kicad_sch'
with tempfile.TemporaryDirectory() as directory:
    tmp=Path(directory)
    def run(*args):subprocess.run([cli,'sch',*map(str,args),str(source)],check=True)
    run('erc','--format','json','--exit-code-violations','-o',tmp/'erc.json')
    run('export','svg','-o',tmp)
    run('export','pdf','-o',root/'docs/hardware/schematic.pdf')
    run('export','netlist','--format','kicadxml','-o',tmp/'netlist.xml')
    svg=(tmp/'anden-dcc.svg').read_text()
    svg=re.sub(r'<title>.*?</title>','<title>anden-dcc simple schematic - Rev C proposal</title>',svg,flags=re.S)
    (root/'docs/hardware/schematic.svg').write_text('\n'.join(line.rstrip() for line in svg.splitlines())+'\n')
    tree=ET.parse(tmp/'netlist.xml');tree.find('./design/source').text='hardware/kicad/anden-dcc.kicad_sch';tree.find('./design/date').text='2026-10-29'
    tree.write(folder/'netlist.xml',encoding='utf-8',xml_declaration=True)
    erc=json.loads((tmp/'erc.json').read_text());erc['date']='2026-10-29';erc['source']='hardware/kicad/anden-dcc.kicad_sch'
    (folder/'erc.json').write_text(json.dumps(erc,indent=2)+'\n')
    files=['hardware/kicad/anden-dcc.kicad_sch','hardware/kicad/Anden.kicad_sym','hardware/kicad/netlist.xml','hardware/kicad/erc.json','docs/hardware/schematic.svg','docs/hardware/schematic.pdf']
    version=subprocess.check_output([cli,'version'],text=True).strip()
    (folder/'validation.json').write_text(json.dumps({'revision':'C','kicad':version,'erc_violations':0,'physical_validation':False,'sha256':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files}},indent=2)+'\n')
subprocess.run(['python3',str(root/'tools/draw_hardware.py')],check=True)
subprocess.run(['python3',str(root/'tests/check_hardware.py')],check=True)
