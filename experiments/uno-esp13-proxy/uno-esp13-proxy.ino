#include <Arduino.h>

// Experimental stop-and-wait bridge for the stacked ESP13's shared UART.
// USB requests and reports use ASCII hex (never ESP SLIP delimiters).
// The ESP request runs in a separate phase; replies are buffered, never echoed.
uint8_t packet[640];
uint16_t sequence = 0;

uint32_t crcByte(uint32_t crc, uint8_t value) {
  crc ^= value;
  for (uint8_t i=0;i<8;++i) crc=(crc>>1) ^ ((crc&1)?0xEDB88320UL:0);
  return crc;
}

int readByte() {
  unsigned long start=millis();
  while (!Serial.available()) if (millis()-start>3000) return -1;
  return Serial.read();
}

int readHex() {
  int value=0;
  for (uint8_t i=0;i<2;++i) {
    int c=readByte(),n;
    if(c>='0' && c<='9') n=c-'0';
    else if(c>='A' && c<='F') n=c-'A'+10;
    else if(c>='a' && c<='f') n=c-'a'+10;
    else return -1;
    value=(value<<4)|n;
  }
  return value;
}

void hexByte(uint8_t value) {
  const char* digits="0123456789ABCDEF";
  Serial.write(digits[value>>4]);Serial.write(digits[value&15]);
}

void error(const __FlashStringHelper* text) {
  Serial.print(F("\n!E1"));hexByte(sequence>>8);hexByte(sequence);
  Serial.write(':');Serial.println(text);Serial.flush();
}

void report(uint16_t size) {
  uint8_t head[]={uint8_t(sequence>>8),uint8_t(sequence),uint8_t(size>>8),uint8_t(size)};
  uint32_t crc=0xFFFFFFFFUL;
  for(uint8_t b:head)crc=crcByte(crc,b);
  for(uint16_t i=0;i<size;++i)crc=crcByte(crc,packet[i]);
  crc^=0xFFFFFFFFUL;
  Serial.print(F("\n!R1"));
  for(uint8_t b:head)hexByte(b);
  for(int8_t shift=24;shift>=0;shift-=8)hexByte(crc>>shift);
  for(uint16_t i=0;i<size;++i)hexByte(packet[i]);
  Serial.println();Serial.flush();
}

void request() {
  uint8_t head[12];
  for(uint8_t i=0;i<sizeof(head);++i) {
    int b=readHex();if(b<0){error(F("HEADER"));return;}head[i]=b;
  }
  sequence=(uint16_t(head[0])<<8)|head[1];
  uint16_t size=(uint16_t(head[2])<<8)|head[3];
  uint16_t waitMs=(uint16_t(head[4])<<8)|head[5];
  uint8_t rate=head[6],operation=head[7];
  if(size>sizeof(packet) || rate>3 || operation>1 || waitMs>30000) {
    error(F("BOUNDS"));return;
  }
  uint32_t crc=0xFFFFFFFFUL;
  for(uint8_t i=0;i<8;++i)crc=crcByte(crc,head[i]);
  for(uint16_t i=0;i<size;++i) {
    int b=readHex();if(b<0){error(F("PAYLOAD"));return;}
    packet[i]=b;crc=crcByte(crc,b);
  }
  if(readByte()!='\n'){error(F("TERMINATOR"));return;}
  uint32_t expected=(uint32_t(head[8])<<24)|(uint32_t(head[9])<<16)|
                    (uint32_t(head[10])<<8)|head[11];
  if((crc^0xFFFFFFFFUL)!=expected){error(F("CRC"));return;}
  if(operation==0){report(size);return;} // Host round-trip test; no ESP traffic.

  Serial.flush();
  const unsigned long rates[]={115200,250000,500000,1000000};
  Serial.begin(rates[rate]);
  while(Serial.available())Serial.read();
  Serial.write(packet,size);Serial.flush();
  uint16_t received=0;
  bool overflow=false;
  unsigned long start=millis(),last=start;
  while(millis()-start<waitMs) {
    while(Serial.available()) {
      int c=Serial.read();
      if(received<sizeof(packet))packet[received++]=c;
      else overflow=true;
      last=millis();
    }
    if(received && millis()-last>=25)break;
  }
  Serial.begin(115200);
  if(overflow)error(F("OVERFLOW"));
  else report(received);
}

void setup() {
  digitalWrite(3,LOW);pinMode(3,OUTPUT);
  digitalWrite(11,LOW);pinMode(11,OUTPUT);
  const uint8_t controls[]={8,9,12,13};
  for(uint8_t pin:controls){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  Serial.begin(115200);delay(1500);
  Serial.println(F("\n!ANDEN_PROXY_V1 READY MOTOR_OUTPUTS_DISABLED"));Serial.flush();
}

void loop() {
  // Ignore ESP startup noise until a complete checked host frame arrives.
  static uint8_t prefix=0;
  const char marker[]="!A1";
  while(Serial.available()) {
    int c=Serial.read();
    if(c==marker[prefix]) {
      if(++prefix==3){prefix=0;request();}
    } else prefix=c=='!'?1:0;
  }
}
