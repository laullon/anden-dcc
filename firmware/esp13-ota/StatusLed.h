#pragma once
#include <stdint.h>

// DOIT ESP13 shield's blue LED: GPIO16, active LOW. Not the generic core LED.
// Pure timing policy; the sketch applies its output without blocking delays.
class StatusLed {
public:
  static constexpr uint8_t PIN = 16;
  static constexpr uint32_t PULSE_MS = 120, HEARTBEAT_MS = 500;
  bool request(const char* method, const char* path, uint32_t now);
  bool on(uint32_t now, bool connected);
  uint32_t requests() const;

private:
  bool active = false;
  uint32_t lastActivity = 0, activityRequests = 0;
};
