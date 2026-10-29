#!/usr/bin/env python3
"""Generate editable KiCad 8 schematics from explicit component-to-net maps."""
import copy
import os
import json
import math
import re
import uuid
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'hardware/kicad'
OUT.mkdir(parents=True,exist_ok=True)
LIB=next((p for p in [Path(os.environ.get('KICAD_SYMBOL_DIR','/nonexistent')),Path(os.environ.get('KICAD8_SYMBOL_DIR','/nonexistent')),Path('/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols'),Path('/usr/share/kicad/symbols')] if (p/'Device.kicad_sym').exists()),None)
if LIB is None: raise SystemExit('Install KiCad 8 symbols or set KICAD_SYMBOL_DIR.')
NS=uuid.UUID('c9514391-2681-4543-a161-24229d2fa39d')
def uid(name):return str(uuid.uuid5(NS,name))
def q(s):return json.dumps(str(s))
def atom(s):return json.loads(s) if isinstance(s,str) and s.startswith('"') else s
def parse(text):
 tokens=re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+',text)
 stack=[];result=None
 for t in tokens:
  if t=='(':
   n=[]
   if stack:stack[-1].append(n)
   stack.append(n)
  elif t==')':
   n=stack.pop()
   if not stack:result=n
  else:stack[-1].append(t)
 return result
def dump(n,indent=0):
 if not isinstance(n,list):return n
 text='('
 for i,x in enumerate(n):
  if isinstance(x,list):text+='\n'+'  '*(indent+1)+dump(x,indent+1)
  else:text+=(' ' if i else '')+x
 return text+')'

def children(n,key):return [x for x in n if isinstance(x,list) and x and x[0]==key]
def child(n,key):return next(iter(children(n,key)),None)
libraries={};cache={};custom={}
def load(libid):
 if libid in custom:return copy.deepcopy(custom[libid])
 if libid in cache:return copy.deepcopy(cache[libid])
 lib,name=libid.split(':')
 if lib not in libraries:libraries[lib]=parse((LIB/(lib+'.kicad_sym')).read_text())
 source=next(n for n in children(libraries[lib],'symbol') if atom(n[1])==name)
 base=child(source,'extends')
 if base:
  inherited=load(lib+':'+atom(base[1]));old=atom(inherited[1])
  for group in children(inherited,'symbol'):
   group[1]=q(name+atom(group[1])[len(old.split(':')[-1]):])
  property_names={atom(p[1]) for p in children(source,'property')}
  inherited=[x for x in inherited if not(isinstance(x,list) and x[0]=='property' and atom(x[1]) in property_names)]
  inherited += copy.deepcopy(children(source,'property'))
  source=inherited
 else:source=copy.deepcopy(source)
 source[1]=q(name)
 cache[libid]=source
 return copy.deepcopy(source)

def pin(number,name,x,y,angle,kind='passive'):
 return parse(f'(pin {kind} line (at {x} {y} {angle})(length 2.54)(name {q(name)}(effects(font(size 1 1))))(number {q(number)}(effects(font(size 1 1)))))')
def module(name,title,width,height,pins):
 body=['symbol',q(name),['pin_names',['offset','1']],['in_bom','yes'],['on_board','yes'],
       parse(f'(property "Reference" "M" (at 0 {height/2+4} 0)(effects(font(size 1.27 1.27))))'),
       parse(f'(property "Value" {q(title)} (at 0 {height/2+2} 0)(effects(font(size 1.27 1.27))))'),
       ['symbol',q(name+'_0_1'),parse(f'(rectangle(start {-width/2} {height/2})(end {width/2} {-height/2})(stroke(width .254)(type default))(fill(type background)))')],
       ['symbol',q(name+'_1_1'),*pins]]
 custom['Anden:'+name]=body

