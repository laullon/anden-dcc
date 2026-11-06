#include "HttpSupport.h"
#include "UnoLink.h"

namespace HttpSupport {
String jsonString(const String& value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t c = value[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += char(c);
    } else if (c < 0x20) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", unsigned(c));
      out += escaped;
    } else
      out += char(c);
  }
  return out + '"';
}

void sendJson(ESP8266WebServer& server, int code, const String& body) {
  server.keepAlive(false); // Release the single HTTP client between control requests.
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void sendUnoError(ESP8266WebServer& server, const UnoLink& uno, int code, const String& error) {
  String body = uno.json();
  body.remove(body.length() - 1);
  body += ",\"error\":" + jsonString(error) + "}";
  sendJson(server, code, body);
}

} // namespace HttpSupport
