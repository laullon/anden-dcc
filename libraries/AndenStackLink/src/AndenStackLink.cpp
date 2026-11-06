#include "AndenStackLink.h"
#include <string.h>

namespace AndenLink {
namespace {
int hex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}
} // namespace

bool Receiver::feed(char c) {
  if (c == '@') {
    used = 0;
    collecting = true;
    return false;
  }
  if (!collecting || c == '\r')
    return false;
  if (c == '\n') {
    collecting = false;
    if (used < 6 || body[used - 5] != '*') {
      ++badFrames;
      return false;
    }
    uint16_t expected = 0;
    for (size_t i = used - 4; i < used; ++i) {
      int nibble = hex(body[i]);
      if (nibble < 0) {
        ++badFrames;
        return false;
      }
      expected = uint16_t((expected << 4) | nibble);
    }
    size_t size = used - 5;
    if (size > MAX_BODY || expected != crc16(body, size)) {
      ++badFrames;
      return false;
    }
    body[size] = 0;
    ++validFrames;
    return true;
  }
  if (used >= sizeof(body) - 1 || uint8_t(c) < 32 || uint8_t(c) > 126) {
    collecting = false;
    ++badFrames;
    return false;
  }
  body[used++] = c;
  return false;
}

uint16_t crc16(const char* data, size_t size) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < size; ++i) {
    crc ^= uint16_t(uint8_t(data[i])) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 0x8000) ? uint16_t((crc << 1) ^ 0x1021) : uint16_t(crc << 1);
  }
  return crc;
}

size_t encode(char* output, size_t capacity, const char* body) {
  size_t size = strlen(body);
  if (!size || size > MAX_BODY || capacity < size + 8)
    return 0;
  for (size_t i = 0; i < size; ++i)
    if (body[i] == '@' || body[i] == '*' || uint8_t(body[i]) < 32 || uint8_t(body[i]) > 126)
      return 0;
  uint16_t crc = crc16(body, size);
  output[0] = '@';
  memcpy(output + 1, body, size);
  output[size + 1] = '*';
  const char digits[] = "0123456789ABCDEF";
  for (uint8_t i = 0; i < 4; ++i)
    output[size + 2 + i] = digits[(crc >> (12 - i * 4)) & 15];
  output[size + 6] = '\n';
  output[size + 7] = 0;
  return size + 7;
}

bool number(const char* text, uint16_t& value) {
  if (!*text)
    return false;
  uint32_t result = 0;
  for (; *text; ++text) {
    if (*text < '0' || *text > '9')
      return false;
    result = result * 10 + uint8_t(*text - '0');
    if (result > 65535)
      return false;
  }
  value = uint16_t(result);
  return true;
}

bool number32(const char* text, uint32_t& value, uint32_t maximum) {
  if (!*text)
    return false;
  uint32_t result = 0;
  for (; *text; ++text) {
    if (*text < '0' || *text > '9')
      return false;
    uint8_t digit = *text - '0';
    if (digit > maximum || result > (maximum - digit) / 10)
      return false;
    result = result * 10 + digit;
  }
  value = result;
  return true;
}

size_t fields(char* text, char** parts, size_t capacity) {
  size_t count = 0;
  while (true) {
    if (!*text || *text == ',' || count == capacity)
      return 0;
    parts[count++] = text;
    char* comma = strchr(text, ',');
    if (!comma)
      return count;
    *comma = 0;
    text = comma + 1;
  }
}

} // namespace AndenLink