pico={1:'GP0',2:'GP1',3:'GND',4:'GP2',5:'GP3',6:'GP4',7:'GP5',8:'GND',9:'GP6',10:'GP7',11:'GP8',12:'GP9',13:'GND',14:'GP10',15:'GP11',16:'GP12',17:'GP13',18:'GND',19:'GP14',20:'GP15',21:'GP16',22:'GP17',23:'GND',24:'GP18',25:'GP19',26:'GP20',27:'GP21',28:'GND',29:'GP22',30:'RUN',31:'GP26_ADC0',32:'GP27_ADC1',33:'AGND',34:'GP28_ADC2',35:'ADC_VREF',36:'3V3_OUT',37:'3V3_EN',38:'GND',39:'VSYS',40:'VBUS_USB'}
p=[]
for number,name in pico.items():
 left=number<=20;i=number-1 if left else 40-number
 kind='power_in' if number in [3,8,13,18,23,28,33,38,39] else 'power_out' if number==36 else 'passive'
 p.append(pin(number,name,-25.4 if left else 25.4,24.13-i*2.54,0 if left else 180,kind))
module('PicoW','Pico W · physical pins',45.72,53.34,p)
module('PD15','USB-C PD trigger / 15 V',30.48,17.78,[pin('1','OUT+',17.78,3.81,180,'power_out'),pin('2','OUT-',17.78,-3.81,180)])
module('MP1584','MP1584 module / 5.00 V',33.02,20,[pin('1','IN+',-19.05,5.08,0,'power_in'),pin('2','IN-',-19.05,-5.08,0),pin('3','OUT+',19.05,5.08,180,'power_out'),pin('4','OUT-',19.05,-5.08,180)])
pins=[]
for number,name,y in [('D12','DIR_A',30.48),('D13','DIR_B',20.32),('D3','EN_A',10.16),('D11','EN_B',0),('D9','BRAKE_A',-10.16),('D8','BRAKE_B',-20.32)]:pins.append(pin(number,name,-22.86,y,0,'input'))
for number,name,y in [('A0','SNS_A',30.48),('A1','SNS_B',20.32),('A+','DCC_A+' ,5.08),('A-','DCC_A-' ,-5.08),('B+','B+ unused',-20.32),('B-','B- unused',-30.48)]:pins.append(pin(number,name,22.86,y,180,'output' if number in ['A0','A1'] else 'passive'))
pins += [pin('5V','5V_LOGIC',-7.62,43.18,270,'power_in'),pin('VIN','VIN_MOTOR',7.62,43.18,270,'power_in'),pin('GND','GND',0,-43.18,90,'power_in')]
module('DeekRobot','Deek-Robot L298P',40.64,81.28,pins)
stop=['symbol',q('EStop'),['in_bom','yes'],['on_board','yes'],parse('(property "Reference" "S"(at 0 5.08 0)(effects(font(size 1.27 1.27))))'),parse('(property "Value" "E-STOP NC+NO"(at 0 7.62 0)(effects(font(size 1.27 1.27))))')]
for unit,nums,opened in [(1,('11','12'),False),(2,('13','14'),True)]:
 stop += [['symbol',q(f'EStop_{unit}_1'),pin(nums[0],'',-7.62,0,0),pin(nums[1],'',7.62,0,180),parse('(polyline(pts(xy -5.08 0)(xy 5.08 '+('3.81' if opened else '0')+'))(stroke(width .254)(type default))(fill(type none)))')]]
custom['Anden:EStop']=stop

module('AHCT125','SN74AHCT125N',38.1,50.8,[
 pin('2','1A / DCC',-21.59,17.78,0,'input'),pin('1','~{1OE}',-21.59,12.7,0,'input'),
 pin('5','2A / ENABLE',-21.59,7.62,0,'input'),pin('4','~{2OE}',-21.59,2.54,0,'input'),
 pin('9','3A unused',-21.59,-2.54,0,'input'),pin('10','~{3OE}',-21.59,-7.62,0,'input'),
 pin('12','4A unused',-21.59,-12.7,0,'input'),pin('13','~{4OE}',-21.59,-17.78,0,'input'),
 pin('3','1Y / DCC',21.59,17.78,180,'tri_state'),pin('6','2Y / ENABLE',21.59,7.62,180,'tri_state'),
 pin('8','3Y unused',21.59,-2.54,180,'tri_state'),pin('11','4Y unused',21.59,-12.7,180,'tri_state'),
 pin('14','VCC',21.59,-17.78,180,'power_in'),pin('7','GND',21.59,-22.86,180,'power_in')])
