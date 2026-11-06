#pragma once
#include <stdint.h>
class FavoriteFunctions {
public:
  static constexpr uint8_t LOCOS = 8, LIMIT = 8, NAME_BYTES = 24;
  struct Favorite {
    uint8_t number = 0;
    char name[NAME_BYTES + 1] = {};
  };
  struct Set {
    uint8_t count = 0;
    Favorite entries[LIMIT] = {};
  };
  Set locos[LOCOS] = {};
  static bool validName(const char* name);
  int find(uint8_t id, uint8_t number) const;
  bool save(uint8_t id, uint8_t number, const char* name);
  bool remove(uint8_t id, uint8_t number);
  void clear(uint8_t id);
  bool loadRecord(const char* record);
};
