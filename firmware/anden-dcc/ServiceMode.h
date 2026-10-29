#pragma once
#include <stdint.h>

// Host-tested building blocks only. The live GP2 engine transmits three-byte
// F0 frames; it must never be fed these four-byte service-mode packets.
namespace ServiceMode {
constexpr uint8_t PREAMBLE_BITS = 20;
struct Packet {
  uint8_t bytes[4] = {};
  uint8_t size = 0;
  bool valid() const { return size != 0; }
};
enum class Command : uint8_t { VerifyByte = 1, Bit = 2, WriteByte = 3 };

constexpr Packet direct(uint16_t cv, uint8_t data, Command command) {
  if (cv < 1 || cv > 1024) return {};
  uint16_t index = cv - 1;
  uint8_t first = 0x70 | (uint8_t(command) << 2) | (index >> 8);
  uint8_t second = index & 0xff;
  return {{first, second, data, uint8_t(first ^ second ^ data)}, 4};
}
constexpr Packet verifyByte(uint16_t cv, uint8_t value) {
  return direct(cv, value, Command::VerifyByte);
}
constexpr Packet writeByte(uint16_t cv, uint8_t value) {
  return direct(cv, value, Command::WriteByte);
}
constexpr Packet bit(uint16_t cv, uint8_t position, bool value, bool write) {
  if (position > 7) return {};
  return direct(cv, 0xe0 | (write ? 0x10 : 0) | (value ? 8 : 0) | position, Command::Bit);
}
constexpr Packet reset() { return {{0, 0, 0, 0}, 3}; }

struct Address {
  uint16_t value = 0;
  bool extended = false;
  bool valid = false;
};
// Only call with CVs obtained from verified reads. Zero/invalid values are not
// an address-found result. This does not pretend a missing ACK means bit zero.
constexpr Address address(uint8_t cv29, uint8_t cv1, uint8_t cv17, uint8_t cv18) {
  bool extended = cv29 & 0x20;
  if (!extended) return {cv1, false, cv1 >= 1 && cv1 <= 127};
  uint16_t value = (uint16_t(cv17 & 0x3f) << 8) | cv18;
  return {value, true, cv17 >= 192 && cv17 <= 231 && value >= 1 && value <= 10239};
}

struct ShortAddressPlan {
  Packet setAddress;
  Packet selectShort;
  bool valid() const { return setAddress.valid() && selectShort.valid(); }
};
constexpr ShortAddressPlan shortAddress(uint16_t value) {
  if (value < 1 || value > 127) return {};
  // Set CV1, then clear only CV29 bit 5. Never replace the whole CV29 with a
  // guessed default; direction, speed steps and analog settings must survive.
  return {writeByte(1, value), bit(29, 5, false, true)};
}

enum class VerifyResult { Zero, One, NoAck, Ambiguous };
constexpr VerifyResult verifiedBit(bool ackForZero, bool ackForOne) {
  return ackForZero ? (ackForOne ? VerifyResult::Ambiguous : VerifyResult::Zero)
                    : (ackForOne ? VerifyResult::One : VerifyResult::NoAck);
}
}