ROOT_ID=uid('root');PROJECT='anden-dcc'
sheets=[];components=[]
class Sheet:
 def __init__(self,key,title,page):
  self.key=key;self.title=title;self.page=page;self.id=ROOT_ID;self.hier=uid('hier:'+key);self.items=[];self.symbols={};self.instances=[];self.seq=0
  sheets.append(self)
 def ident(self,prefix):self.seq+=1;return uid(f'{self.key}/{prefix}/{self.seq}')
 def wire(self,a,b):
  self.items.append(parse(f'(wire(pts(xy {a[0]} {a[1]})(xy {b[0]} {b[1]}))(stroke(width 0)(type default))(uuid {self.ident("wire")}))'))
 def label(self,net,xy,angle=0):
  self.items.append(parse(f'(global_label {q(net)} (shape passive)(at {xy[0]} {xy[1]} {angle})(effects(font(size 1 1))(justify {"right" if angle in (180,270) else "left"}))(uuid {self.ident("label")})(property "Intersheetrefs" "${{INTERSHEET_REFS}}"(at {xy[0]} {xy[1]} 0)(effects(font(size 1 1))(hide yes))))'))
 def note(self,x,y,text,size=1.27):self.items.append(parse(f'(text {q(text)}(at {x} {y} 0)(effects(font(size {size} {size}))(justify left top))(uuid {self.ident("text")}))'))
 def add(self,libid,ref,value,x,y,nets,unit=1,angle=0,hide_value=False,hide_nets=()):
  x=round(round(x/1.27)*1.27,4);y=round(round(y/1.27)*1.27,4)
  sym=load(libid);self.symbols[libid]=sym
  ident=self.ident(f'{ref}.{unit}')
  if ref.startswith(('U','Q','S')): hide_value=True
  field_angle=angle%180
  rx,ry,vx,vy=x+2.54,y-5.08,x+2.54,y-2.54
  if ref in ['M1','M2','M3','M4','U1']:
   halfheight={'M1':26.67,'M2':40.64,'M3':8.89,'M4':10,'U1':25.4}[ref]
   rx,ry,vx,vy=x-10,y-halfheight-6,x-10,y-halfheight-3
   if ref=='U1':hide_value=False
  if ref.startswith('J'):rx,ry,vx,vy=x-5,y-8,x-20,y-5

  instance=parse(f'''(symbol(lib_id {q(libid)})(at {x} {y} {angle})(unit {unit})(in_bom yes)(on_board yes)(dnp no)(uuid {ident})
    (property "Reference" {q(ref)}(at {rx} {ry} {field_angle})(effects(font(size 1.1 1.1))(justify left){"(hide yes)" if ref.startswith("#") else ""}))
    (property "Value" {q(value)}(at {vx} {vy} {field_angle})(effects(font(size 1.1 1.1))(justify left){'(hide yes)' if hide_value else ''}))
    (instances(project {q(PROJECT)}(path {q('/'+ROOT_ID)}(reference {q(ref)})(unit {unit})))))''')
  points={};pinlist=[]
  for group in children(sym,'symbol'):
   match=re.search(r'_(\d+)_(\d+)$',atom(group[1]))
   if not match or int(match[1]) not in (0,unit):continue
   for p in children(group,'pin'):
    number=atom(child(p,'number')[1]);pos=child(p,'at');lx,ly=float(pos[1]),float(pos[2]);pa=float(pos[3])
    theta=math.radians(angle);rx=lx*math.cos(theta)-ly*math.sin(theta);ry=lx*math.sin(theta)+ly*math.cos(theta)
    point=(round(x+rx,4),round(y-ry,4));points[number]=point
    instance.append(parse(f'(pin {q(number)}(uuid {self.ident(ref+"pin"+number)}))'))
    net=nets.get(number)
    if net is None:
     self.items.append(parse(f'(no_connect(at {point[0]} {point[1]})(uuid {self.ident("nc")}))'))
    elif number not in hide_nets:
     direction=(pa+angle)%360
     dx,dy={0:(-6.35,0),90:(0,6.35),180:(6.35,0),270:(0,-6.35)}[round(direction)]
     end=(round(point[0]+dx,4),round(point[1]+dy,4))
     self.wire(point,end)
     self.label(net,end,180 if dx<0 else 0 if dx>0 else 270 if dy>0 else 90)
    pinlist.append({'pin':number,'name':atom(child(p,'name')[1]),'net':net})
  self.items.append(instance);self.instances.append(instance)
  components.append({'sheet':self.key,'reference':ref,'unit':unit,'value':value,'symbol':libid,'pins':pinlist})
  return points
 def resistor(self,ref,value,x,y,a,b,angle=0):return self.add('Device:R',ref,value,x,y,{'1':a,'2':b},angle=angle)
 def divider(self,top_ref,bottom_ref,top_value,bottom_value,x,y,net):
  x=round(round(x/1.27)*1.27,4);y=round(round(y/1.27)*1.27,4)
  a=self.add('Device:R',top_ref,top_value,x,y,{'1':'+5V_LOGIC','2':net},hide_nets=('2',))
  b=self.add('Device:R',bottom_ref,bottom_value,x,y+25.4,{'1':net,'2':'GND'},hide_nets=('1',))
  self.wire(a['2'],b['1'])
  mid=(x,round(y+12.7,4));end=(round(x+8.89,4),mid[1]);self.wire(mid,end);self.label(net,end,0)
  self.items.append(parse(f'(junction(at {mid[0]} {mid[1]})(diameter 0)(color 0 0 0 0)(uuid {self.ident("junction")}))'))
 def cap(self,ref,value,x,y,net,polar=False):return self.add('Device:C_Polarized' if polar else 'Device:C',ref,value,x,y,{'1':net,'2':'GND'})
 def save(self):
  definitions=[]
  for name,sym in sorted(self.symbols.items()):
   sym=copy.deepcopy(sym);sym[1]=q(name);definitions.append(sym)
  schematic=['kicad_sch',['version','20231120'],['generator',q('anden-dcc')],['uuid',self.id],['paper',q('A3')],
   ['title_block',['title',q(self.title)],['date',q('2026-10-29')],['rev',q('C')],['company',q('anden-dcc / laullon')],['comment','1',q('NOT BUILT OR ELECTRICALLY VALIDATED')],['comment','2',q('Firmware 0.5.2: GP2 logic only; track power disabled')]],
   ['lib_symbols',*definitions],*self.items]
  (OUT/(self.key+'.kicad_sch')).write_text(dump(schematic)+'\n')

