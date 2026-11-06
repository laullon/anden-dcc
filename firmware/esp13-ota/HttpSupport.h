#pragma once
#include <Arduino.h>
#include <ESP8266WebServer.h>

class UnoLink;

namespace HttpSupport {
String jsonString(const String& value);
void sendJson(ESP8266WebServer& server, int code, const String& body);
void sendUnoError(ESP8266WebServer& server, const UnoLink& uno, int code, const String& error);
} // namespace HttpSupport
