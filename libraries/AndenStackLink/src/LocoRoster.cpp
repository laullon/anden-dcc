#include "LocoRoster.h"
#include "DccPacket.h"

uint32_t LocoRoster::functionMask(const Loco& loco) {
  return (loco.functions & ~uint32_t(1)) | (loco.f0 ? 1 : 0);
}

bool LocoRoster::setFunction(Loco& loco, uint8_t number, bool on) {
  if (number > 28)
    return false;
  if (!number) {
    loco.f0 = on;
    return true;
  }
  if (on)
    loco.functions |= uint32_t(1) << number;
  else
    loco.functions &= ~(uint32_t(1) << number);
  return true;
}

uint8_t LocoRoster::dioramaMask() const {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < count; ++i)
    if (entries[i].onDiorama)
      mask |= 1 << i;
  return mask;
}

int LocoRoster::findId(uint8_t id) const {
  for (uint8_t i = 0; i < count; ++i)
    if (entries[i].id == id)
      return i;
  return -1;
}

int LocoRoster::findAddress(uint8_t address) const {
  for (uint8_t i = 0; i < count; ++i)
    if (entries[i].address == address)
      return i;
  return -1;
}

const char* LocoRoster::typeName(Type type) {
  return type == Type::Diesel ? "diesel" : type == Type::Steam ? "steam" : "electric";
}

bool LocoRoster::parseType(const char* text, Type& type) {
  if (!text)
    return false;
  if (!strcmp(text, "electric"))
    type = Type::Electric;
  else if (!strcmp(text, "diesel"))
    type = Type::Diesel;
  else if (!strcmp(text, "steam"))
    type = Type::Steam;
  else
    return false;
  return true;
}

LocoRoster::Result LocoRoster::save(uint8_t id, uint8_t address, const char* name,
                                    const char* type) {
  Type selected = Type::Electric;
  if (type && !parseType(type, selected))
    return Result::Invalid;
  if (!address || address > 127 || !name || strlen(name) > NAME_BYTES)
    return Result::Invalid;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(name); *p; ++p)
    if (*p < 32 || *p == 127)
      return Result::Invalid;
  int index = id ? findId(id) : -1;
  if (id && index < 0)
    return Result::Missing;
  int duplicate = findAddress(address);
  if (duplicate >= 0 && duplicate != index)
    return Result::Duplicate;
  if (index < 0) {
    if (count == CAPACITY)
      return Result::Full;
    for (id = 1; findId(id) >= 0; ++id) {
    }
    index = count++;
    entries[index].id = id;
  }
  // Changing decoder starts its requested lights off; renaming preserves F0.
  if (entries[index].address != address) {
    entries[index].f0 = false;
    entries[index].functions = 0;
    entries[index].forward = true;
  }
  if (type || !entries[index].address)
    entries[index].type = selected;
  entries[index].address = address;
  strcpy(entries[index].name, name);
  return Result::Ok;
}

bool LocoRoster::loadRecord(const char* line, bool legacy, bool hasPresence) {
  if (!line || strlen(line) > 80)
    return false;
  char record[81];
  strcpy(record, line);
  char* first = strchr(record, '\t');
  if (!first)
    return false;
  *first++ = 0;
  char* second = strchr(first, '\t');
  if (!second)
    return false;
  *second++ = 0;
  const char* type = "electric";
  char* name = second;
  if (!legacy) {
    char* third = strchr(second, '\t');
    if (!third)
      return false;
    *third++ = 0;
    type = second;
    name = third;
  }
  bool onDiorama = false;
  if (hasPresence) {
    char* separator = strchr(name, '\t');
    if (!separator)
      return false;
    *separator++ = 0;
    if (strcmp(name, "0") && strcmp(name, "1"))
      return false;
    onDiorama = *name == '1';
    name = separator;
  }
  auto decimal = [](const char* t, unsigned maximum) -> int {
    if (!*t || strlen(t) > 3)
      return -1;
    unsigned value = 0;
    for (; *t; ++t) {
      if (*t < '0' || *t > '9')
        return -1;
      value = value * 10 + (*t - '0');
    }
    return value >= 1 && value <= maximum ? int(value) : -1;
  };
  int id = decimal(record, CAPACITY), address = decimal(first, 127);
  if (id < 0 || address < 0 || findId(id) >= 0)
    return false;
  if (save(0, address, name, type) != Result::Ok)
    return false;
  entries[count - 1].id = id;
  entries[count - 1].onDiorama = onDiorama;
  return true;
}

bool LocoRoster::remove(uint8_t id) {
  int index = findId(id);
  if (index < 0)
    return false;
  for (uint8_t i = index; i + 1 < count; ++i)
    entries[i] = entries[i + 1];
  entries[--count] = {};
  return true;
}

void LocoRoster::schedule(uint32_t (&words)[CAPACITY]) const {
  for (uint8_t i = 0; i < CAPACITY; ++i) {
    if (count) {
      const auto& loco = entries[i % count];
      words[i] = DccPacket::word(loco.address, loco.f0);
    } else
      words[i] = (0xffu << 24) | (0xffu << 6); // NMRA idle FF 00 FF.
  }
}
