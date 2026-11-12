#pragma once
#include <stdint.h>
#include <CabLabel.h>

class CabDisplay {
public:
  void begin();
  void update(uint8_t address, uint8_t speed, const char* name);

private:
  uint8_t address = 255, speed = 255, page = 0, column = 0;
  char name[CabLabel::LENGTH + 1] = {};
  uint32_t lastWrite = 0;
  bool ready = false;
};
