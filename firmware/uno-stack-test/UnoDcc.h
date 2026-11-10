#pragma once
#include <stdint.h>

namespace UnoDcc {
// 20 us mirror phase and 1 us sampled-lateness allowance.
constexpr uint16_t BRIDGE_DELAY_TICKS = 40;
constexpr uint16_t BRIDGE_LATE_TICKS = 2;
constexpr uint8_t MAX_CRAWL_STEP = 3;

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
bool functions(uint8_t count, const uint8_t* addresses, uint8_t directions, const uint32_t* flags,
               uint8_t cabMask = 0);
// DCC speed code 1 is emergency stop. Codes 2 and above are motion.
bool motion(uint8_t address, uint8_t speedCode);
bool find(uint8_t first, uint8_t last, uint8_t low, uint8_t high);
} // namespace UnoDcc
