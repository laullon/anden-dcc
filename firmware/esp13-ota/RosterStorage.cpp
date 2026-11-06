#include "RosterStorage.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <flash_hal.h>

namespace {
bool filesystemIsErased() {
  constexpr uint32_t MAPPED_FLASH = 0x40200000;
  if (FS_start < MAPPED_FLASH || FS_end <= FS_start)
    return false;
  const uint32_t address = FS_PHYS_ADDR, size = FS_PHYS_SIZE;
  const uint32_t flashSize = ESP.getFlashChipRealSize();
  if (address > flashSize || size > flashSize - address)
    return false;
  uint8_t bytes[128];
  for (uint32_t offset = 0; offset < size;) {
    const uint32_t chunk = size - offset < sizeof(bytes) ? size - offset : sizeof(bytes);
    if (flash_hal_read(address + offset, chunk, bytes) != FLASH_HAL_OK)
      return false;
    for (uint32_t i = 0; i < chunk; ++i)
      if (bytes[i] != 0xFF)
        return false;
    offset += chunk;
    yield();
  }
  return true;
}
} // namespace

bool mountRosterStorage() {
  LittleFSConfig config;
  config.setAutoFormat(false);
  if (!LittleFS.setConfig(config))
    return false;
  if (LittleFS.begin())
    return true;
  // A failed mount may contain recoverable configuration. Initialize only
  // first-use (all-FF) flash, never a nonblank or unreadable filesystem.
  return filesystemIsErased() && LittleFS.format() && LittleFS.begin();
}
