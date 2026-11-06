#pragma once
#include <stdint.h>

// Bench pulses and stationary track control. The ADC limit remains uncalibrated.
class BridgeBench {
 public:
  static constexpr uint16_t DURATION_MS=3000,RAW_LIMIT=100;
  enum State : uint8_t {Idle,Running,Complete,Stopped,RawTrip,TimingFault,GuardExpired,LeaseExpired};
  State state=Idle;
  enum Mode : uint8_t {DcReverse,DcForward,Dcc,Track};
  uint8_t mode=DcReverse;
  bool direction=false;
  bool active() const {return state==Running;}
  bool start(uint8_t requestedMode,uint32_t now,uint16_t raw,bool healthy) {
    if(requestedMode>Track || active())return false; // A second command cannot extend a running pulse.
    if(!healthy){state=TimingFault;return false;}
    if(raw>=RAW_LIMIT){state=RawTrip;return false;}
    mode=requestedMode;direction=mode==DcForward;started=now;state=Running;return true;
  }
  void poll(uint32_t now,uint16_t raw,bool healthy,bool guardExpired) {
    if(!active())return;
    if(!healthy)state=TimingFault;
    else if(guardExpired)state=GuardExpired;
    else if(raw>=RAW_LIMIT)state=RawTrip;
    else if(uint32_t(now-started)>=duration())state=mode==Track?LeaseExpired:Complete;
  }
  uint16_t duration() const {return mode==Track?2000:DURATION_MS;}
  bool renew(uint32_t now){if(!active() || mode!=Track)return false;if(uint32_t(now-started)>=duration()){state=LeaseExpired;return false;}started=now;return true;}
  void stop(){if(active())state=Stopped;}
  uint16_t remaining(uint32_t now) const {
    uint32_t elapsed=now-started;
    return active() && elapsed<duration() ? duration()-elapsed : 0;
  }
 private:
  uint32_t started=0;
};

// Main arms/disarms with interrupts masked. Timer1 calls tick() each half-bit.
// 60,000 halves last 3.48–6 seconds at the fixed 58/100 us timings, providing
// a second cutoff if the main loop stops servicing its 3-second deadline.
class BridgeGuard {
 public:
  static constexpr uint16_t HALVES=60000;
  volatile bool expired=false;
  void arm(){remaining=HALVES;expired=false;}
  void disarm(){remaining=0;}
  bool tick(){if(remaining && !--remaining){expired=true;return true;}return false;}
 private:
  volatile uint16_t remaining=0;
};
static_assert(uint32_t(BridgeGuard::HALVES)*58>uint32_t(BridgeBench::DURATION_MS)*1000,"Guard must outlast the normal pulse");
