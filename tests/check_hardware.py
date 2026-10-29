#!/usr/bin/env python3
"""Check actual KiCad-exported connectivity and freshness for the simple circuit."""
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
folder=root/'hardware/kicad'
validation=json.loads((folder/'validation.json').read_text())
for name,digest in validation['sha256'].items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest, f'Stale hardware validation: {name}'
erc=json.loads((folder/'erc.json').read_text())
assert all(not s.get('violations') for s in erc['sheets']), 'Native ERC reported violations'
tree=ET.parse(folder/'netlist.xml')
pins={}
for net in tree.findall('./nets/net'):
    for node in net.findall('node'): pins[node.get('ref'),node.get('pin')]=net.get('name')
def net(ref,pin): return pins[ref,str(pin)]
for pin in [1,4,7,9,12]: assert net('U1',pin)=='GND'
for pin in [10,13,14]: assert net('U1',pin)=='+5V_LOGIC'
for pin in [8,11]: assert net('U1',pin).startswith('unconnected-')
assert net('M1',4)==net('U1',2)==net('R1',1)=='DCC_3V3'
assert net('U1',3)==net('M2','D12')=='DCC_5V'
assert net('M1',9)==net('U1',5)==net('R2',1)=='ENABLE_3V3'
assert net('U1',6)==net('M2','D3')==net('R3',1)=='ENABLE_5V'
for ref in ['R1','R2','R3','R5']: assert net(ref,2)=='GND'
for pin in ['D9','D8','D11','D13']: assert net('M2',pin)=='GND'
for pin in ['A1','B+','B-']: assert net('M2',pin).startswith('unconnected-')
assert net('M2','A+')==net('J1',1)==net('J2',1)=='RAIL_PLUS'
assert net('M2','A-')==net('J1',2)==net('J2',2)=='RAIL_MINUS'
assert net('M2','A+') not in ['GND','RAIL_MINUS']
assert net('M2','A0')==net('R4',1)=='SENSE_RAW'
assert net('R4',2)==net('R5',1)==net('C6',1)==net('M1',31)=='SENSE_ADC'
assert net('C6',2)=='GND'
assert net('D1',2)==net('M2','5V')==net('M4',3)=='+5V_LOGIC'
assert net('D1',1)==net('M1',39)=='PICO_VSYS'
for pin in [36,40]: assert net('M1',pin).startswith('unconnected-')
assert net('S1',1)==net('F1',2)==net('M4',1)=='+15V_FUSED'
assert net('S1',2)==net('M2','VIN')==net('C1',1)=='TRACK_VM'
values={c.get('ref'):c.findtext('value') for c in tree.findall('./components/comp')}
assert len(values)==21 and values['U1']=='SN74AHCT125N'
assert values['R4']==values['R5']=='10k 1%'
assert not any(ref in values for ref in ['U2','U3','U4','U5','Q1','Q2','S2'])
print('PASS: 21-part simple circuit, native ERC clean, two tracks on A, B disabled, AHCT ties, enable pull-downs, ADC divider, diode polarity and switched motor supply. No hardware performance is implied.')
