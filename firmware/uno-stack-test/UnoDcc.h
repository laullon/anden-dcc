#pragma once
#include <stdint.h>

namespace UnoDcc {
// 20 us mirror phase and 1 us sampled-lateness allowance.
constexpr uint16_t BRIDGE_DELAY_TICKS = 40;
constexpr uint16_t BRIDGE_LATE_TICKS = 2;

struct Status {
  uint8_t count, mask;
  uint16_t signature, faults;
  uint32_t packets;
  bool running;
};

void begin();
bool isRunning();
bool guardExpired();
bool bridgeStartPending();
bool bridgeIsActive();
void startStopped();
bool armBridge(bool dcc = false);
void disarmBridge();
void renewGuard();
Status status();
bool set(uint8_t count, const uint8_t* addresses, uint8_t mask);
bool control(uint8_t count, const uint8_t* addresses, uint8_t mask, uint8_t directions,
             const uint8_t* speeds);
bool functions(uint8_t count, const uint8_t* addresses, uint8_t directions, const uint32_t* flags);
bool find(uint8_t first, uint8_t last, uint8_t low, uint8_t high);
} // namespace UnoDcc
