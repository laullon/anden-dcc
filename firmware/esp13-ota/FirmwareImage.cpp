#include "FirmwareImage.h"

constexpr size_t FirmwareImage::LIMIT;

namespace {
constexpr char APP_ID[] = "ANDEN_ESP13_OTA_V1";
// Prefix lengths for the identity matcher, allowing identifiers across chunks.
constexpr uint8_t PREFIX[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
// elf2bin stores length and CRC after the 4 KiB loader and sketch headers.
constexpr size_t SIZE_OFFSET = 4096 + 16;
constexpr size_t CRC_OFFSET = SIZE_OFFSET + 4;
static_assert(sizeof(PREFIX) == sizeof(APP_ID) - 1, "Identity prefix table must match APP_ID");
} // namespace

void FirmwareImage::begin(size_t available) {
  capacity = available < LIMIT ? available : LIMIT;
  received = 0;
  crc = 0xFFFFFFFF;
  expectedSize = 0;
  expectedCrc = 0;
  matched = 0;
  identified = false;
  finished = false;
  failure = capacity ? nullptr : "No OTA update space is available.";
}

bool FirmwareImage::append(const uint8_t* data, size_t length) {
  if (failure)
    return false;
  if (finished) {
    failure = "Firmware upload has already finished.";
    return false;
  }
  if ((length && !data) || length > capacity - received) {
    failure = "Firmware exceeds the available OTA update space.";
    return false;
  }
  for (size_t index = 0; index < length; ++index, ++received) {
    uint8_t value = data[index];
    if (((received == 0 || received == 4096) && value != 0xE9) ||
        ((received == 3 || received == 4099) && value >> 4 != 2)) {
      failure = "Expected an Arduino ESP8266 image with the 1 MB flash profile.";
      return false;
    }
    if (!identified) {
      while (matched && value != uint8_t(APP_ID[matched]))
        matched = PREFIX[matched - 1];
      if (value == uint8_t(APP_ID[matched]))
        ++matched;
      identified = matched == sizeof(APP_ID) - 1;
    }
    if (received >= SIZE_OFFSET && received < CRC_OFFSET)
      expectedSize |= uint32_t(value) << (8 * (received - SIZE_OFFSET));
    if (received >= CRC_OFFSET && received < CRC_OFFSET + 4)
      expectedCrc |= uint32_t(value) << (8 * (received - CRC_OFFSET));
    // elf2bin calculates CRC-32/MPEG-2 with both metadata words set to zero.
    if (received >= SIZE_OFFSET && received < CRC_OFFSET + 4)
      value = 0;
    crc ^= uint32_t(value) << 24;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc << 1) ^ ((crc & 0x80000000) ? 0x04C11DB7 : 0);
  }
  return true;
}

bool FirmwareImage::finish() {
  if (failure)
    return false;
  if (received < CRC_OFFSET + 4 || expectedSize != received)
    failure = "Firmware image is truncated or has trailing data.";
  else if (!identified)
    failure = "Image does not identify as anden ESP13 OTA firmware.";
  else if (expectedCrc != crc)
    failure = "Firmware image checksum is invalid.";
  else
    finished = true;
  return !failure;
}

const char* FirmwareImage::error() const {
  return failure;
}

size_t FirmwareImage::size() const {
  return received;
}
