#pragma once
#include <Arduino.h>
#include <util/atomic.h>
#include <UnoDccStream.h>
#include "BridgeBench.h"

static_assert(F_CPU==16000000UL,"Uno DCC timing requires a 16 MHz CPU");
namespace UnoDcc {
UnoDccStream stream;
BridgeGuard bridgeGuard;
volatile bool running=false;
volatile uint16_t deadlineFaults=0;
// D3 is the Uno shield's enable A. Only this atomic operation can set it HIGH.
bool armBridge(){bool armed=false;ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
  if(running){bridgeGuard.arm();PORTD|=_BV(PD3);armed=true;}
}return armed;}
void disarmBridge(){ATOMIC_BLOCK(ATOMIC_RESTORESTATE){PORTD&=~_BV(PD3);bridgeGuard.disarm();}}
void begin(){
  pinMode(10,OUTPUT);digitalWrite(10,LOW);
  uint8_t empty[1]={0};stream.prepare(0,empty,0);stream.commit();
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    TCCR1A=0;TCCR1B=0;TIMSK1=0;TCNT1=0;
    OCR1B=stream.nextHalfTicks();TIFR1=_BV(OCF1B)|_BV(TOV1);
    TCCR1A=_BV(COM1B0); // OC1B toggles in hardware at compare, normal mode.
    running=true;TIMSK1=_BV(OCIE1B);TCCR1B=_BV(CS11);
  }
}
bool set(uint8_t count,const uint8_t* addresses,uint8_t mask){
  if(!running)return false;
  uint16_t sig=UnoDccStream::signature(count,addresses,mask);
  if(stream.pending==255 && stream.banks[stream.active].count==count &&
     stream.banks[stream.active].mask==mask && stream.banks[stream.active].signature==sig &&
     !memcmp(stream.banks[stream.active].addresses,addresses,count))return true;
  if(!stream.prepare(count,addresses,mask))return false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){stream.commit();}
  uint32_t start=millis();
  while(stream.pending!=255 && uint32_t(millis()-start)<20){}
  return stream.pending==255;
}
struct Status {uint8_t count,mask;uint16_t signature,faults;uint32_t packets;bool running;};
Status status(){Status s;ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
  const auto& bank=stream.banks[stream.active];s.count=bank.count;s.mask=bank.mask;s.signature=bank.signature;
  s.faults=deadlineFaults;s.packets=stream.packets;s.running=running;
}return s;}
}
ISR(TIMER1_COMPB_vect){
  if(UnoDcc::bridgeGuard.tick())PORTD&=~_BV(PD3);
  uint16_t due=OCR1B+UnoDcc::stream.nextHalfTicks();
  if(int16_t(due-TCNT1)<=8){
    TCCR1A=0;TIMSK1=0;PORTB&=~_BV(PB2);PORTD&=~_BV(PD3);UnoDcc::running=false;++UnoDcc::deadlineFaults;
  }else OCR1B=due;
}
