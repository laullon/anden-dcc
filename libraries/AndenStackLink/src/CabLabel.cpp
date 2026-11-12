#include "CabLabel.h"
#include <string.h>

namespace {
const char HEX[] = "0123456789ABCDEF";

int nibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}
} // namespace

void CabLabel::encode(char* hex, const char* name) {
  // Latin-1 U+00C0..U+00FF, folded for the small uppercase display font.
  const char latin[] = "AAAAAA?CEEEEIIIIDNOOOOO?OUUUUY?S"
                       "AAAAAA?CEEEEIIIIDNOOOOO?OUUUUY?Y";
  static_assert(sizeof(latin) == 65, "Latin-1 folding covers 64 code points");
  if (!*name)
    name = "LOCOMOTIVE";
  uint8_t count = 0;
  while (*name && count < LENGTH) {
    uint8_t first = uint8_t(*name++);
    char c = first >= 32 && first <= 126 ? char(first) : '?';
    if (first >= 128) {
      if (first == 0xC3 && (uint8_t(*name) & 0xC0) == 0x80)
        c = latin[uint8_t(*name) - 0x80];
      // Consume a whole UTF-8 character, including unsupported scripts/emoji.
      while ((uint8_t(*name) & 0xC0) == 0x80)
        ++name;
    }
    if (c >= 'a' && c <= 'z')
      c -= 'a' - 'A';
    hex[count * 2] = HEX[uint8_t(c) >> 4];
    hex[count * 2 + 1] = HEX[uint8_t(c) & 15];
    ++count;
  }
  hex[count * 2] = 0;
}

bool CabLabel::decode(char* label, const char* hex) {
  const size_t length = strlen(hex);
  if (length > LENGTH * 2 || length % 2)
    return false;
  // Validate before publishing any bytes to the caller's label.
  for (size_t i = 0; i < length; i += 2) {
    int high = nibble(hex[i]), low = nibble(hex[i + 1]);
    if (high < 0 || low < 0 || (high * 16 + low) < 32 || (high * 16 + low) > 126)
      return false;
  }
  for (size_t i = 0; i < length; i += 2)
    label[i / 2] = char(nibble(hex[i]) * 16 + nibble(hex[i + 1]));
  label[length / 2] = 0;
  return true;
}
