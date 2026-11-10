#pragma once
#include <stdint.h>
#include <string.h>

// Names and addresses are configuration; function state is deliberately RAM-only.
class LocoRoster {
public:
  static constexpr uint8_t CAPACITY = 8;
  static constexpr uint8_t NAME_BYTES = 48;
  enum class Type : uint8_t { Electric, Diesel, Steam };
  struct Loco {
    uint8_t id = 0, address = 0;
    bool f0 = false, forward = true, onDiorama = false;
    uint32_t functions = 0;
    Type type = Type::Electric;
    char name[NAME_BYTES + 1] = {};
  };
  enum class Result { Ok, Invalid, Duplicate, Full, Missing };
  Loco entries[CAPACITY] = {};
  uint8_t count = 0;

  static uint32_t functionMask(const Loco& loco);
  static bool setFunction(Loco& loco, uint8_t number, bool on);
  uint8_t dioramaMask() const;
  int findId(uint8_t id) const;
  int findAddress(uint8_t address) const;
  static const char* typeName(Type type);
  static bool parseType(const char* text, Type& type);
  // Omitting type preserves existing cards and defaults new cards to electric.
  Result save(uint8_t id, uint8_t address, const char* name, const char* type = nullptr);
  // Decode the actual LittleFS record format on-device and in host checks.
  bool loadRecord(const char* line, bool legacy, bool hasPresence = false);
  bool remove(uint8_t id);
  void schedule(uint32_t (&words)[CAPACITY]) const;
};
