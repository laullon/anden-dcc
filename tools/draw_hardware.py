#!/usr/bin/env python3
"""Generate the project's editable SVG wiring sheets. No external packages."""
from pathlib import Path
from html import escape
R=Path(__file__).resolve().parents[1]
OUT=R/'docs/hardware'
INK='#142d32';MINT='#437d65';ORANGE='#b56a28';BLUE='#3b6f9d';RED='#a34030';GRAY='#647676'
class Sheet:
 def __init__(self,number,title,subtitle):
  self.s=['<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="850" viewBox="0 0 1200 850">', '<rect width="1200" height="850" fill="#fffef9"/>','<style>text{font-family:Arial,sans-serif;fill:#142d32} .mono{font-family:monospace}</style>']
  self.text(42,40,'anden-dcc / HARDWARE',14,weight='bold');self.text(1158,40,f'{number} / REV A',13,anchor='end')
  self.text(42,86,title,29,weight='bold');self.text(42,116,subtitle,13,color=GRAY)
  self.line(42,139,1158,139,'#d7dfd6',1)
  self.rect(42,768,1116,47,8,'#f8e9da');self.text(58,790,'PROPOSED POWER ASSEMBLY · NOT BUILT OR ELECTRICALLY VALIDATED',12,color=RED,weight='bold');self.text(58,807,'Firmware 0.4.0 drives GP2 only. Other pins are reservations. See hardware README and sources before assembly.',11)
 def text(self,x,y,t,size=13,color=INK,weight='normal',anchor='start'):
  self.s.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" style="fill:{color}" font-weight="{weight}" text-anchor="{anchor}">{escape(str(t))}</text>')
 def line(self,x,y,a,b,color=INK,width=2,dash=None):
  self.s.append(f'<path d="M{x} {y}H{a}V{b}" fill="none" stroke="{color}" stroke-width="{width}"'+(f' stroke-dasharray="{dash}"' if dash else '')+'/>')
 def path(self,d,color=INK,width=2):self.s.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}"/>')
 def rect(self,x,y,w,h,rx=10,fill='#f4f2eb',stroke=None):self.s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke or "#d7dfd6"}"/>')
 def box(self,x,y,w,h,title,lines=()):
  self.rect(x,y,w,h);self.text(x+15,y+27,title,15,weight='bold')
  for i,t in enumerate(lines):self.text(x+15,y+51+20*i,t,12)
 def dot(self,x,y,c=INK):self.s.append(f'<circle cx="{x}" cy="{y}" r="3.5" fill="{c}"/>')
 def ground(self,x,y):self.path(f'M{x} {y}v9m-12 0h24m-18 5h12m-9 5h6',BLUE,1.8)
 def resistor(self,x,y,label,vertical=False):
  if vertical:self.line(x,y,x,y+10);self.rect(x-5,y+10,10,28,0,'#fffef9',INK);self.line(x,y+38,x,y+48);self.text(x+13,y+28,label,11)
  else:self.line(x,y,x+10,y);self.rect(x+10,y-5,38,10,0,'#fffef9',INK);self.line(x+48,y,x+58,y);self.text(x+29,y-12,label,11,anchor='middle')
 def save(self,name):self.s.append('</svg>');(OUT/name).write_text('\n'.join(self.s)+'\n')

