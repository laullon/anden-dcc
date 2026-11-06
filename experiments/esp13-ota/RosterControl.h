#pragma once
#include <LittleFS.h>
#include <LocoRoster.h>

#include <AddressFinder.h>
LocoRoster roster;
AddressFinder finder;
String finderStatus(){
  return "{\"stage\":"+jsonString(finder.name())+",\"active\":"+(finder.active()?"true":"false")+
    ",\"step\":"+String(finder.step)+",\"first\":"+String(finder.first)+",\"last\":"+String(finder.last)+
    ",\"low\":"+String(finder.low)+",\"high\":"+String(finder.high)+",\"middle\":"+String(finder.middle())+
    ",\"result\":"+String(finder.result)+",\"waitMs\":"+String(finder.wait(millis()))+"}";
}
bool applyFinder(const AddressFinder& value){
  unsigned low=0,high=0;
  if(value.stage==AddressFinder::Stage::VerifyOn){low=value.first;high=value.last;}
  else if(value.stage==AddressFinder::Stage::Split){low=value.low;high=value.middle();}
  else if(value.stage==AddressFinder::Stage::ConfirmOn)low=high=value.low;
  String command="FIND,"+String(value.first)+","+String(value.last)+","+String(low)+","+String(high);
  if(!uno.query(command.c_str()))return false;
  char spec[]={char(value.first),char(value.last),char(low),char(high)};
  // The checked query acknowledges only after the new packet bank is active.
  return uno.statusSignature()==AndenLink::crc16(spec,4);
}
void serviceFinder(){
  if(!finder.active())return;
  if(!uno.trackActive() && finder.stage!=AddressFinder::Stage::Cleanup)finder.cancel(millis());
  if(finder.tick(millis()) && !applyFinder(finder)){uno.query("BENCHSTOP");finder.cancel(millis());}
  if(!finder.active()){uno.finderHold=false;uno.invalidate();uno.syncRoster(roster);}
}
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
  out+=",\"pin\":10,\"bridgePin\":12,\"halfOneUs\":58,\"halfZeroUs\":100,\"maxLocos\":8,\"trackPower\":";
  out+=uno.dccPowerActive()?"true":"false";out+=",\"packets\":"+String(uno.packetCount());
  out+=",\"timingFaults\":"+String(uno.faultCount())+",\"locos\":[";
  for(uint8_t i=0;i<roster.count;++i){if(i)out+=',';const auto& l=roster.entries[i];
    out+="{\"id\":"+String(l.id)+",\"address\":"+String(l.address)+",\"name\":"+jsonString(l.name)+",\"type\":"+jsonString(LocoRoster::typeName(l.type));
    out+=",\"forward\":";out+=l.forward?"true":"false";out+=",\"speed\":0";
    out+=",\"f0\":";out+=l.f0?"true":"false";
    char bytes[32];snprintf(bytes,sizeof(bytes),",\"bytes\":[\"%02X\",\"%02X\",\"%02X\"]}",l.address,DccPacket::instruction(l.f0),DccPacket::checksum(l.address,l.f0));out+=bytes;
  }
  return out+"],\"finder\":"+finderStatus()+"}";
}
#include "BrowserLease.h"
BrowserLease browserLease;
const char* trackStopReason="Track A output is disabled.";
void serviceTrackLease(){
  if(!browserLease.active())return;
  if(browserLease.expired(millis())){
    browserLease.stop();trackStopReason="Browser connection lost (6 seconds).";uno.query("BENCHSTOP");return;
  }
  if(browserLease.due(millis())){
    if(!uno.query("KEEP") || !uno.trackActive()){
      browserLease.stop();trackStopReason="Uno link or output stopped.";uno.query("BENCHSTOP");return;
    }
    browserLease.renewed(millis());
  }
}
void trackRoutes(){
  server.on("/api/track/on",HTTP_POST,[]{
    if(finder.active() || !uno.query("PING") || !uno.trackSupported() || !uno.outputsDisabled()){sendUnoError(409,F("Track firmware and a connected, stopped roster are required."));return;}
    if(!uno.syncRoster(roster) || !uno.query("TRACKON") || !uno.trackActive()){sendUnoError(409,F("Track start refused. Check current cutoff and timing status."));return;}
    uint16_t trackLease=browserLease.start(millis());trackStopReason="Track A output is disabled.";
    sendJson(200,"{\"lease\":"+String(trackLease)+"}");
  });
  server.on("/api/track/keep",HTTP_POST,[]{
    uint16_t lease;
    if(!AndenLink::number(server.arg("lease").c_str(),lease) || !browserLease.accepts(lease,millis())){sendUnoError(409,F("Track session expired. Turn power on explicitly."));return;}
    if(!uno.query("KEEP") || !uno.trackActive()){browserLease.stop();trackStopReason="Uno link or output stopped.";sendUnoError(409,F("Track stopped. Check status before restarting."));return;}
    browserLease.touch(lease,millis());browserLease.renewed(millis());
    sendJson(200,F("{\"ok\":true}"));
  });
  server.on("/api/track/off",HTTP_POST,[]{
    if(server.hasArg("lease")){
      uint16_t lease;
      if(!AndenLink::number(server.arg("lease").c_str(),lease) || !browserLease.accepts(lease,millis())){sendUnoError(409,F("This page no longer owns the track session."));return;}
    }
    browserLease.stop();trackStopReason=server.hasArg("lease")?"Control page closed or reloaded.":"Power off requested.";bool ok=uno.query("BENCHSTOP") && uno.outputsDisabled();
    sendJson(ok?200:503,ok?F("{\"ok\":true}"):F("{\"error\":\"Stop not acknowledged; the local timeout still applies.\"}"));
  });
}
void finderRoutes(){
  server.on("/finder/start",HTTP_POST,[]{
    if(finder.active() || !uno.query("PING") || !uno.trackActive()){sendUnoError(409,F("Power on for the stationary test before starting a search."));return;}
    int first=rosterNumber(server.arg("first"),127),last=rosterNumber(server.arg("last"),127);
    if(first<0 || last<first){sendJson(400,F("{\"error\":\"Use an ordered range from 1 to 127. Address 0 is broadcast.\"}"));return;}
    AddressFinder changed=finder;if(!changed.start(first,last,millis())){sendUnoError(409,F("Search unavailable."));return;}
    uno.finderHold=true;
    if(!applyFinder(changed)){uno.finderHold=false;uno.query("BENCHSTOP");sendUnoError(503,F("Search schedule was not acknowledged; power stopped."));return;}
    finder=changed;sendJson(200,dccStatus());
  });
  server.on("/finder/answer",HTTP_POST,[]{
    serviceFinder();String answer=server.arg("answer");
    if(server.arg("step")!=String(finder.step) || (answer!="on" && answer!="off" && answer!="retry")){sendUnoError(409,F("Refresh the current search question."));return;}
    AddressFinder changed=finder;bool ok=answer=="retry"?changed.retry(finder.step,millis()):changed.answer(answer=="on",finder.step,millis());
    if(!ok){sendUnoError(409,F("Wait for the lights to settle before answering."));return;}
    if(!applyFinder(changed)){uno.query("BENCHSTOP");sendUnoError(503,F("Search update failed; power stopped."));return;}
    finder=changed;sendJson(200,dccStatus());
  });
  server.on("/finder/cancel",HTTP_POST,[]{
    if(server.arg("step")!=String(finder.step)){sendUnoError(409,F("Refresh the current search first."));return;}
    if(finder.active()){
      if(finder.stage!=AddressFinder::Stage::Cleanup){finder.cancel(millis());if(!applyFinder(finder))uno.query("BENCHSTOP");}
    }else finder.dismiss();
    sendJson(200,dccStatus());
  });
}
void rosterRoutes(){
  server.on("/dcc/direction",HTTP_POST,[]{
    if(finder.active()){sendUnoError(409,F("Finish or cancel the address search first."));return;}
    int address=rosterNumber(server.arg("address"),127);String forward=server.arg("forward");
    if(!uno.trackSupported()){sendUnoError(409,F("Install the new Uno firmware first."));return;}
    int index=address<0?-1:roster.findAddress(address);
    if(index<0 || (forward!="0" && forward!="1")){sendJson(400,F("{\"error\":\"Invalid locomotive or direction\"}"));return;}
    LocoRoster changed=roster;changed.entries[index].forward=forward=="1";
    if(!uno.syncRoster(changed)){sendUnoError(503,F("Uno did not acknowledge direction."));return;}
    roster=changed;sendJson(200,dccStatus());
  });
  server.on("/dcc/f0",HTTP_POST,[]{
    if(finder.active()){sendUnoError(409,F("Finish or cancel the address search first."));return;}
    int address=rosterNumber(server.arg("address"),127);String on=server.arg("on");
    if(address<0 || (on!="0" && on!="1")){sendJson(400,F("{\"error\":\"Invalid address or F0 state\"}"));return;}
    int index=roster.findAddress(address);if(index<0){sendJson(404,F("{\"error\":\"Loco not in roster\"}"));return;}
    LocoRoster changed=roster;changed.entries[index].f0=on=="1";
    if(!uno.syncRoster(changed)){sendJson(503,F("{\"error\":\"Uno did not acknowledge the DCC roster\"}"));return;}
    roster=changed;sendJson(200,dccStatus());
  });
  server.on("/locos/save",HTTP_POST,[]{
    if(finder.active()){sendUnoError(409,F("Finish or cancel the address search first."));return;}
    if(!uno.outputsDisabled()){sendUnoError(409,F("Turn track power off before editing the roster."));return;}
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
    if(finder.active()){sendUnoError(409,F("Finish or cancel the address search first."));return;}
    if(!uno.outputsDisabled()){sendUnoError(409,F("Turn track power off before editing the roster."));return;}
    int id=rosterNumber(server.arg("id"),8);LocoRoster changed=roster;
    if(id<0){sendJson(400,F("{\"error\":\"Invalid ID\"}"));return;}
    if(!changed.remove(id)){sendJson(404,F("{\"error\":\"Loco not found\"}"));return;}
    if(!saveRoster(changed)){sendJson(500,F("{\"error\":\"Could not save roster\"}"));return;}
    roster=changed;uno.invalidate();uno.syncRoster(roster);sendJson(200,dccStatus());
  });
}
