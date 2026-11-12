#pragma once
#include <stdint.h>

namespace CabLabel {
constexpr uint8_t LENGTH = 16;
// Display labels are bounded ASCII; saved UTF-8 names remain unchanged.
void encode(char* hex, const char* name);
bool decode(char* label, const char* hex);
} // namespace CabLabel