s=Sheet('01','One cable. Two voltage rails.','USB-C PD power distribution · star ground · motor power disconnected by emergency stop')
s.box(50,192,192,117,'M3 · USB-C PD trigger',['15 V profile / ≥3 A','Select 0 / 1 / 1 only if','the delivered board agrees.'])
s.text(50,174,'USB-C charger: explicit 15 V / 3 A profile',12)
s.line(242,247,285,247,ORANGE,3);s.resistor(285,247,'F1 · 3 A fuse');s.line(343,247,402,247,ORANGE,3);s.dot(402,247,ORANGE);s.text(383,225,'+15V_FUSED',12,color=ORANGE)
s.path('M402 247V412',ORANGE,3)
s.box(463,189,202,115,'S1 · Emergency stop',['NC contact opens track','supply; NO contact grounds','latch /CLR on sheet 03.'])
s.line(402,247,463,247,ORANGE,3);s.line(665,247,737,247,ORANGE,3)
s.box(737,192,193,112,'M2 · Deek-Robot',['VIN motor screw terminal','NOT the 5V logic header','A/B → isolated rail pairs'])
s.line(930,247,984,247,ORANGE,3);s.text(990,242,'Track A / B',13);s.text(990,264,'bridge outputs',11,color=GRAY)
s.dot(702,247,ORANGE);s.path('M702 247V312',ORANGE);s.path('M691 312h22m-22 7h22M702 319v19',ORANGE);s.ground(702,338);s.text(728,322,'C1',11);s.text(728,339,'100 µF / 25 V',11)
s.box(318,412,230,120,'M4 · MP1584 module',['Adjust to 5.00 V first','IN+ = +15V_FUSED','IN− / OUT− = GND'])
s.line(402,412,402,412,ORANGE,3);s.line(548,470,618,470,MINT,3);s.dot(618,470,MINT);s.text(560,451,'+5V_LOGIC',12,color=MINT)
s.box(725,390,235,105,'Shield + external logic',['Shield 5V header','U1/U2/U3/U4 VCC','100 nF at each IC supply'])
s.path('M618 470V441H725',MINT,3)
s.path('M618 470V582H700',MINT,3);s.text(629,565,'D1 · 1N5819',12);s.path('M700 568L724 582L700 596ZM726 568V596',MINT,2)
s.line(726,582,810,582,MINT,3);s.text(753,565,'cathode →',11,color=MINT)
s.box(810,535,299,111,'M1 · Pico W',['VSYS = physical pin 39','GND = pin 3 or other GND','Leave VBUS and 3V3 isolated.'])
s.dot(685,441,MINT);s.path('M685 441V479',MINT);s.path('M674 479h22m-22 7h22M685 486v17',MINT);s.ground(685,503);s.text(708,520,'C2 · 47 µF / 10 V',11)
s.line(70,690,1110,690,BLUE,3);s.text(70,718,'GND: PD−, buck IN−/OUT−, shield GND, Pico GND and all logic GND.',13,color=BLUE)
s.text(800,355,'DCC rails are bipolar.',12,color=GRAY);s.text(800,375,'Allow for L298 voltage loss.',12,color=GRAY)
s.text(50,746,'D1 allows Pico USB recovery without backfeeding the external 5 V rail. No Arduino is used.',12)
s.save('power.svg')

s=Sheet('02','Control and current sensing.','5 V TTL-compatible translation · bridge enable defaults low · no rail connected to logic ground')
s.box(50,176,265,276,'M1 · Pico W',['3.3 V GPIO; common GND','GP2 active; other outputs reserved'])
s.box(456,176,265,276,'U1 · SN74AHCT125N',['5 V supply: pin 14; GND: pin 7','OE 1/4/10/13 = BUFFER_DISABLE','10 kΩ pull-down at each input'])
s.box(864,176,287,276,'M2 · Deek-Robot L298P',['5 V logic supply / common GND','10 kΩ pull-down at ENA and ENB'])
for i,(a,yout) in enumerate([(2,3),(5,6),(9,8),(12,11)]):
 y=294+i*36
 s.text(68,y+4,['GP2 / pin 4 · DCC-A','GP3 / pin 5 · DCC-B','GP6 / pin 9 · EN-A','GP7 / pin 10 · EN-B'][i],12);s.text(480,y+4,f'Channel {i+1}: {a} → {yout}',12);s.text(880,y+4,['D12 · DIRA','D13 · DIRB','D3 · PWMA / ENA','D11 · PWMB / ENB'][i],12);s.line(315,y,456,y,MINT);s.text(386,y-8,f'in {a}',11,anchor='middle')
 s.line(721,y,864,y,ORANGE);s.text(790,y-8,f'out {yout}',11,anchor='middle')
s.text(65,438,'GP4 ARM, GP5 DISARM → sheet 03',11)
s.text(879,438,'D9 BRAKEA + D8 BRAKEB → GND',11)
s.text(50,479,'ENA and ENB: 10 kΩ to GND at shield inputs. U1 OE = BUFFER_DISABLE from U2 /Q on sheet 03.',12)
s.text(50,505,'U1 high-impedance outputs must never leave the bridge enables high or floating.',12,color=RED)
for i,(label,adc,pin) in enumerate([('A0 / SNSA','GP26 / ADC0',31),('A1 / SNSB','GP27 / ADC1',32)]):
 y=540+i*100
 s.text(50,y+5,f'Shield {label}',13)
 s.line(183,y,255,y,ORANGE);s.resistor(255,y,'10 kΩ');s.line(313,y,385,y,MINT);s.dot(385,y,MINT);s.resistor(385,y+10,'10 kΩ',True);s.ground(385,y+58)
 s.line(385,y,942,y,MINT);s.text(625,y-12,f'{adc}, physical {pin}',12)
 s.path(f'M550 {y}v17m-10 0h20m-20 6h20m-10 0v16',MINT);s.ground(550,y+39);s.text(570,y+31,'1 nF',11)
 s.text(948,y+4,f'→ U3 pin {2 if i==0 else 6} (−)',12)
