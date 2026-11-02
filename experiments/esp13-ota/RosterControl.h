#pragma once
#include <LittleFS.h>
#include <LocoRoster.h>

LocoRoster roster;
bool rosterStorage=false;
int rosterNumber(const String& text,int maximum){
  uint16_t value;if(!AndenLink::number(text.c_str(),value)||!value||value>maximum)return -1;return value;
}
void loadRoster(){
  roster.save(0,3,"","electric");roster.save(0,4,"","diesel");
  LittleFSConfig config;config.setAutoFormat(false);LittleFS.setConfig(config);
  rosterStorage=LittleFS.begin();
  // v0.1.x never used this reserved filesystem. Initialize it on first use.
  if(!rosterStorage && LittleFS.format())rosterStorage=LittleFS.begin();
  if(!rosterStorage || !LittleFS.exists("/locos.cfg"))return;
  File file=LittleFS.open("/locos.cfg","r");if(!file || file.size()>1024)return;
  if(file.readStringUntil('\n')!="anden-roster-2")return;
  LocoRoster loaded;
  while(file.available()){String line=file.readStringUntil('\n');if(!loaded.loadRecord(line.c_str(),false))return;}
  roster=loaded;
}
bool saveRoster(const LocoRoster& value){
  if(!rosterStorage)return false;
  File file=LittleFS.open("/locos.tmp","w");if(!file)return false;
  String data="anden-roster-2\n";
  for(uint8_t i=0;i<value.count;++i){const auto& l=value.entries[i];
    data+=String(l.id)+"\t"+String(l.address)+"\t"+LocoRoster::typeName(l.type)+"\t"+l.name+"\n";}
  bool ok=file.print(data)==data.length();file.close();
  if(ok)ok=LittleFS.rename("/locos.tmp","/locos.cfg");
  if(!ok)LittleFS.remove("/locos.tmp");return ok;
}
String dccStatus(){
  String out="{\"ready\":";out+=uno.matches(roster)?"true":"false";
  out+=",\"running\":";out+=uno.dccReady()?"true":"false";
  out+=",\"pin\":10,\"halfOneUs\":58,\"halfZeroUs\":100,\"maxLocos\":8,\"trackPower\":false,\"packets\":"+String(uno.packetCount());
  out+=",\"timingFaults\":"+String(uno.faultCount())+",\"locos\":[";
  for(uint8_t i=0;i<roster.count;++i){if(i)out+=',';const auto& l=roster.entries[i];
    out+="{\"id\":"+String(l.id)+",\"address\":"+String(l.address)+",\"name\":"+jsonString(l.name)+",\"type\":"+jsonString(LocoRoster::typeName(l.type));
    out+=",\"f0\":";out+=l.f0?"true":"false";
    char bytes[32];snprintf(bytes,sizeof(bytes),",\"bytes\":[\"%02X\",\"%02X\",\"%02X\"]}",l.address,DccPacket::instruction(l.f0),DccPacket::checksum(l.address,l.f0));out+=bytes;
  }
  return out+"]}";
}
void rosterRoutes(){
  server.on("/dcc/f0",HTTP_POST,[]{
    int address=rosterNumber(server.arg("address"),127);String on=server.arg("on");
    if(address<0 || (on!="0" && on!="1")){sendJson(400,F("{\"error\":\"Invalid address or F0 state\"}"));return;}
    int index=roster.findAddress(address);if(index<0){sendJson(404,F("{\"error\":\"Loco not in roster\"}"));return;}
    LocoRoster changed=roster;changed.entries[index].f0=on=="1";
    if(!uno.syncRoster(changed)){sendJson(503,F("{\"error\":\"Uno did not acknowledge the DCC roster\"}"));return;}
    roster=changed;sendJson(200,dccStatus());
  });
  server.on("/locos/save",HTTP_POST,[]{
    int id=server.arg("id").length()?rosterNumber(server.arg("id"),8):0;
    int address=rosterNumber(server.arg("address"),127);String name=server.arg("name"),type=server.arg("type");name.trim();
    if(id<0 || address<0){sendJson(400,F("{\"error\":\"Invalid ID or address\"}"));return;}
    LocoRoster changed=roster;
    auto result=changed.save(id,address,name.c_str(),server.hasArg("type")?type.c_str():nullptr);
    if(result!=LocoRoster::Result::Ok){sendJson(result==LocoRoster::Result::Invalid?400:result==LocoRoster::Result::Missing?404:409,F("{\"error\":\"Invalid name/type, duplicate address or roster full\"}"));return;}
    if(!saveRoster(changed)){sendJson(500,F("{\"error\":\"Could not save roster\"}"));return;}
    roster=changed;uno.invalidate();uno.syncRoster(roster);sendJson(200,dccStatus());
  });
  server.on("/locos/remove",HTTP_POST,[]{
    int id=rosterNumber(server.arg("id"),8);LocoRoster changed=roster;
    if(id<0){sendJson(400,F("{\"error\":\"Invalid ID\"}"));return;}
    if(!changed.remove(id)){sendJson(404,F("{\"error\":\"Loco not found\"}"));return;}
    if(!saveRoster(changed)){sendJson(500,F("{\"error\":\"Could not save roster\"}"));return;}
    roster=changed;uno.invalidate();uno.syncRoster(roster);sendJson(200,dccStatus());
  });
}
