#pragma once
#include <stdint.h>

class CabDisplay {
public:
  void begin();
  void update(uint8_t address, uint8_t speed);

private:
  uint8_t address = 255, speed = 255, page = 0, column = 0;
  uint32_t lastWrite = 0;
  bool ready = false;
};
