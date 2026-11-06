#pragma once
#include <stdint.h>
#include <string.h>

// DOIT ESP13 shield's blue LED: GPIO16, active LOW. Not the generic core LED.
// Pure timing policy; the sketch applies its output without blocking delays.
class StatusLed {
 public:
  static constexpr uint8_t PIN=16;
  static constexpr uint32_t PULSE_MS=120,HEARTBEAT_MS=500;
  bool request(const char* method,const char* path,uint32_t now) {
    // The server hook supplies the path with its query string removed.
    if((!strcmp(method,"GET") || !strcmp(method,"HEAD")) && !strcmp(path,"/api/status"))return false;
    if(!strcmp(path,"/api/track/keep"))return false;
    active=true;lastActivity=now;++activityRequests;return true;
  }
  bool on(uint32_t now,bool connected) {
    if(active && uint32_t(now-lastActivity)>=PULSE_MS)active=false;
    return !active && (connected || (now/HEARTBEAT_MS)%2==0);
  }
  uint32_t requests() const {return activityRequests;}
 private:
  bool active=false;
  uint32_t lastActivity=0,activityRequests=0;
};
