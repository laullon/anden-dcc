#pragma once
#include <stdint.h>
// Browser liveness is separate from the Uno's independent two-second cutoff.
class BrowserLease {
 public:
  static constexpr uint32_t TIMEOUT_MS=6000,KEEP_MS=500;
  void seed(uint16_t value){if(!token)sequence=value;}
  uint16_t start(uint32_t now){if(!++sequence)++sequence;token=sequence;seen=kept=now;return token;}
  bool accepts(uint16_t value,uint32_t now) const {return value && value==token && !expired(now);}
  bool touch(uint16_t value,uint32_t now){if(!accepts(value,now))return false;seen=now;return true;}
  bool expired(uint32_t now) const {return token && uint32_t(now-seen)>=TIMEOUT_MS;}
  bool due(uint32_t now) const {return token && !expired(now) && uint32_t(now-kept)>=KEEP_MS;}
  void renewed(uint32_t now){kept=now;}
  bool active() const {return token!=0;}
  void stop(){token=0;}
 private:
  uint16_t token=0,sequence=0;
  uint32_t seen=0,kept=0;
};
