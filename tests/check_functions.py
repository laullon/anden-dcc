#!/usr/bin/env python3
"""Decode every F0–F28 refresh group and validate named-favorite records."""
from native_support import library_sources
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='anden-functions-') as d:
 p=Path(d);source=p/'test.cpp';exe=p/'test'
 source.write_text(r'''
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include "UnoDccStream.h"
#include "AndenStackLink.h"
#include "LocoRoster.h"
#include "FavoriteFunctions.h"
using Bytes=std::vector<unsigned>;
bool bit(UnoDccStream& s){auto a=s.nextHalfTicks(),b=s.nextHalfTicks();assert(a==b&&(a==116||a==200));return a==116;}
Bytes packet(UnoDccStream& s){for(unsigned i=0;i<16;++i)assert(bit(s));assert(!bit(s));Bytes b;do{unsigned n=0;for(unsigned i=0;i<8;++i)n=n*2+bit(s);b.push_back(n);assert(b.size()<=4);}while(!bit(s));unsigned sum=0;for(auto n:b)sum^=n;assert(!sum);return b;}
uint32_t decode(const Bytes& b){
 if((b[1]&0xE0)==0x80)return ((b[1]&0x10)?1u:0u)|((b[1]&15)<<1);
 if((b[1]&0xF0)==0xB0)return uint32_t(b[1]&15)<<5;
 if((b[1]&0xF0)==0xA0)return uint32_t(b[1]&15)<<9;
 if(b[1]==0xDE)return uint32_t(b[2])<<13;
 if(b[1]==0xDF)return uint32_t(b[2])<<21;
 assert(b[1]==0x3F&&b.size()==4&&(b[2]==0||b[2]==0x80));return 0;
}
int main(){
 uint8_t addresses[]={3,6,7,8,9,10,126,127};uint32_t functions[8]={};
 for(unsigned fn=0;fn<=28;++fn){
  functions[0]=uint32_t(1)<<fn;functions[1]=DccFunctions::MASK^functions[0];UnoDccStream s;
  assert(s.prepareFunctions(2,addresses,1,functions));s.commit();uint32_t got[2]={};
  for(unsigned i=0;i<24;++i){auto b=packet(s);unsigned loco=(i/6)%2;assert(b[0]==addresses[loco]);got[loco]|=decode(b);if(i%6==1)assert(b[2]==(loco?0:0x80));}
  assert(got[0]==functions[0]&&got[1]==functions[1]);assert(s.banks[s.active].signature==UnoDccStream::functionSignature(2,addresses,1,functions));
 }
 for(unsigned i=0;i<8;++i)functions[i]=(i%2)?DccFunctions::MASK:0;
 UnoDccStream full;assert(full.prepareFunctions(8,addresses,0xA5,functions));full.commit();
 for(unsigned i=0;i<48;++i){auto b=packet(full);assert(b[0]==addresses[i/6]);decode(b);}
 // Changing a snapshot at any half-bit phase cannot tear a packet.
 for(unsigned phase=0;phase<106;++phase){
  UnoDccStream s;uint32_t off[2]={},on[2]={DccFunctions::MASK,DccFunctions::MASK};assert(s.prepareFunctions(2,addresses,1,off));s.commit();packet(s);
  std::vector<unsigned> halves;
  for(unsigned i=0;i<1400;++i){if(i==phase){assert(s.prepareFunctions(2,addresses,2,on));s.commit();}halves.push_back(s.nextHalfTicks());}
  unsigned pos=0;
  while(pos+106<halves.size()){
   auto read=[&](){auto a=halves[pos++];assert(a==halves[pos++]);return a==116;};
   for(unsigned i=0;i<16;++i)assert(read());assert(!read());Bytes b;
   do{unsigned n=0;for(unsigned i=0;i<8;++i)n=n*2+read();b.push_back(n);assert(b.size()<=4);}while(!read());
   unsigned sum=0;for(auto n:b)sum^=n;assert(!sum);assert(b[0]==3||b[0]==6);decode(b);
  }
 }
 UnoDccStream startup;assert(startup.prepareFunctions(8,addresses,255,functions));startup.commit();startup.startStopped();
 for(unsigned i=0;i<32;++i)assert((packet(startup)==Bytes{0,0x41,0x41}));assert(packet(startup)[0]==3);
 UnoDccStream invalid;functions[0]=uint32_t(1)<<29;assert(!invalid.prepareFunctions(1,addresses,1,functions));
 uint32_t parsed;assert(AndenLink::number32("536870911",parsed,DccFunctions::MASK)&&parsed==DccFunctions::MASK);assert(!AndenLink::number32("536870912",parsed,DccFunctions::MASK));assert(!AndenLink::number32("4294967296",parsed));assert(!AndenLink::number32("-1",parsed));
 std::string command="AD1,65535,CONTROLX,8,255";for(unsigned i=0;i<8;++i)command+=",127,536870911";command+=",255";
 char wire[AndenLink::MAX_BODY+8];auto size=AndenLink::encode(wire,sizeof(wire),command.c_str());assert(size);AndenLink::Receiver rx;bool received=false;for(unsigned i=0;i<size;++i)received|=rx.feed(wire[i]);assert(received);
 LocoRoster roster;assert(roster.save(0,3,"Test")==LocoRoster::Result::Ok);auto& loco=roster.entries[0];LocoRoster::setFunction(loco,28,true);LocoRoster::setFunction(loco,0,true);assert(LocoRoster::functionMask(loco)==((uint32_t(1)<<28)|1));LocoRoster::setFunction(loco,0,false);assert(LocoRoster::functionMask(loco)==(uint32_t(1)<<28));assert(!LocoRoster::setFunction(loco,29,true));roster.save(loco.id,4,"Test");assert(!LocoRoster::functionMask(loco));
 FavoriteFunctions favorites;assert(favorites.save(1,2,"Horn"));assert(favorites.save(1,2,"Air horn"));assert(favorites.locos[0].count==1);assert(!std::strcmp(favorites.locos[0].entries[0].name,"Air horn"));
 for(unsigned i=3;i<=9;++i)assert(favorites.save(1,i,"Feature"));assert(!favorites.save(1,10,"Full"));assert(favorites.remove(1,2));assert(favorites.save(1,28,"Smoke"));assert(favorites.save(2,0,"Headlights"));favorites.clear(1);assert(!favorites.locos[0].count&&favorites.locos[1].count==1);
 assert(!favorites.save(0,2,"Bad"));assert(!favorites.save(1,29,"Bad"));assert(!favorites.save(1,1,""));assert(!favorites.save(1,1,"Tab\tname"));assert(!favorites.save(1,1,std::string(25,'a').c_str()));
 LocoRoster presence;
 assert(presence.loadRecord("1\t3\telectric\tOld format",false));
 assert(presence.loadRecord("2\t6\tdiesel\t1\tPresent",false,true));
 assert(!presence.loadRecord("3\t7\tdiesel\t2\tInvalid",false,true));
 assert(presence.count==2 && presence.dioramaMask()==2);
 assert(presence.save(2,6,"Renamed")==LocoRoster::Result::Ok && presence.dioramaMask()==2);
 assert(presence.remove(1) && presence.dioramaMask()==1);
 FavoriteFunctions loaded;assert(loaded.loadRecord("1\t2\tAir horn"));assert(loaded.loadRecord("2\t28\tSmoke"));assert(!loaded.loadRecord("1\t2\tDuplicate"));assert(!loaded.loadRecord("9\t2\tBad"));assert(!loaded.loadRecord("1\t29\tBad"));assert(!loaded.loadRecord("1\t1\tLine\nname"));
}
''')
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-I',str(root/'libraries/AndenStackLink/src'),'-I',str(root/'firmware/esp13-ota'),str(source),str(root/'firmware/esp13-ota/FavoriteFunctions.cpp'),*library_sources(),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS: F0-F28 group decoding/XOR/independence, 8-loco refresh, all publication phases, startup stops, full UART snapshot and favorite validation/limits/records.')
