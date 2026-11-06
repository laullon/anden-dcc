#!/usr/bin/env python3
"""Exercise the actual Uno Timer1 ISR against simulated timer/port registers."""
from native_support import library_sources
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='anden-route-') as folder:
    tmp=Path(folder);(tmp/'util').mkdir()
    (tmp/'util/atomic.h').write_text('#pragma once\n#define ATOMIC_RESTORESTATE 0\n#define ATOMIC_BLOCK(x) for(bool once=true;once;once=false)\n')
    (tmp/'Arduino.h').write_text(r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#define __AVR_ATmega328P__ 1
#define F_CPU 16000000UL
#define _BV(x) (1U<<(x))
#define ISR(name) void name()
#define OUTPUT 1
#define LOW 0
#define HIGH 1
#define PB2 2
#define PB4 4
#define PD3 3
#define COM1B0 4
#define OCF1B 2
#define TOV1 0
#define OCIE1B 2
#define CS11 1
struct Edge {uint32_t time;uint8_t value;};
extern uint32_t clockTicks;
extern std::vector<Edge> directionEdges,enableEdges;
struct Port {
 uint8_t value=0,watched=0;bool direction=false;
 operator uint8_t() const{return value;}
 Port& operator|=(unsigned mask){return assign(value|mask);}
 Port& operator&=(unsigned mask){return assign(value&mask);}
 Port& assign(uint8_t next){if((next^value)&watched)(direction?directionEdges:enableEdges).push_back({clockTicks,uint8_t(bool(next&watched))});value=next;return *this;}
};
struct Counter {
 Counter& operator=(uint16_t value){clockTicks=value;return *this;}
 operator uint16_t(){return uint16_t(++clockTicks);}
};
extern Port PORTB,PORTD;
extern uint8_t PINB,TCCR1A,TCCR1B,TIMSK1,TIFR1;
extern uint16_t OCR1B;
extern Counter TCNT1;
inline void pinMode(uint8_t,uint8_t){}
inline void digitalWrite(uint8_t pin,uint8_t level){unsigned mask=1U<<(pin==10?2:4);if(level)PORTB|=mask;else PORTB&=~mask;}
uint32_t millis();
''')
    (tmp/'test.cpp').write_text(r'''
#include <cassert>
#include <cstddef>
#include <cstdio>
#include "Arduino.h"
uint32_t clockTicks=0;
std::vector<Edge> directionEdges,enableEdges;
std::vector<uint16_t> dccHalves;
Port PORTB,PORTD;
uint8_t PINB=0,TCCR1A=0,TCCR1B=0,TIMSK1=0,TIFR1=0;
uint16_t OCR1B=0;Counter TCNT1;
#include "UnoDcc.h"
#include "BridgeBench.h"
#include <UnoDccStream.h>
void TIMER1_COMPB_vect();
uint32_t hardwareEdge=0;uint16_t lastCompare=0;
bool boundaryEnableSeen=false;
void step(unsigned delay=0){
 assert(UnoDcc::isRunning());
 uint16_t next=OCR1B;hardwareEdge+=uint16_t(next-lastCompare);lastCompare=next;
 clockTicks=hardwareEdge+delay;PINB^=_BV(PB2);
 bool wasEnabled=PORTD.value&_BV(PD3);
 TIMER1_COMPB_vect();
 if(!wasEnabled && (PORTD.value&_BV(PD3))){boundaryEnableSeen=true;dccHalves.clear();}
 dccHalves.push_back(uint16_t(OCR1B-next));
 if(UnoDcc::bridgeIsActive() && !UnoDcc::bridgeStartPending())assert(bool(PORTB.value&_BV(PB4))==bool(PINB&_BV(PB2)));
}
bool pumpTimer=false;
uint32_t millis(){if(pumpTimer)step();return clockTicks/2000;}
int main(){
 PORTB.watched=_BV(PB4);PORTB.direction=true;PORTD.watched=_BV(PD3);
 UnoDcc::begin();
 // Invalid counts must be rejected before signature buffers/addresses are read.
 uint8_t invalidAddresses[]={1,2,3,4,5,6,7,8,9};
 assert(!UnoDcc::set(9,invalidAddresses,0));assert(!UnoDcc::set(1,nullptr,0));
 digitalWrite(12,HIGH);assert(UnoDcc::armBridge(false));
 // DC mode is not touched by the waveform ISR.
 for(unsigned i=0;i<64;++i){step();assert(PORTB.value&_BV(PB4));}
 UnoDcc::disarmBridge();assert(!(PORTD.value&_BV(PD3))&&!(PORTB.value&_BV(PB4)));
 uint8_t addresses[]={3,4};pumpTimer=true;assert(UnoDcc::set(2,addresses,0));pumpTimer=false;
 directionEdges.clear();enableEdges.clear();assert(UnoDcc::armBridge(true));assert(!(PORTD.value&_BV(PD3)));
 // Vary simulated interrupt entry latency through 0..18 us; the phase wait absorbs it.
 for(unsigned i=0;i<88*12;++i)step((i*7)%37);
 assert(boundaryEnableSeen && UnoDcc::status().faults==0);
 assert(enableEdges.size()==1 && enableEdges.front().value==1);
 // Enabling must coincide with the first edge of a complete packet preamble.
 assert(directionEdges.size()>33);
 const unsigned firstDelay=directionEdges.front().time-enableEdges.front().time;
 assert(firstDelay==0 || firstDelay==UnoDccStream::ONE);
 // If the first half is already LOW, it creates no direction transition.
 const unsigned skipped=firstDelay?1:0;
 for(unsigned i=1;i<=32-skipped;++i)assert(directionEdges[i].time-directionEdges[i-1].time==UnoDccStream::ONE);
 assert(directionEdges[33-skipped].time-directionEdges[32-skipped].time==UnoDccStream::ZERO);
 unsigned ones=0,zeros=0;
 for(std::size_t i=1;i<directionEdges.size();++i){
  unsigned width=directionEdges[i].time-directionEdges[i-1].time;
  assert(width==UnoDccStream::ONE || width==UnoDccStream::ZERO);
  if(width==UnoDccStream::ONE)++ones;else ++zeros;
 }
 assert(ones&&zeros);
 UnoDcc::disarmBridge();std::size_t count=directionEdges.size();
 for(unsigned i=0;i<100;++i)step();assert(directionEdges.size()==count && !(PORTD.value&_BV(PD3)));
 // Exercise the new variable-length six-group refresh through the actual ISR.
 uint8_t allAddresses[]={3,6,7,8,9,10,126,127};uint32_t flags[8];
 for(unsigned i=0;i<8;++i)flags[i]=i%2?DccFunctions::MASK:0;
 pumpTimer=true;assert(UnoDcc::functions(8,allAddresses,0xA5,flags));pumpTimer=false;
 directionEdges.clear();assert(UnoDcc::armBridge(true));
 for(unsigned i=0;i<48*106*2;++i)step((i*7)%37);
 assert(UnoDcc::isRunning()&&UnoDcc::status().faults==0);
 for(std::size_t i=1;i<directionEdges.size();++i){auto width=directionEdges[i].time-directionEdges[i-1].time;assert(width==116||width==200);}
 UnoDcc::disarmBridge();
 // A CRC collision across schedule modes must not skip a real legacy update.
 const uint8_t oneAddress[]={3};const uint32_t collidingFunctions[]={122242};
 assert(UnoDccStream::signature(1,oneAddress,0)==UnoDccStream::functionSignature(1,oneAddress,1,collidingFunctions));
 pumpTimer=true;assert(UnoDcc::functions(1,oneAddress,1,collidingFunctions));
 assert(UnoDcc::set(1,oneAddress,0));pumpTimer=false;
 assert(UnoDcc::armBridge(true));while(UnoDcc::bridgeStartPending())step();
 while(dccHalves.size()<88*3)step();
 for(unsigned packet=0;packet<3;++packet){
   auto bit=[&](unsigned p){unsigned a=dccHalves[packet*88+p*2],b=dccHalves[packet*88+p*2+1];assert(a==b);return a==UnoDccStream::ONE;};
   for(unsigned i=0;i<16;++i)assert(bit(i));assert(!bit(16)&&!bit(25)&&!bit(34)&&bit(43));
   uint8_t bytes[3]={};for(unsigned byte=0;byte<3;++byte)for(unsigned p=0;p<8;++p)bytes[byte]=(bytes[byte]<<1)|bit(17+byte*9+p);
   assert(bytes[0]==3&&bytes[1]==0x80&&bytes[2]==0x83);
 }
 UnoDcc::disarmBridge();
 // No main-loop servicing: the actual ISR's backup must still shut off A.
 enableEdges.clear();assert(UnoDcc::armBridge(true));
 for(unsigned i=0;i<BridgeGuard::HALVES+88 && !UnoDcc::guardExpired();++i)step();
 assert(UnoDcc::guardExpired() && !(PORTD.value&_BV(PD3)) && !UnoDcc::bridgeIsActive());
 assert(enableEdges.size()==2 && enableEdges[0].value==1 && enableEdges[1].value==0);
 unsigned duration=enableEdges[1].time-enableEdges[0].time;
 assert(duration>=uint32_t(BridgeGuard::HALVES-1)*UnoDccStream::ONE && duration<=uint32_t(BridgeGuard::HALVES)*UnoDccStream::ZERO);
 // A mirror arriving outside the allowed phase must stop both output and timer.
 assert(UnoDcc::armBridge(true));while(UnoDcc::bridgeStartPending())step();
 step(UnoDcc::BRIDGE_DELAY_TICKS+3);
 assert(!UnoDcc::isRunning() && !(PORTD.value&_BV(PD3)) && !(PORTB.value&_BV(PB4)) && UnoDcc::status().faults==1);
 std::printf("PASS: actual Timer1 ISR, DC isolation, packet-boundary enable, D12 legacy/F0-F28 58/100 us phase model under ISR delays, stop, backup guard and late-edge fail-off.\n");
}
''')
    exe=tmp/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(tmp),'-I',str(root/'firmware/uno-stack-test'),'-I',str(root/'libraries/AndenStackLink/src'),str(tmp/'test.cpp'),str(root/'firmware/uno-stack-test/UnoDcc.cpp'),str(root/'firmware/uno-stack-test/BridgeBench.cpp'),*library_sources(),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
