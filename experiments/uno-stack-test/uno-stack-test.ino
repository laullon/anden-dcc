#include <Arduino.h>
#include <AndenStackLink.h>
#include "UnoDcc.h"

constexpr char VERSION[]="0.3.0";
constexpr uint8_t ENABLE_A=3,ENABLE_B=11,BRAKE_A=9,BRAKE_B=8,DIR_A=12,DIR_B=13;
AndenLink::Receiver receiver;
BridgeBench bench;
uint8_t ledEdges=0;
uint16_t rawA=0;
uint32_t nextEdge=0,lastSense=0;

void serviceBridge(bool fresh=false) {
  uint32_t now=millis();
  if(fresh || (bench.active() && uint32_t(now-lastSense)>=1)){
    rawA=analogRead(A0);lastSense=now;
  }
  bool wasActive=bench.active();
  bench.poll(now,rawA,UnoDcc::running,UnoDcc::bridgeGuard.expired);
  if(wasActive && !bench.active())UnoDcc::disarmBridge();
  digitalWrite(ENABLE_B,LOW);
  if(!bench.active()){digitalWrite(ENABLE_A,LOW);digitalWrite(DIR_A,LOW);}
}

bool startBridge(bool direction) {
  serviceBridge(true);
  if(digitalRead(ENABLE_A) || digitalRead(ENABLE_B) || bench.active())return false;
  if(!bench.start(direction,millis(),rawA,UnoDcc::running))return false;
  ledEdges=0;digitalWrite(DIR_B,LOW);
  digitalWrite(DIR_A,direction?HIGH:LOW); // Brake A remains LOW: opposite bridge inputs.
  if(!UnoDcc::armBridge()){
    bench.poll(millis(),rawA,false,false);serviceBridge();return false;
  }
  return true;
}

void sendState(uint16_t sequence) {
  serviceBridge(true);
  uint8_t flags=(digitalRead(ENABLE_A)?1:0)|(digitalRead(ENABLE_B)?2:0)|
      (digitalRead(BRAKE_A)?4:0)|(digitalRead(BRAKE_B)?8:0)|
      (digitalRead(DIR_A)?16:0)|(digitalRead(DIR_B)?32:0);
  char body[AndenLink::MAX_BODY+1],wire[AndenLink::MAX_BODY+8];
  auto dcc=UnoDcc::status();
  snprintf(body,sizeof(body),"AD1,%u,STATE,%s,%u,%u,%u,%u,%u,%u,%u,%u,%lu,%u,%u,%u",sequence,VERSION,
      unsigned(rawA),unsigned(analogRead(A1)),unsigned(flags),unsigned(ledEdges),
      unsigned(dcc.running),unsigned(dcc.count),unsigned(dcc.signature),unsigned(dcc.mask),dcc.packets,unsigned(dcc.faults),
      unsigned(bench.remaining(millis())),unsigned(bench.state));
  size_t size=AndenLink::encode(wire,sizeof(wire),body);
  if(size)Serial.write(reinterpret_cast<const uint8_t*>(wire),size);
}

void command() {
  char* parts[20];size_t n=AndenLink::fields(receiver.body,parts,20);
  if(n<3 || strcmp(parts[0],"AD1"))return;
  uint16_t sequence;if(!AndenLink::number(parts[1],sequence)||!sequence)return;
  serviceBridge();
  if(!strcmp(parts[2],"BENCHSTOP") && n==3){
    bench.stop();UnoDcc::disarmBridge();serviceBridge();sendState(sequence);return;
  }
  if(!strcmp(parts[2],"PULSE") && n==4){
    uint16_t direction;if(!AndenLink::number(parts[3],direction)||direction>1)return;
    startBridge(direction);sendState(sequence);return;
  }
  if(!strcmp(parts[2],"ROSTER")){
    if(bench.active())return;
    uint16_t count;if(n<4 || !AndenLink::number(parts[3],count)||count>8 || n!=4+count*2)return;
    uint8_t addresses[8]={0},mask=0;
    for(uint8_t i=0;i<count;++i){uint16_t address,on;
      if(!AndenLink::number(parts[4+i*2],address)||!address||address>127 ||
         !AndenLink::number(parts[5+i*2],on)||on>1)return;
      addresses[i]=address;if(on)mask|=1<<i;
    }
    if(!UnoDcc::set(count,addresses,mask))return;
  }else if(n!=3)return;
  else if(!strcmp(parts[2],"LED")){
    if(bench.active())return;
    digitalWrite(DIR_B,HIGH);ledEdges=15;nextEdge=millis()+120;
  }else if(strcmp(parts[2],"PING") && strcmp(parts[2],"STATUS"))return;
  serviceBridge();sendState(sequence);
}

void setup() {
  const uint8_t pins[]={ENABLE_A,ENABLE_B,BRAKE_A,BRAKE_B,DIR_A,DIR_B};
  for(uint8_t pin:pins){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  Serial.begin(115200);UnoDcc::begin();
}

void loop() {
  serviceBridge();
  if(ledEdges && int32_t(millis()-nextEdge)>=0){
    --ledEdges;digitalWrite(DIR_B,ledEdges?!digitalRead(DIR_B):LOW);nextEdge=millis()+120;
  }
  while(Serial.available()){
    serviceBridge();
    int c=Serial.read();if(c>=0 && receiver.feed(char(c)))command();
  }
}