s=Sheet('anden-dcc','anden-dcc / simple one-output wiring',1)
s.note(15,15,'anden-dcc / ONE OUTPUT, TWO TRACKS',3)
s.note(15,24,'Pico W + Deek-Robot L298P + USB-C PD + MP1584.',1.5)
s.add('Anden:PD15','M3','USB-C PD / 15 V, >=3 A',40,47,{'1':'+15V_RAW','2':'GND'})
s.add('Device:Fuse','F1','3 A / >=32 V DC',108,43,{'1':'+15V_RAW','2':'+15V_FUSED'},angle=90)
s.add('Switch:SW_SPST','S1','TRACK POWER / OFF',185,43,{'1':'+15V_FUSED','2':'TRACK_VM'})
s.cap('C1','100 uF / 25 V',270,47,'TRACK_VM',True)
s.note(305,38,'S1: manual motor-power cutoff.\nLogic stays powered when S1 is off.\nUse contacts rated >=30 V DC / 3 A.',1.2)
s.add('Anden:MP1584','M4','MP1584 / SET 5.00 V',52,100,{'1':'+15V_FUSED','2':'GND','3':'+5V_LOGIC','4':'GND'})
s.cap('C2','47 uF / >=10 V',110,95,'+5V_LOGIC',True)
s.add('Device:D_Schottky','D1','1N5819 / band to VSYS',169,95,{'1':'PICO_VSYS','2':'+5V_LOGIC'},angle=180)
s.cap('C5','100 nF / Pico VSYS',224,95,'PICO_VSYS')
s.cap('C3','100 nF / U1',270,95,'+5V_LOGIC')
s.cap('C4','100 nF / shield',310,95,'+5V_LOGIC')

