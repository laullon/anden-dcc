#include <Arduino.h>
#include <AndenStackLink.h>

constexpr char VERSION[]="0.1.0";
constexpr uint8_t ENABLE_A=3,ENABLE_B=11,BRAKE_A=9,BRAKE_B=8,DIR_A=12,DIR_B=13;
AndenLink::Receiver receiver;
uint8_t ledEdges=0;
uint32_t nextEdge=0;

void holdOutputsDisabled() {
  digitalWrite(ENABLE_A,LOW);
  digitalWrite(ENABLE_B,LOW);
}

void sendState(uint16_t sequence) {
  uint8_t flags=(digitalRead(ENABLE_A)?1:0)|(digitalRead(ENABLE_B)?2:0)|
      (digitalRead(BRAKE_A)?4:0)|(digitalRead(BRAKE_B)?8:0)|
      (digitalRead(DIR_A)?16:0)|(digitalRead(DIR_B)?32:0);
  char body[AndenLink::MAX_BODY+1],wire[AndenLink::MAX_BODY+8];
  snprintf(body,sizeof(body),"AD1,%u,STATE,%s,%u,%u,%u,%u",sequence,VERSION,
      unsigned(analogRead(A0)),unsigned(analogRead(A1)),unsigned(flags),unsigned(ledEdges));
  size_t size=AndenLink::encode(wire,sizeof(wire),body);
  if (size) Serial.write(reinterpret_cast<const uint8_t*>(wire),size);
}

void command() {
  char* parts[3];
  if (AndenLink::fields(receiver.body,parts,3)!=3 || strcmp(parts[0],"AD1")) return;
  uint16_t sequence;
  if (!AndenLink::number(parts[1],sequence) || !sequence) return;
  if (!strcmp(parts[2],"LED")) {
    // L is D13, also shield direction B. Both bridge enables stay LOW.
    digitalWrite(DIR_B,HIGH);ledEdges=15;nextEdge=millis()+120;
  } else if (strcmp(parts[2],"PING") && strcmp(parts[2],"STATUS")) return;
  holdOutputsDisabled();sendState(sequence);
}

void setup() {
  const uint8_t pins[]={ENABLE_A,ENABLE_B,BRAKE_A,BRAKE_B,DIR_A,DIR_B};
  for (uint8_t pin:pins) {digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  Serial.begin(115200);
}

void loop() {
  holdOutputsDisabled();
  if (ledEdges && int32_t(millis()-nextEdge)>=0) {
    --ledEdges;
    digitalWrite(DIR_B,ledEdges ? !digitalRead(DIR_B) : LOW);
    nextEdge=millis()+120;
  }
  while (Serial.available()) {
    int c=Serial.read();
    if (c>=0 && receiver.feed(char(c))) command();
  }
}
