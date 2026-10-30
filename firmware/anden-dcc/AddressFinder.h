#pragma once
#include <stdint.h>
#include "DccPacket.h"

// Human-observed F0 search. No CV reads/writes, broadcast or speed packets.
class AddressFinder {
 public:
  static constexpr unsigned WORDS = 128;
  // 128 * 44 bits * 200 us < 1.13 s. Four seconds covers three full
  // worst-case refreshes plus the PIO FIFO and decoder response margin.
  static constexpr uint32_t SETTLE_MS = 4000, TIMEOUT_MS = 180000;
  enum class Stage { Idle, VerifyOn, VerifyOff, Split, ConfirmOn, ConfirmOff, Cleanup, Found, Failed };
  Stage stage = Stage::Idle;
  uint8_t first=1, last=100, low=1, high=100, result=0;
  uint32_t step=0, changed=0, action=0;
  bool success=false;
  bool active() const { return stage>=Stage::VerifyOn && stage<=Stage::Cleanup; }
  uint8_t middle() const { return low+(high-low)/2; }
  uint32_t wait(uint32_t now) const {
    uint32_t elapsed=now-changed;
    return active() && elapsed<SETTLE_MS ? SETTLE_MS-elapsed : 0;
  }
  const char* name() const {
    switch(stage) {
      case Stage::VerifyOn:return "verifyOn"; case Stage::VerifyOff:return "verifyOff";
      case Stage::Split:return "split"; case Stage::ConfirmOn:return "confirmOn";
      case Stage::ConfirmOff:return "confirmOff"; case Stage::Cleanup:return "cleanup";
      case Stage::Found:return "found"; case Stage::Failed:return "failed";
      default:return "idle";
    }
  }
  bool start(unsigned lo,unsigned hi,uint32_t now) {
    if(active() || lo<1 || hi>127 || lo>hi) return false;
    first=low=lo; last=high=hi;result=0;success=false;
    change(Stage::VerifyOn,now);return true;
  }
  bool answer(bool on,uint32_t token,uint32_t now) {
    if(!active() || stage==Stage::Cleanup || token!=step || wait(now))return false;
    switch(stage) {
      case Stage::VerifyOn: if(on)change(Stage::VerifyOff,now);else finish(false,now);break;
      case Stage::VerifyOff: if(!on) next(now);else finish(false,now);break;
      case Stage::Split:
        if(on)high=middle();else low=middle()+1;
        next(now);break;
      case Stage::ConfirmOn: if(on)change(Stage::ConfirmOff,now);else finish(false,now);break;
      case Stage::ConfirmOff: finish(!on,now);break;
      default:return false;
    }
    return true;
  }
  bool retry(uint32_t token,uint32_t now) {
    if(!active() || stage==Stage::Cleanup || token!=step || wait(now))return false;
    change(stage,now);return true;
  }
  void cancel(uint32_t now) { if(active() && stage!=Stage::Cleanup)finish(false,now); }
  void dismiss() { if(!active()) {stage=Stage::Idle;result=0;++step;} }
  // Returns true only when a timeout changes the transmitted schedule.
  bool tick(uint32_t now) {
    if(stage==Stage::Cleanup && !wait(now)) {stage=success?Stage::Found:Stage::Failed;return false;}
    if(active() && stage!=Stage::Cleanup && uint32_t(now-action)>=TIMEOUT_MS) {finish(false,now);return true;}
    return false;
  }
  void schedule(uint32_t (&words)[WORDS]) const {
    unsigned count=last-first+1;
    for(unsigned i=0;i<WORDS;++i) {
      uint8_t a=first+i%count;
      bool on=stage==Stage::VerifyOn || (stage==Stage::Split && a>=low && a<=middle()) ||
              (stage==Stage::ConfirmOn && a==low);
      words[i]=DccPacket::word(a,on);
    }
  }
 private:
  void change(Stage s,uint32_t now) {stage=s;changed=action=now;++step;}
  void next(uint32_t now) {change(low==high?Stage::ConfirmOn:Stage::Split,now);}
  void finish(bool ok,uint32_t now) {success=ok;result=ok?low:0;change(Stage::Cleanup,now);}
};
