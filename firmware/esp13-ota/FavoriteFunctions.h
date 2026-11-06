#pragma once
#include <stdint.h>
#include <string.h>
#include <AndenStackLink.h>
class FavoriteFunctions {
 public:
  static constexpr uint8_t LOCOS=8,LIMIT=8,NAME_BYTES=24;
  struct Favorite {uint8_t number=0;char name[NAME_BYTES+1]={};};
  struct Set {uint8_t count=0;Favorite entries[LIMIT]={};};
  Set locos[LOCOS]={};
  static bool validName(const char* name){
    if(!name || !*name || strlen(name)>NAME_BYTES)return false;
    for(const unsigned char* p=(const unsigned char*)name;*p;++p)if(*p<32||*p==127)return false;return true;
  }
  int find(uint8_t id,uint8_t number) const {
    if(!id||id>LOCOS)return -1;const auto& set=locos[id-1];
    for(uint8_t i=0;i<set.count;++i)if(set.entries[i].number==number)return i;return -1;
  }
  bool save(uint8_t id,uint8_t number,const char* name){
    if(!id||id>LOCOS||number>28||!validName(name))return false;
    auto& set=locos[id-1];int index=find(id,number);
    if(index<0){if(set.count==LIMIT)return false;index=set.count++;}
    set.entries[index].number=number;strcpy(set.entries[index].name,name);return true;
  }
  bool remove(uint8_t id,uint8_t number){
    int index=find(id,number);if(index<0)return false;auto& set=locos[id-1];
    for(uint8_t i=index;i+1<set.count;++i)set.entries[i]=set.entries[i+1];set.entries[--set.count]={};return true;
  }
  void clear(uint8_t id){if(id&&id<=LOCOS)locos[id-1]={};}
  bool loadRecord(const char* record){
    if(!record || strlen(record)>40)return false;char text[41];strcpy(text,record);
    char* number=strchr(text,'\t');if(!number)return false;*number++=0;
    char* name=strchr(number,'\t');if(!name)return false;*name++=0;
    uint16_t id,fn;if(!AndenLink::number(text,id)||!id||id>LOCOS||!AndenLink::number(number,fn)||fn>28||find(id,fn)>=0)return false;
    return save(id,fn,name);
  }
};
