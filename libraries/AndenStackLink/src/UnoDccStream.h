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
  struct Bank {uint8_t packets[CAPACITY*2][7],sizes[CAPACITY*2],addresses[CAPACITY],count,mask,slots,first,last,onLow,onHigh;uint16_t signature;};
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
  static bool stopBit(uint8_t position,uint8_t address){
    if(position<16 || position==52)return true;
    if(position==16 || position==25 || position==34 || position==43)return false;
    if(position<25)return address&(0x80>>(position-17));
    if(position<34)return 0x3F&(0x80>>(position-26));
    if(position<43)return 0x80&(0x80>>(position-35));
    return (address^0xBF)&(0x80>>(position-44));
  }
  // Call commit() with interrupts disabled after prepare() succeeds.
  bool prepare(uint8_t count,const uint8_t* addresses,uint8_t mask) {
    if(count>CAPACITY || pending!=255 || (count<8 && (mask>>count)))return false;
    for(uint8_t i=0;i<count;++i){
      if(!addresses[i] || addresses[i]>127)return false;
      for(uint8_t j=0;j<i;++j)if(addresses[j]==addresses[i])return false;
    }
    uint8_t bank=active^1;
    Bank& b=banks[bank];memset(&b,0,sizeof(b));b.count=count;b.mask=mask;b.slots=count?count:1;
    b.signature=signature(count,addresses,mask);
    for(uint8_t i=0;i<(count?count:1);++i){
      b.sizes[i]=BITS;
      uint8_t a=count?addresses[i]:0xFF;
      uint8_t instruction=count?DccPacket::instruction(mask&(1<<i)):0;
      uint8_t sum=a^instruction;
      b.addresses[i]=count?a:0;
      for(uint8_t p=0;p<BITS;++p)if(packetBit(p,a,instruction,sum))b.packets[i][p>>3]|=0x80>>(p&7);
    }
    return true;
  }
  static uint16_t controlSignature(uint8_t count,const uint8_t* addresses,uint8_t mask,uint8_t directions,const uint8_t* speeds){
    char bytes[27];bytes[0]=count;bytes[1]=mask;bytes[2]=directions;
    for(uint8_t i=0;i<count;++i){bytes[3+i*2]=addresses[i];bytes[4+i*2]=speeds[i];}
    return AndenLink::crc16(bytes,3+count*2);
  }
  static void encode(uint8_t* out,uint8_t& size,const uint8_t* bytes,uint8_t count){
    size=17+count*9;memset(out,0,7);
    for(uint8_t p=0;p<size;++p){
      bool bit=p<16 || p==size-1;
      if(p>=16 && p<size-1){uint8_t q=p-16;bit=q%9 && (bytes[q/9]&(0x80>>((q%9)-1)));}
      if(bit)out[p>>3]|=0x80>>(p&7);
    }
  }
  bool prepareControl(uint8_t count,const uint8_t* addresses,uint8_t mask,uint8_t directions,const uint8_t* speeds){
    if(!speeds || (count<8 && (directions>>count)))return false;
    for(uint8_t i=0;i<count && i<CAPACITY;++i)if(speeds[i]!=0)return false;
    if(!prepare(count,addresses,mask))return false;
    Bank& b=banks[active^1];b.slots=count?count*2:1;
    b.signature=controlSignature(count,addresses,mask,directions,speeds);
    if(!count){uint8_t stop[]={0,0x41,0x41};encode(b.packets[0],b.sizes[0],stop,3);}
    for(uint8_t i=0;i<count;++i){
      uint8_t f=DccPacket::instruction(mask&(1<<i));uint8_t function[]={addresses[i],f,uint8_t(addresses[i]^f)};
      uint8_t speed=uint8_t((directions&(1<<i)?0x80:0)|0);
      uint8_t motion[]={addresses[i],0x3F,speed,uint8_t(addresses[i]^0x3F^speed)};
      encode(b.packets[i*2],b.sizes[i*2],function,3);encode(b.packets[i*2+1],b.sizes[i*2+1],motion,4);
    }
    return true;
  }
  bool prepareFinder(uint8_t first,uint8_t last,uint8_t onLow,uint8_t onHigh){
    if(!first || last>127 || first>last || (onLow && (onLow<first || onHigh>last || onLow>onHigh)) || (!onLow && onHigh))return false;
    uint8_t empty[1]={0};if(!prepare(0,empty,0))return false;
    Bank& b=banks[active^1];b.first=first;b.last=last;b.onLow=onLow;b.onHigh=onHigh;b.slots=(last-first+1)*2;
    char spec[]={char(first),char(last),char(onLow),char(onHigh)};b.signature=AndenLink::crc16(spec,4);return true;
  }
  void startStopped(){startupPending=true;}
  void commit(){pending=active^1;}
  bool atPacketStart() const {return position==0 && !second;}
  uint16_t nextHalfTicks() {
    if(!second){
      if(!position && pending!=255){active=pending;pending=255;slot=0;}
      if(!position && startupPending){startupPending=false;startupRemaining=32;slot=0;}
      const Bank& b=banks[active];uint8_t address=b.first+(slot>>1);
      uint8_t instruction=DccPacket::instruction(b.onLow && address>=b.onLow && address<=b.onHigh);
      bool bit=startupRemaining?packetBit(position,0,0x41,0x41):b.first?((slot&1)?stopBit(position,address):packetBit(position,address,instruction,address^instruction)):bool(banks[active].packets[slot][position>>3] & (0x80>>(position&7)));
      duration=bit?ONE:ZERO;
      second=true;return duration;
    }
    second=false;
    if(++position==(startupRemaining?BITS:banks[active].first?((slot&1)?53:BITS):banks[active].sizes[slot])){
      position=0;++packets;
      if(startupRemaining)--startupRemaining;
      else if(++slot>=banks[active].slots)slot=0;
    }
    return duration;
  }
 private:
  volatile bool startupPending=false;
  uint8_t startupRemaining=0,position=0,slot=0;
  uint16_t duration=ONE;
  bool second=false;
};
