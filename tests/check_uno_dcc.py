#!/usr/bin/env python3
"""Decode complete Uno DCC streams, including every snapshot update boundary."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
for name in ['LocoRoster.h','DccPacket.h']:
    assert (root/'libraries/AndenStackLink/src'/name).read_bytes()==(root/'firmware/anden-dcc'/name).read_bytes(), 'Shared Pico model must stay identical'
with tempfile.TemporaryDirectory(prefix='anden-uno-dcc-') as temp:
    file=Path(temp)/'test.cpp';exe=Path(temp)/'test'
    file.write_text(r'''
#include <cassert>
#include <vector>
#include <array>
#include "UnoDccStream.h"
using Packet=std::array<unsigned,3>;
Packet decode(const std::vector<unsigned>& halves,size_t start){
  bool bits[44];
  for(unsigned i=0;i<44;++i){unsigned a=halves[start+i*2],b=halves[start+i*2+1];
    assert(a==b);assert(a==116||a==200);bits[i]=a==116;}
  for(unsigned i=0;i<16;++i)assert(bits[i]);
  assert(!bits[16]&&!bits[25]&&!bits[34]&&bits[43]);
  Packet p{};for(unsigned byte=0;byte<3;++byte)for(unsigned bit=0;bit<8;++bit)p[byte]=(p[byte]<<1)|bits[17+byte*9+bit];
  assert((p[0]^p[1])==p[2]);return p;
}
void emit(UnoDccStream& s,std::vector<unsigned>& out,unsigned count){while(count--)out.push_back(s.nextHalfTicks());}
int main(){
  unsigned char addresses[]={3,4},changed[]={7,9};
  for(unsigned phase=0;phase<88;++phase){
    UnoDccStream s;assert(s.prepare(2,addresses,0));s.commit();std::vector<unsigned> warm;
    emit(s,warm,88);assert((decode(warm,0)==Packet{3,0x80,0x83}));
    std::vector<unsigned> stream;
    for(unsigned i=0;i<88*4;++i){if(i==phase){assert(s.prepare(2,changed,2));s.commit();}emit(s,stream,1);}
    bool seen=false;
    for(unsigned i=0;i<4;++i){Packet p=decode(stream,i*88);
      assert((p==Packet{3,0x80,0x83})||(p==Packet{4,0x80,0x84})||
             (p==Packet{7,0x80,0x87})||(p==Packet{9,0x90,0x99}));
      if(p[0]==7||p[0]==9)seen=true;
    }assert(seen);
  }
  UnoDccStream idle;assert(idle.prepare(0,addresses,0));idle.commit();std::vector<unsigned> waveform;
  emit(idle,waveform,88);assert((decode(waveform,0)==Packet{255,0,255}));
  UnoDccStream full;unsigned char many[]={1,2,3,4,5,6,126,127};
  assert(full.prepare(8,many,0xA5));full.commit();waveform.clear();emit(full,waveform,88*16);
  for(unsigned i=0;i<16;++i){auto p=decode(waveform,i*88);assert(p[0]==many[i%8]);assert(p[1]==((0xA5&(1<<(i%8)))?0x90:0x80));}
  UnoDccStream invalid;unsigned char duplicates[]={3,3};
  assert(!invalid.prepare(2,duplicates,0));assert(!invalid.prepare(9,many,0));assert(!invalid.prepare(2,addresses,4));
  assert(invalid.prepare(2,addresses,0));invalid.commit();assert(!invalid.prepare(2,changed,0));
}
''')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-I',str(root/'libraries/AndenStackLink/src'),str(file),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: Uno 58/100 us half-bit model, full packet decoding/XOR, all 88 publication phases, eight-loco refresh and idle packets.')