map_pico={str(n):'GND' for n,name in pico.items() if name in ['GND','AGND']}
map_pico.update({'4':'DCC_3V3','9':'ENABLE_3V3','31':'SENSE_ADC','39':'PICO_VSYS'})
s.add('Anden:PicoW','M1','Raspberry Pi Pico W',76,183,map_pico)
s.add('Anden:AHCT125','U1','SN74AHCT125N',210,183,{
 '2':'DCC_3V3','3':'DCC_5V','1':'GND',
 '5':'ENABLE_3V3','6':'ENABLE_5V','4':'GND',
 '9':'GND','10':'+5V_LOGIC','8':None,'12':'GND','13':'+5V_LOGIC','11':None,
 '14':'+5V_LOGIC','7':'GND'})
s.add('Anden:DeekRobot','M2','Deek-Robot L298P / A only',338,178,{
 'D12':'DCC_5V','D3':'ENABLE_5V','D9':'GND',
 'D13':'GND','D11':'GND','D8':'GND','A0':'SENSE_RAW','A1':None,
 '5V':'+5V_LOGIC','VIN':'TRACK_VM','GND':'GND',
 'A+':'RAIL_PLUS','A-':'RAIL_MINUS','B+':None,'B-':None})
for ref,value,x,net in [('R1','10k',150,'DCC_3V3'),('R2','10k',186,'ENABLE_3V3'),('R3','10k',230,'ENABLE_5V')]:
 s.resistor(ref,value,x,238,net,'GND')
s.resistor('R4','10k 1%',170,270,'SENSE_RAW','SENSE_ADC',angle=90)
s.resistor('R5','10k 1%',218,270,'SENSE_ADC','GND')
s.cap('C6','1 nF',258,270,'SENSE_ADC')
s.add('Connector_Generic:Conn_01x02','J1','TRACK 1',386,224,{'1':'RAIL_PLUS','2':'RAIL_MINUS'})
s.add('Connector_Generic:Conn_01x02','J2','TRACK 2',386,244,{'1':'RAIL_PLUS','2':'RAIL_MINUS'})
s.note(284,242,'GP26: nominal 0.825 V/A after divider.\nCalibrate before enabling track power.',1.1)
s.note(15,259,'A+ feeds one rail of BOTH tracks.\nA- feeds their other rails. Neither rail is GND.\nEach loco is still controlled by its own DCC address.\nChannel B is unused; do not parallel A and B outputs.\nPower enable / current cutoff are not in firmware yet.',1.2)
s.note(15,130,'USB recovery: external 5 V goes through D1 to VSYS pin 39.\nLeave VBUS and 3V3_OUT isolated from the external 5 V supply.',1.1)
for x,net,ref in [(345,'GND','#FLG01'),(365,'PICO_VSYS','#FLG02'),(385,'TRACK_VM','#FLG03'),(405,'+15V_FUSED','#FLG04')]:
 s.add('power:PWR_FLAG',ref,'PWR_FLAG',x,84,{'1':net},hide_value=True)
s.save()
path=OUT/'anden-dcc.kicad_sch'
doc=parse(path.read_text());doc.append(parse('(sheet_instances(path "/"(page "1")))'));path.write_text(dump(doc)+'\n')
used={name for name in s.symbols if name.startswith('Anden:')}
(OUT/'Anden.kicad_sym').write_text(dump(['kicad_symbol_lib',['version','20231120'],['generator',q('anden-dcc')],*[v for k,v in custom.items() if k in used]])+'\n')
(OUT/'sym-lib-table').write_text('(sym_lib_table (version 7) (lib (name "Anden")(type "KiCad")(uri "${KIPRJMOD}/Anden.kicad_sym")(options "")(descr "Module pinouts and complete AHCT buffer package")))\n')
(OUT/'anden-dcc.kicad_pro').write_text(json.dumps({'meta':{'filename':'anden-dcc.kicad_pro','version':1}},indent=2)+'\n')
(OUT/'design.json').write_text(json.dumps({'revision':'C','source':'tools/draw_kicad.py','components':components},indent=2)+'\n')
print('Generated simple single-sheet KiCad schematic: one bridge, one buffer, two tracks.')
