#pragma once
#include <stdint.h>
// Browser liveness is separate from the Uno's independent two-second cutoff.
class BrowserLease {
public:
  static constexpr uint32_t TIMEOUT_MS = 6000, KEEP_MS = 500;
  void seed(uint16_t value);
  uint16_t start(uint32_t now);
  bool accepts(uint16_t value, uint32_t now) const;
  bool touch(uint16_t value, uint32_t now);
  bool expired(uint32_t now) const;
  bool due(uint32_t now) const;
  void renewed(uint32_t now);
  bool active() const;
  void stop();

private:
  uint16_t token = 0, sequence = 0;
  // Track the latest browser heartbeat and latest Uno renewal separately.
  uint32_t seen = 0, kept = 0;
};
