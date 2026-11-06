#pragma once
#include <stddef.h>
#include <stdint.h>

// Streaming validator for the uncompressed Arduino ESP8266 core 3.1.2 image.
// No upload is committed until finish() has accepted the complete image.
class FirmwareImage {
public:
  static constexpr size_t LIMIT = 448 * 1024;

  void begin(size_t capacity);
  bool append(const uint8_t* data, size_t length);
  bool finish();
  const char* error() const;
  size_t size() const;

private:
  size_t capacity = 0;
  size_t received = 0;
  uint32_t crc = 0xFFFFFFFF;
  uint32_t expectedSize = 0;
  uint32_t expectedCrc = 0;
  uint8_t matched = 0;
  bool identified = false;
  bool finished = false;
  const char* failure = nullptr;
};
