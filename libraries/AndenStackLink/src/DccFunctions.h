#pragma once
#include <stdint.h>
namespace DccFunctions {
constexpr uint8_t MAX_FUNCTION=28,GROUPS=6;
constexpr uint32_t MASK=0x1FFFFFFFUL;
inline uint8_t groupOne(uint32_t flags){return 0x80|((flags&1)?0x10:0)|((flags>>1)&15);}
// A complete refresh includes all five function groups and a zero-speed packet.
inline uint8_t packet(uint8_t address,uint32_t flags,bool forward,uint8_t group,uint8_t* out){
  out[0]=address;uint8_t count=3;
  switch(group){
    case 0:out[1]=groupOne(flags);break;
    case 1:out[1]=0x3F;out[2]=forward?0x80:0;count=4;break;
    case 2:out[1]=0xB0|((flags>>5)&15);break;
    case 3:out[1]=0xA0|((flags>>9)&15);break;
    case 4:out[1]=0xDE;out[2]=(flags>>13)&255;count=4;break;
    case 5:out[1]=0xDF;out[2]=(flags>>21)&255;count=4;break;
    default:return 0;
  }
  uint8_t sum=0;for(uint8_t i=0;i<count-1;++i)sum^=out[i];out[count-1]=sum;return count;
}
inline bool packetBit(uint8_t position,const uint8_t* bytes,uint8_t count){
  if(position<16 || position==16+count*9)return true;
  if(position==16 || position==25 || position==34 || position==43)return false;
  if(position<25)return bytes[0]&(0x80>>(position-17));
  if(position<34)return bytes[1]&(0x80>>(position-26));
  if(position<43)return bytes[2]&(0x80>>(position-35));
  return bytes[3]&(0x80>>(position-44));
}
}
