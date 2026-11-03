#pragma once
#include <Arduino.h>
#include <AndenStackLink.h>
#include <LocoRoster.h>
#include <UnoDccStream.h>

class UnoLink {
 public:
  void begin() { Serial.setRxBufferSize(256);Serial.begin(115200);Serial.setDebugOutput(false); }
  void attach(const LocoRoster& value){desired=&value;dirty=true;}
  bool finderHold=false;
  void invalidate(){dirty=true;}
  void loop() {
    if(uint32_t(millis()-lastPoll)>=(benchActive()?300:2000) && query("STATUS") && capable && desired && !finderHold && (outputsDisabled() || trackActive()) && !matches(*desired))syncRoster(*desired);
  }
  bool matches(const LocoRoster& value) const {
    uint8_t addresses[8]={0},speeds[8]={0},mask=0,directions=0;
    for(uint8_t i=0;i<value.count;++i){addresses[i]=value.entries[i].address;if(value.entries[i].f0)mask|=1<<i;if(value.entries[i].forward)directions|=1<<i;}
    return !dirty && (outputsDisabled() || trackActive()) && online() && dccRunning && dccCount==value.count && dccMask==mask &&
      dccSignature==(trackCapable?UnoDccStream::controlSignature(value.count,addresses,mask,directions,speeds):UnoDccStream::signature(value.count,addresses,mask));
  }
  bool syncRoster(const LocoRoster& value){
    if(!capable || !online() || (!outputsDisabled() && !trackActive()))return false;
    String command="ROSTER,"+String(value.count);
    uint8_t addresses[8]={0},speeds[8]={0},mask=0,directions=0;
    for(uint8_t i=0;i<value.count;++i){const auto& l=value.entries[i];addresses[i]=l.address;if(l.f0)mask|=1<<i;if(l.forward)directions|=1<<i;
      command+=","+String(l.address)+","+String(l.f0?1:0);}
    if(trackCapable){command="CONTROL,"+String(value.count)+","+String(mask)+","+String(directions);
      for(uint8_t i=0;i<value.count;++i)command+=","+String(addresses[i])+",0";
    }
    if(!query(command.c_str()) || !dccRunning || dccCount!=value.count || dccMask!=mask ||
       dccSignature!=(trackCapable?UnoDccStream::controlSignature(value.count,addresses,mask,directions,speeds):UnoDccStream::signature(value.count,addresses,mask))){dirty=true;return false;}
    dirty=false;return true;
  }
  bool trackSupported() const {return trackCapable;}
  bool trackActive() const {return benchActive() && mode==3;}
  bool dccReady() const {return online() && capable && dccRunning;}
  bool benchSupported() const {return benchCapable;}
  bool benchActive() const {return online() && benchCapable && benchState==1 && (flags&1);}
  uint8_t benchResult() const {return benchState;}
  uint8_t benchMode() const {return mode;}
  bool dccBenchSupported() const {return dccBenchCapable;}
  bool dccPowerActive() const {return benchActive() && mode>=2;}
  uint16_t statusSignature() const {return dccSignature;}
  uint32_t packetCount() const{return packets;}
  uint16_t faultCount() const{return faults;}

  bool query(const char* command) {
    lastPoll=millis();
    if (!++sequence) ++sequence;
    char body[AndenLink::MAX_BODY+1],wire[AndenLink::MAX_BODY+8];
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
    out+=F(",\"dccRunning\":");out+=dccRunning?"true":"false";
    out+=F(",\"dccCount\":");out+=dccCount;
    out+=F(",\"dccSignature\":");out+=dccSignature;
    out+=F(",\"dccMask\":");out+=dccMask;
    out+=F(",\"dccPackets\":");out+=packets;
    out+=F(",\"timingFaults\":");out+=faults;
    out+=F(",\"trackSupported\":");out+=trackCapable?"true":"false";
    out+=F(",\"trackActive\":");out+=trackActive()?"true":"false";
    out+=F(",\"bench\":{\"supported\":");out+=benchCapable?"true":"false";
    out+=F(",\"dccSupported\":");out+=dccBenchCapable?"true":"false";
    out+=F(",\"mode\":");out+=mode;
    out+=F(",\"active\":");out+=benchActive()?"true":"false";
    out+=F(",\"remainingMs\":");out+=benchRemaining;
    out+=F(",\"state\":");out+=benchState;
    out+=F(",\"rawLimit\":100},\"motorPowerTested\":false}");return out;
  }
 private:
  AndenLink::Receiver receiver;
  uint16_t sequence=0,adcA=0,adcB=0,flags=0,ledEdges=0;
  uint32_t lastPoll=0,lastSeen=0,roundTrip=0,timeouts=0;
  bool known=false;
  char version[12]={0};
  const LocoRoster* desired=nullptr;
  bool dirty=true,capable=false,dccRunning=false;
  uint16_t dccCount=0,dccMask=0,dccSignature=0,faults=0;
  uint32_t packets=0;
  bool benchCapable=false,dccBenchCapable=false,trackCapable=false;
  uint16_t benchRemaining=0;
  uint8_t benchState=0,mode=0;
  bool accept(uint16_t expected) {
    char* parts[18];
    size_t n=AndenLink::fields(receiver.body,parts,18);
    if ((n!=8 && n!=14 && n!=16 && n!=17 && n!=18) || strcmp(parts[0],"AD1") ||
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
    uint16_t running=0,count=0,sig=0,mask=0,misses=0;uint32_t total=0;
    if(n>=14){
      if(!AndenLink::number(parts[8],running)||running>1||!AndenLink::number(parts[9],count)||count>8||
         !AndenLink::number(parts[10],sig)||!AndenLink::number(parts[11],mask)||mask>255||
         !AndenLink::number(parts[13],misses)||!*parts[12])return false;
      for(const char* p=parts[12];*p;++p){if(*p<'0'||*p>'9'||total>(UINT32_MAX-uint8_t(*p-'0'))/10)return false;total=total*10+(*p-'0');}
    }
    uint16_t remaining=0,testState=0;
    if(n>=16 && (!AndenLink::number(parts[14],remaining)||remaining>3000 || !AndenLink::number(parts[15],testState)||testState>7))return false;
    uint16_t reportedMode=(f&16)?1:0;
    if(n>=17 && (!AndenLink::number(parts[16],reportedMode)||reportedMode>(n==18?3:2)))return false;
    benchCapable=n>=16;dccBenchCapable=n>=17;trackCapable=n==18 && !strcmp(parts[17],"1");benchRemaining=remaining;benchState=testState;mode=reportedMode;
    capable=n>=14;dccRunning=running;dccCount=count;dccSignature=sig;dccMask=mask;faults=misses;packets=total;
    memcpy(version,parts[3],length+1);adcA=a;adcB=b;flags=f;ledEdges=edges;return true;
  }
};
