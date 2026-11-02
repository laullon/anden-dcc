#pragma once
#include <stdint.h>
#include <string.h>
#include "AndenStackLink.h"
#include "DccPacket.h"

// Main code prepares the inactive bank only while no publication is pending.
// The ISR takes a pending bank only at a complete packet boundary.
class UnoDccStream {
 public:
  static constexpr uint8_t CAPACITY=8, BITS=44;
  static constexpr uint16_t ONE=116,ZERO=200; // Timer1 /8, 0.5 us per tick.
  struct Bank {uint8_t packets[CAPACITY][6],addresses[CAPACITY],count,mask;uint16_t signature;};
  Bank banks[2]={};
  volatile uint8_t active=0,pending=255;
  volatile uint32_t packets=0;
  static uint16_t signature(uint8_t count,const uint8_t* addresses,uint8_t mask) {
    char bytes[10];bytes[0]=count;bytes[1]=mask;
    for(uint8_t i=0;i<count;++i)bytes[i+2]=addresses[i];
    return AndenLink::crc16(bytes,count+2);
  }
  static bool packetBit(uint8_t position,uint8_t address,uint8_t instruction,uint8_t checksum) {
    if(position<16 || position==43)return true;
    if(position==16 || position==25 || position==34)return false;
    if(position<25)return address & (0x80 >> (position-17));
    if(position<34)return instruction & (0x80 >> (position-26));
    return checksum & (0x80 >> (position-35));
  }
  // Call commit() with interrupts disabled after prepare() succeeds.
  bool prepare(uint8_t count,const uint8_t* addresses,uint8_t mask) {
    if(count>CAPACITY || pending!=255 || (count<8 && (mask>>count)))return false;
    for(uint8_t i=0;i<count;++i){
      if(!addresses[i] || addresses[i]>127)return false;
      for(uint8_t j=0;j<i;++j)if(addresses[j]==addresses[i])return false;
    }
    uint8_t bank=active^1;
    Bank& b=banks[bank];memset(&b,0,sizeof(b));b.count=count;b.mask=mask;
    b.signature=signature(count,addresses,mask);
    for(uint8_t i=0;i<(count?count:1);++i){
      uint8_t a=count?addresses[i]:0xFF;
      uint8_t instruction=count?DccPacket::instruction(mask&(1<<i)):0;
      uint8_t sum=a^instruction;
      b.addresses[i]=count?a:0;
      for(uint8_t p=0;p<BITS;++p)if(packetBit(p,a,instruction,sum))b.packets[i][p>>3]|=0x80>>(p&7);
    }
    return true;
  }
  void commit(){pending=active^1;}
  uint16_t nextHalfTicks() {
    if(!second){
      if(!position && pending!=255){active=pending;pending=255;slot=0;}
      duration=(banks[active].packets[slot][position>>3] & (0x80>>(position&7)))?ONE:ZERO;
      second=true;return duration;
    }
    second=false;
    if(++position==BITS){position=0;++packets;if(++slot>=(banks[active].count?banks[active].count:1))slot=0;}
    return duration;
  }
 private:
  uint8_t position=0,slot=0;
  uint16_t duration=ONE;
  bool second=false;
};
