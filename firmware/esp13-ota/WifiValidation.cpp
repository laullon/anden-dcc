#include "WifiValidation.h"

bool validSsid(const char* text, size_t size) {
  if (!size || size > 32)
    return false;
  for (size_t i = 0; i < size; ++i)
    if (static_cast<unsigned char>(text[i]) < 0x20 || text[i] == 0x7F)
      return false;
  return true;
}

bool validWifiKey(const char* text, size_t size) {
  if (!size)
    return true; // Open Wi-Fi network.
  if (size < 8 || size > 64)
    return false;
  for (size_t i = 0; i < size; ++i) {
    unsigned char c = text[i];
    if (c < 0x20 || c > 0x7E)
      return false;
    if (size == 64 && !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
      return false;
  }
  return true;
}
