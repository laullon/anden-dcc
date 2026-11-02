#pragma once
#include <stdint.h>

namespace DccPacket {
// Function Group One: F0/FL bit 4; F1-F4 intentionally off in this prototype.
constexpr uint8_t instruction(bool f0) { return f0 ? 0x90 : 0x80; }
constexpr uint8_t checksum(uint8_t address, bool f0) {
  return address ^ instruction(f0);
}
constexpr uint32_t word(uint8_t address, bool f0) {
  // 8 address + zero + 8 instruction + zero + 8 XOR, shifted MSB-first.
  return (uint32_t(address) << 24) | (uint32_t(instruction(f0)) << 15) |
         (uint32_t(checksum(address, f0)) << 6);
}
}
