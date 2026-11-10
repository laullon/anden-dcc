#pragma once
#include <stdint.h>

class CabControl {
public:
  void begin();
  void roster(uint8_t count, const uint8_t* addresses, uint8_t cabMask);
  void clearRoster();
  bool poll(bool trackPowered);
  bool stop();
  uint8_t address() const;
  uint8_t step() const;

private:
  uint8_t addresses[8] = {};
  uint8_t count = 0, selected = 0, speed = 0, phase = 0;
  int8_t transitions = 0;
  bool selectLatched = false, stopLatched = false;
  uint32_t selectLowAt = 0, stopLowAt = 0, motionStarted = 0;
};
