#pragma once
#include <stdint.h>

// Uno-only OLED transport. Timer1 remains able to preempt every transfer.
namespace CabI2c {
constexpr uint8_t MAX_BYTES = 31;
void begin();
bool write(uint8_t address, const uint8_t* bytes, uint8_t count);
} // namespace CabI2c
