#!/usr/bin/env python3
"""Decode the stationary control stream and startup stop sequence."""
from native_support import library_sources
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='anden-track-') as folder:
    p=Path(folder);source=p/'test.cpp';exe=p/'test'
    source.write_text(r'''
#include <cassert>
#include <vector>
#include "UnoDccStream.h"
using Bytes=std::vector<unsigned>;
bool bit(UnoDccStream& s){unsigned a=s.nextHalfTicks(),b=s.nextHalfTicks();assert(a==b);assert(a==116||a==200);return a==116;}
Bytes packet(UnoDccStream& s){
 for(unsigned i=0;i<16;++i)assert(bit(s));assert(!bit(s));Bytes bytes;
 do{unsigned byte=0;for(unsigned i=0;i<8;++i)byte=byte*2+bit(s);bytes.push_back(byte);assert(bytes.size()<=4);}while(!bit(s));
 unsigned sum=0;for(auto b:bytes)sum^=b;assert(!sum);return bytes;
}
int main(){
 uint8_t a[]={3,4},speeds[]={0,0};UnoDccStream s;
 assert(s.prepareControl(2,a,1,1,speeds));s.commit();
 assert((packet(s)==Bytes{3,0x90,0x93}));
 assert((packet(s)==Bytes{3,0x3F,0x80,0xBC}));
 assert((packet(s)==Bytes{4,0x80,0x84}));
 assert((packet(s)==Bytes{4,0x3F,0x00,0x3B}));
 assert(s.banks[s.active].signature==UnoDccStream::controlSignature(2,a,1,1,speeds));
 s.startStopped();for(unsigned i=0;i<32;++i)assert((packet(s)==Bytes{0,0x41,0x41}));
 assert((packet(s)==Bytes{3,0x90,0x93}));
 // Publish during every phase of a four-byte packet; never mix halves or bytes.
 for(unsigned phase=0;phase<106;++phase){
  UnoDccStream t;assert(t.prepareControl(2,a,0,3,speeds));t.commit();packet(t);
  std::vector<unsigned> halves;
  for(unsigned i=0;i<106*10;++i){if(i==phase){assert(t.prepareControl(2,a,3,0,speeds));t.commit();}halves.push_back(t.nextHalfTicks());}
  unsigned position=0,decoded=0;
  while(position+106<halves.size()){
   auto read=[&](){unsigned x=halves[position++];assert(x==halves[position++]);return x==116;};
   for(unsigned i=0;i<16;++i)assert(read());assert(!read());Bytes b;
   do{unsigned v=0;for(unsigned i=0;i<8;++i)v=v*2+read();b.push_back(v);assert(b.size()<=4);}while(!read());
   unsigned sum=0;for(auto v:b)sum^=v;assert(!sum);assert(b[0]==3||b[0]==4);
   assert(b.size()==3?(b[1]==0x80||b[1]==0x90):(b[1]==0x3F&&(b[2]==0||b[2]==0x80)));++decoded;
  }assert(decoded>=8);
 }
 UnoDccStream empty;assert(empty.prepareControl(0,a,0,0,speeds));empty.commit();assert((packet(empty)==Bytes{0,0x41,0x41}));
 // Range generation uses no 127-packet SRAM table; verify every address and split.
 for(unsigned split=1;split<=127;++split){
  UnoDccStream f;assert(f.prepareFinder(1,127,1,split));f.commit();
  for(unsigned cycle=0;cycle<2;++cycle)for(unsigned a=1;a<=127;++a){auto p=packet(f);assert(p.size()==3&&p[0]==a&&p[1]==(a<=split?0x90:0x80));auto stop=packet(f);assert(stop.size()==4&&stop[0]==a&&stop[1]==0x3F&&stop[2]==0x80);}
 }
 UnoDccStream clear;assert(clear.prepareFinder(37,37,0,0));clear.commit();assert((packet(clear)==Bytes{37,0x80,0xA5}));
 assert(!clear.prepareFinder(0,100,1,50));assert(!clear.prepareFinder(1,128,1,50));assert(!clear.prepareFinder(3,40,1,20));
 UnoDccStream invalid;uint8_t bad[]={1,0};assert(!invalid.prepareControl(2,a,0,3,bad));assert(!invalid.prepareControl(2,a,0,4,speeds));
}
''')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-I',str(root/'libraries/AndenStackLink/src'),str(source),*library_sources(),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: 128-step zero-speed/direction packets, XOR, F0 refresh, 32 startup broadcast stops, empty-roster stop, all 106 publication phases and all 127 finder splits.')
