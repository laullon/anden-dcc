#pragma once
#include <stddef.h>
#include <stdint.h>

namespace AndenLink {
constexpr size_t MAX_BODY = 144;

uint16_t crc16(const char* data, size_t size);

size_t encode(char* output, size_t capacity, const char* body);

class Receiver {
public:
  char body[MAX_BODY + 6];
  uint32_t validFrames = 0, badFrames = 0;
  bool feed(char c);

private:
  size_t used = 0;
  bool collecting = false;
};

bool number(const char* text, uint16_t& value);

bool number32(const char* text, uint32_t& value, uint32_t maximum = 0xFFFFFFFFUL);
size_t fields(char* text, char** parts, size_t capacity);
} // namespace AndenLink
