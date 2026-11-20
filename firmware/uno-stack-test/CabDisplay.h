#pragma once
#include <stdint.h>
#include <CabLabel.h>

class CabDisplay {
public:
  void begin();
  void update(uint8_t address, uint8_t speed, const char* name,
              uint16_t rawCurrent, uint16_t peakCurrent, bool powered);

private:
  uint8_t address = 255, speed = 255, page = 0, column = 0;
  char name[CabLabel::LENGTH + 1] = {};
  uint16_t rawCurrent = 0, peakCurrent = 0;
  uint32_t lastWrite = 0, lastTelemetry = 0;
  bool ready = false, powered = false, currentOnly = false;
};
