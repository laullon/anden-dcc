#pragma once
#include <stdint.h>
namespace DccFunctions {
constexpr uint8_t MAX_FUNCTION = 28, GROUPS = 6;
constexpr uint32_t MASK = 0x1FFFFFFFUL;
inline uint8_t groupOne(uint32_t flags) {
  return 0x80 | ((flags & 1) ? 0x10 : 0) | ((flags >> 1) & 15);
}
// A complete refresh includes all five function groups and a zero-speed packet.
uint8_t packet(uint8_t address, uint32_t flags, bool forward, uint8_t group, uint8_t* out);
bool packetBit(uint8_t position, const uint8_t* bytes, uint8_t count);
} // namespace DccFunctions
