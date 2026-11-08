#include "AddressFinder.h"
#include "DccPacket.h"

bool AddressFinder::active() const {
  return stage >= Stage::VerifyOn && stage <= Stage::Cleanup;
}

uint8_t AddressFinder::middle() const {
  return low + (high - low) / 2;
}

uint32_t AddressFinder::wait(uint32_t now) const {
  uint32_t elapsed = now - changed;
  return active() && elapsed < settleMs ? settleMs - elapsed : 0;
}

const char* AddressFinder::name() const {
  switch (stage) {
  case Stage::VerifyOn:
    return "verifyOn";
  case Stage::VerifyOff:
    return "verifyOff";
  case Stage::Split:
    return "split";
  case Stage::ConfirmOn:
    return "confirmOn";
  case Stage::ConfirmOff:
    return "confirmOff";
  case Stage::Cleanup:
    return "cleanup";
  case Stage::Found:
    return "found";
  case Stage::Failed:
    return "failed";
  default:
    return "idle";
  }
}

bool AddressFinder::start(unsigned lo, unsigned hi, uint32_t now) {
  if (active() || lo < 1 || hi > 127 || lo > hi)
    return false;
  first = low = lo;
  last = high = hi;
  result = 0;
  success = false;
  change(Stage::VerifyOn, now);
  return true;
}

// Verify the lights respond in both states before splitting the range. The
// step token binds each answer to the question that produced it.
bool AddressFinder::answer(bool on, uint32_t token, uint32_t now) {
  if (!active() || stage == Stage::Cleanup || token != step || wait(now))
    return false;
  switch (stage) {
  case Stage::VerifyOn:
    if (on)
      change(Stage::VerifyOff, now);
    else
      finish(false, now);
    break;
  case Stage::VerifyOff:
    if (!on)
      next(now);
    else
      finish(false, now);
    break;
  case Stage::Split:
    if (on)
      high = middle();
    else
      low = middle() + 1;
    next(now);
    break;
  case Stage::ConfirmOn:
    if (on)
      change(Stage::ConfirmOff, now);
    else
      finish(false, now);
    break;
  case Stage::ConfirmOff:
    finish(!on, now);
    break;
  default:
    return false;
  }
  return true;
}

bool AddressFinder::retry(uint32_t token, uint32_t now) {
  if (!active() || stage == Stage::Cleanup || token != step || wait(now))
    return false;
  change(stage, now);
  return true;
}

void AddressFinder::cancel(uint32_t now) {
  if (active() && stage != Stage::Cleanup)
    finish(false, now);
}

void AddressFinder::dismiss() {
  if (!active()) {
    stage = Stage::Idle;
    result = 0;
    ++step;
  }
}

bool AddressFinder::tick(uint32_t now) {
  if (stage == Stage::Cleanup && !wait(now)) {
    stage = success ? Stage::Found : Stage::Failed;
    return false;
  }
  if (active() && stage != Stage::Cleanup && uint32_t(now - action) >= TIMEOUT_MS) {
    finish(false, now);
    return true;
  }
  return false;
}

void AddressFinder::schedule(uint32_t (&words)[WORDS]) const {
  unsigned count = last - first + 1;
  for (unsigned i = 0; i < WORDS; ++i) {
    uint8_t a = first + i % count;
    bool on = stage == Stage::VerifyOn || (stage == Stage::Split && a >= low && a <= middle()) ||
              (stage == Stage::ConfirmOn && a == low);
    words[i] = DccPacket::word(a, on);
  }
}

void AddressFinder::change(Stage s, uint32_t now) {
  stage = s;
  changed = action = now;
  ++step;
}

void AddressFinder::next(uint32_t now) {
  change(low == high ? Stage::ConfirmOn : Stage::Split, now);
}

void AddressFinder::finish(bool ok, uint32_t now) {
  // Clear search lights before reporting a result and restoring the roster.
  success = ok;
  result = ok ? low : 0;
  change(Stage::Cleanup, now);
}
