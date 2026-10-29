#pragma once
#include <stdint.h>
#include <string.h>
#include "DccPacket.h"

// Names and addresses are configuration; function state is deliberately RAM-only.
class LocoRoster {
 public:
  static constexpr uint8_t CAPACITY = 8;
  static constexpr uint8_t NAME_BYTES = 48;
  struct Loco { uint8_t id = 0, address = 0; bool f0 = false; char name[NAME_BYTES + 1] = {}; };
  enum class Result { Ok, Invalid, Duplicate, Full, Missing };
  Loco entries[CAPACITY] = {};
  uint8_t count = 0;

  int findId(uint8_t id) const {
    for (uint8_t i = 0; i < count; ++i) if (entries[i].id == id) return i;
    return -1;
  }
  int findAddress(uint8_t address) const {
    for (uint8_t i = 0; i < count; ++i) if (entries[i].address == address) return i;
    return -1;
  }
  Result save(uint8_t id, uint8_t address, const char* name) {
    if (!address || address > 127 || !name || strlen(name) > NAME_BYTES) return Result::Invalid;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(name); *p; ++p)
      if (*p < 32 || *p == 127) return Result::Invalid;
    int index = id ? findId(id) : -1;
    if (id && index < 0) return Result::Missing;
    int duplicate = findAddress(address);
    if (duplicate >= 0 && duplicate != index) return Result::Duplicate;
    if (index < 0) {
      if (count == CAPACITY) return Result::Full;
      for (id = 1; findId(id) >= 0; ++id) {}
      index = count++;
      entries[index].id = id;
    }
    // Changing decoder starts its requested lights off; renaming preserves F0.
    if (entries[index].address != address) entries[index].f0 = false;
    entries[index].address = address;
    strcpy(entries[index].name, name);
    return Result::Ok;
  }
  bool remove(uint8_t id) {
    int index = findId(id);
    if (index < 0) return false;
    for (uint8_t i = index; i + 1 < count; ++i) entries[i] = entries[i + 1];
    entries[--count] = {};
    return true;
  }
  void schedule(uint32_t (&words)[CAPACITY]) const {
    for (uint8_t i = 0; i < CAPACITY; ++i) {
      if (count) {
        const auto& loco = entries[i % count];
        words[i] = DccPacket::word(loco.address, loco.f0);
      } else words[i] = (0xffu << 24) | (0xffu << 6); // NMRA idle FF 00 FF.
    }
  }
};
