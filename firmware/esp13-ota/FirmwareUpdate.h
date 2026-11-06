#pragma once
#include "FirmwareImage.h"
#include <ESP8266WebServer.h>

// Stage firmware only; commit after the entire multipart request is validated.
class FirmwareUpdate {
public:
  explicit FirmwareUpdate(ESP8266WebServer& webServer);
  void registerRoutes();
  void beginRequest(size_t capacity);

private:
  ESP8266WebServer& server;
  FirmwareImage image;
  const char* failure = nullptr;
  bool seenFile = false, complete = false, staging = false, hasPending = false;
  uint8_t pending = 0;
  void upload();
  void respond();
  void fail(const char* reason);
  void discard();
};
