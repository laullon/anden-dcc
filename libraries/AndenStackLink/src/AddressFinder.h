#pragma once
#include <stdint.h>

// Human-observed F0 search state. The Uno builds the live schedule with F0
// and zero-speed packets for each address in the original range.
class AddressFinder {
public:
  static constexpr unsigned WORDS = 128;
  // Default for the F0-only schedule model. The ESP allows more time for the
  // live range schedule, which also sends a stop packet per address.
  static constexpr uint32_t SETTLE_MS = 4000, TIMEOUT_MS = 180000;
  enum class Stage {
    Idle,
    VerifyOn,
    VerifyOff,
    Split,
    ConfirmOn,
    ConfirmOff,
    Cleanup,
    Found,
    Failed
  };
  Stage stage = Stage::Idle;
  uint8_t first = 1, last = 100, low = 1, high = 100, result = 0;
  uint32_t step = 0, changed = 0, action = 0, settleMs = SETTLE_MS;
  bool success = false;
  bool active() const;
  uint8_t middle() const;
  uint32_t wait(uint32_t now) const;
  const char* name() const;
  bool start(unsigned lo, unsigned hi, uint32_t now);
  bool answer(bool on, uint32_t token, uint32_t now);
  bool retry(uint32_t token, uint32_t now);
  void cancel(uint32_t now);
  void dismiss();
  // Returns true only when a timeout changes the transmitted schedule.
  bool tick(uint32_t now);
  // F0-only schedule used by the host model checks.
  void schedule(uint32_t (&words)[WORDS]) const;

private:
  void change(Stage s, uint32_t now);
  void next(uint32_t now);
  void finish(bool ok, uint32_t now);
};
