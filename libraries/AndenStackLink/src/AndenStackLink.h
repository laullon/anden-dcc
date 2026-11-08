#pragma once
#include <stddef.h>
#include <stdint.h>

namespace AndenLink {
constexpr size_t MAX_BODY = 144;

uint16_t crc16(const char* data, size_t size);

// Frames use @body*CCCC followed by LF. CRC covers the body bytes only.
// Return the wire length, or zero if the body or output capacity is invalid.
size_t encode(char* output, size_t capacity, const char* body);

class Receiver {
public:
  char body[MAX_BODY + 6];
  uint32_t validFrames = 0, badFrames = 0;
  // True exposes a checked, NUL-terminated body until the next frame arrives.
  bool feed(char c);

private:
  size_t used = 0;
  bool collecting = false;
};

bool number(const char* text, uint16_t& value);

bool number32(const char* text, uint32_t& value, uint32_t maximum = 0xFFFFFFFFUL);
// Split in place. The resulting field pointers borrow the mutable frame buffer.
size_t fields(char* text, char** parts, size_t capacity);
} // namespace AndenLink
