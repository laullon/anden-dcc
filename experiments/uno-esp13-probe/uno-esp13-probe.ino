#include <Arduino.h>
#include <string.h>

// Uno-only experiment. No DCC, motor PWM, Wi-Fi configuration or ESP flashing.
// ESP13 shares the Uno hardware UART with USB. Capture each reply silently,
// then return to 115200 for a bounded report; never continuously echo replies
// back to the ESP, which would create a serial feedback loop.
char reply[320];

void discardFor(unsigned ms) {
  unsigned long start=millis();
  do {while(Serial.available())Serial.read();} while(millis()-start<ms);
}

unsigned capture(unsigned ms) {
  unsigned count=0;unsigned long start=millis();
  while(millis()-start<ms) {
    while(Serial.available()) {
      int c=Serial.read();
      if(count<sizeof(reply)-1)reply[count++]=char(c);
    }
  }
  reply[count]=0;return count;
}

bool probe(unsigned long baud) {
  Serial.print(F("PROBE_BEGIN baud="));Serial.println(baud);Serial.flush();
  Serial.begin(baud);discardFor(250);
  Serial.print(F("\r\nAT\r\n"));Serial.flush();
  unsigned received=capture(800);
  bool ack=strstr(reply,"OK")!=nullptr;
  bool version=false;
  if(ack) {
    discardFor(200);
    Serial.print(F("AT+GMR\r\n"));Serial.flush();
    received+=capture(1200);
    version=strstr(reply,"version") || strstr(reply,"Version") || strstr(reply,"SDK");
  }
  Serial.flush();Serial.begin(115200);
  Serial.print(F("PROBE_RESULT baud="));Serial.print(baud);
  Serial.print(F(" at="));Serial.print(ack?F("OK"):F("NONE"));
  Serial.print(F(" firmware="));Serial.print(version?F("IDENTIFIED"):F("UNKNOWN"));
  Serial.print(F(" received="));Serial.println(received);Serial.flush();
  discardFor(300);
  return ack;
}

bool hasSyncReply(unsigned received) {
  uint8_t frame[32];
  unsigned length=0;
  bool escaped=false,overflow=false,inFrame=false;
  for(unsigned i=0;i<received;++i) {
    uint8_t b=uint8_t(reply[i]);
    if(b==0xC0) {
      if(inFrame && !overflow && !escaped && length>=10) {
        unsigned size=unsigned(frame[2]) | (unsigned(frame[3])<<8);
        if(frame[0]==1 && frame[1]==8 && size>=2 &&
           length==8+size && frame[length-2]==0 && frame[length-1]==0)
          return true;
      }
      length=0;escaped=false;overflow=false;inFrame=true;
      continue;
    }
    if(!inFrame || overflow)continue;
    if(escaped) {
      if(b==0xDC)b=0xC0;
      else if(b==0xDD)b=0xDB;
      else {overflow=true;continue;}
      escaped=false;
    } else if(b==0xDB) {escaped=true;continue;}
    if(length<sizeof(frame))frame[length++]=b;
    else overflow=true;
  }
  return false;
}

void pingRom() {
  // Espressif ROM SYNC only: no erase, flash/RAM write, or register commands.
  const uint8_t header[]={0xC0,0,8,36,0,0,0,0,0,7,7,0x12,0x20};
  discardFor(50);
  Serial.write(header,sizeof(header));
  for(uint8_t i=0;i<32;++i)Serial.write(uint8_t(0x55));
  Serial.write(uint8_t(0xC0));Serial.flush();
  unsigned received=capture(400);
  bool ok=hasSyncReply(received);
  Serial.print(F("ROM_SYNC status="));Serial.print(ok?F("OK"):F("NO_REPLY"));
  Serial.print(F(" received="));Serial.println(received);Serial.flush();
}

void setup() {
  // Standard unmodified L298P shield: both enables LOW throughout this test.
  digitalWrite(3,LOW);pinMode(3,OUTPUT);
  digitalWrite(11,LOW);pinMode(11,OUTPUT);
  const uint8_t controls[]={8,9,12,13};
  for(uint8_t pin:controls) {digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  Serial.begin(115200);delay(1500);
  Serial.println(F("ANDEN_UNO_ESP13_PROBE v3; MOTOR_OUTPUTS_DISABLED"));Serial.flush();
  discardFor(300);
  const unsigned long rates[]={115200,9600,57600,38400,19200,74880};
  bool found=false;
  for(unsigned long baud:rates)if(probe(baud)){found=true;break;}
  Serial.println(found?F("PROBE_DONE ESP_AT_RESPONDING"):F("PROBE_DONE NO_AT_RESPONSE"));
  Serial.println(F("BOOT_MONITOR_READY baud=74880; PRESS_ESP_RST"));
  Serial.flush();
  Serial.begin(74880);
}

void loop() {
  // Passively collect the ESP ROM boot message. Reporting is deliberately brief
  // and bounded: the shared TX wire must never echo incoming bytes back to ESP.
  unsigned stored=0;
  unsigned long received=0;
  unsigned long start=millis();
  while(millis()-start<8000) {
    while(Serial.available()) {
      int c=Serial.read();
      ++received;
      if(stored<sizeof(reply))reply[stored++]=char(c);
    }
  }
  Serial.begin(115200);
  Serial.print(F("BOOT_CAPTURE received="));Serial.print(received);
  Serial.print(F(" first64_hex="));
  for(unsigned i=0;i<stored && i<64;++i) {
    uint8_t b=uint8_t(reply[i]);
    if(b<16)Serial.print('0');
    Serial.print(b,HEX);
  }
  Serial.println();Serial.flush();
  discardFor(100);
  pingRom();
  discardFor(100);
  Serial.begin(74880);
}
