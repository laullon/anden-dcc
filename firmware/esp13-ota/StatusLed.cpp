#include "StatusLed.h"
#include <string.h>

bool StatusLed::request(const char* method, const char* path, uint32_t now) {
  // The server hook supplies the path with its query string removed.
  if ((!strcmp(method, "GET") || !strcmp(method, "HEAD")) && !strcmp(path, "/api/status"))
    return false;
  if (!strcmp(path, "/api/track/keep"))
    return false;
  active = true;
  lastActivity = now;
  ++activityRequests;
  return true;
}

bool StatusLed::on(uint32_t now, bool connected) {
  if (active && uint32_t(now - lastActivity) >= PULSE_MS)
    active = false;
  return !active && (connected || (now / HEARTBEAT_MS) % 2 == 0);
}

uint32_t StatusLed::requests() const {
  return activityRequests;
}
