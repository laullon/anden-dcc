#pragma once
#include <Arduino.h>
#include <AndenStackLink.h>

class UnoLink {
 public:
  void begin() { Serial.setRxBufferSize(256);Serial.begin(115200);Serial.setDebugOutput(false); }
  void loop() {
    if (uint32_t(millis()-lastPoll)>=2000) query("STATUS");
  }
  bool query(const char* command) {
    lastPoll=millis();
    if (!++sequence) ++sequence;
    char body[40],wire[48];
    snprintf(body,sizeof(body),"AD1,%u,%s",sequence,command);
    size_t size=AndenLink::encode(wire,sizeof(wire),body);
    if (!size) return false;
    while (Serial.available()) receiver.feed(char(Serial.read()));
    Serial.write(reinterpret_cast<const uint8_t*>(wire),size);Serial.flush();
    uint32_t start=millis();
    while (uint32_t(millis()-start)<300) {
      while (Serial.available()) {
        int c=Serial.read();
        if (c>=0 && receiver.feed(char(c)) && accept(sequence)) {
          lastSeen=millis();roundTrip=lastSeen-start;known=true;return true;
        }
      }
      delay(1);
    }
    ++timeouts;return false;
  }
  bool online() const { return known && uint32_t(millis()-lastSeen)<6000; }
  bool outputsDisabled() const { return known && !(flags&3); }
  String json() const {
    String out=F("{\"online\":");out+=online()?"true":"false";
    out+=F(",\"version\":\"");out+=version;out+='"';
    out+=F(",\"lastSeenMs\":");out+=known?String(uint32_t(millis()-lastSeen)):"null";
    out+=F(",\"roundTripMs\":");out+=known?String(roundTrip):"null";
    out+=F(",\"rxFrames\":");out+=receiver.validFrames;
    out+=F(",\"badFrames\":");out+=receiver.badFrames;
    out+=F(",\"timeouts\":");out+=timeouts;
    out+=F(",\"enableA\":");out+=known?((flags&1)?"true":"false"):"null";
    out+=F(",\"enableB\":");out+=known?((flags&2)?"true":"false"):"null";
    out+=F(",\"adcA\":");out+=known?String(adcA):"null";
    out+=F(",\"adcB\":");out+=known?String(adcB):"null";
    out+=F(",\"ledOn\":");out+=known?((flags&32)?"true":"false"):"null";
    out+=F(",\"ledEdges\":");out+=known?String(ledEdges):"null";
    out+=F(",\"motorPowerTested\":false}");return out;
  }
 private:
  AndenLink::Receiver receiver;
  uint16_t sequence=0,adcA=0,adcB=0,flags=0,ledEdges=0;
  uint32_t lastPoll=0,lastSeen=0,roundTrip=0,timeouts=0;
  bool known=false;
  char version[12]={0};
  bool accept(uint16_t expected) {
    char* parts[8];
    if (AndenLink::fields(receiver.body,parts,8)!=8 || strcmp(parts[0],"AD1") ||
        strcmp(parts[2],"STATE")) return false;
    uint16_t id,a,b,f,edges;
    if (!AndenLink::number(parts[1],id) || id!=expected ||
        !AndenLink::number(parts[4],a) || a>1023 ||
        !AndenLink::number(parts[5],b) || b>1023 ||
        !AndenLink::number(parts[6],f) || f>63 ||
        !AndenLink::number(parts[7],edges) || edges>15) return false;
    size_t length=strlen(parts[3]);
    if (!length || length>=sizeof(version)) return false;
    for (size_t i=0;i<length;++i)
      if (!((parts[3][i]>='0' && parts[3][i]<='9') || parts[3][i]=='.')) return false;
    memcpy(version,parts[3],length+1);adcA=a;adcB=b;flags=f;ledEdges=edges;return true;
  }
};