s.text(50,734,'Divided sense: nominal 0.825 V/A. ADC ≤2.5 V for a 5 V sense output; calibrate the delivered shield.',12)
s.save('control.svg')

s=Sheet('03','Trip once. Stay off.','Proposed asynchronous current trip · explicit edge to arm · power-on reset · hardware emergency stop')
s.box(46,180,260,144,'U3 · LM393 (PDIP-8)',['VCC 8 = +5V; GND 4 = GND','Pin 2 (−) = divided SNSA','Pin 6 (−) = divided SNSB','Pins 3, 5 (+) = VREF'])
s.line(306,246,375,246,INK);s.text(318,233,'OUT 1, 7',11)
s.path('M375 246H737V290H805',INK,2.5);s.dot(526,246)
s.text(499,226,'CLEAR_N / U2 pin 1',12,weight='bold')
s.box(805,180,348,242,'U2 · 74HCT74 (PDIP-14)',['VCC 14 = +5V; GND 7 = GND','D1 pin 2 = +5V; /PRE1 pin 4 = +5V','CLK1 pin 3 = GP4 / ARM (10 kΩ down)','Q1 pin 5 unused','/Q1 pin 6 → BUFFER_DISABLE','10 kΩ OE pull-up to +5V at U1'])
s.line(990,422,990,460,MINT);s.text(804,455,'BUFFER_DISABLE → all U1 OE pins',12,color=MINT)
s.text(804,477,'Also divide 10 kΩ / 10 kΩ → GP28 / pin 34',12)
s.text(804,499,'(1 nF to GND at midpoint; feedback only)',11,color=GRAY)
s.box(46,370,254,169,'U4 · MCP120-485',['+5V / GND; RST → CLEAR_N','Open drain; verify package','bondout before wiring.','Reset at power-up/brownout.','S1 NO: CLEAR_N → GND'])
s.path('M300 406H375V246',INK);s.dot(375,246)
s.path('M526 246V348',INK);s.resistor(526,348,'4.7 kΩ',True);s.text(523,420,'to +5V',11,anchor='middle')
s.path('M660 246V416',INK);s.box(480,456,260,105,'Q1 · 2N3904 disarm',['Collector = CLEAR_N; emitter = GND','GP5 → 10 kΩ → base','100 kΩ base to emitter'])
s.path('M660 416V456',INK);s.dot(660,246)
s.box(46,590,328,130,'VREF · nominal 0.996 V',['+5V → 40.2 kΩ → VREF → 10 kΩ → GND','100 nF from VREF to GND','At 0.825 V/A: trip ≈1.21 A/channel','Calibrate actual sense gain and threshold.'])
s.box(438,602,336,118,'S1 · two linked contacts',['NC: disconnect +15V track supply (sheet 01)','NO: ground CLEAR_N when pressed','Rated for ≥30 V DC / 3 A; verify contacts'])
s.box(816,552,337,168,'Latch behavior',['Any trip drives CLEAR_N low → Q = 0.','/Q = 1 disables U1; EN pull-downs stop bridge.','Current going away does not re-arm.','A new GP4 rising edge is required.','Unused FF2: D/CLK GND; /PRE,/CLR +5V.'])
s.text(46,745,'This is not a measured short-circuit response. Validate trip delay, overshoot, thermal behavior and all fault paths.',12,color=RED)
s.save('protection.svg')
print('Generated power.svg, control.svg, protection.svg')

# Offline, vector-preserving print pack. These are the exact generated sheets.
printable = '<!doctype html><html lang="en"><meta charset="utf-8"><title>anden-dcc wiring · Rev A proposal</title><style>body{margin:0;background:#e8e8e1}.sheet{max-width:1200px;margin:20px auto;background:white}.sheet svg{display:block;width:100%;height:auto}@media print{@page{size:A4 landscape;margin:6mm}body{background:white}.sheet{margin:0;width:285mm;height:198mm;break-after:page}.sheet:last-child{break-after:auto}.sheet svg{width:100%;height:100%}}</style><body>'
for sheet in ['power.svg','control.svg','protection.svg']:
    printable += '<article class="sheet">' + (OUT/sheet).read_text() + '</article>'
printable += '</body></html>'
(OUT/'print.html').write_text(printable)
