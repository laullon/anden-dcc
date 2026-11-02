#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "SetupPage.h"
#include "WifiValidation.h"
#include "UnoLink.h"

// ESP8266 roster + stack bench test. No bridge-enable commands.
// Keep the same flash layout for subsequent OTA builds.
constexpr char VERSION[] = "0.2.0";
constexpr char APP_ID[] = "ANDEN_ESP13_OTA_V1";
constexpr char HOST[] = "anden-esp13";
const IPAddress AP_IP(192, 168, 13, 1);
UnoLink uno;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater(false);  // No debug output on the Uno UART.
bool mdnsReady = false;
bool apReady = false;
bool stationConfigured = false;
bool pendingForget = false;
bool pendingConnect = false;
uint32_t bootId = 0;
uint32_t applyAt = 0;
String nextSsid, nextPassword;

String jsonString(const String& value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t c = value[i];
    if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
    else if (c < 0x20) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", unsigned(c));
      out += escaped;
    } else out += char(c);
  }
  return out + '"';
}

uint32_t otaCapacity() {
  uint32_t free = ESP.getFreeSketchSpace();
  uint32_t available = free > 0x1000 ? (free - 0x1000) & 0xFFFFF000 : 0;
  // Keep room for another image of the same size on the 1 MB layout.
  return min(available, uint32_t(448 * 1024));
}

void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

#include "RosterControl.h"

void sendStatus() {
  bool connected = WiFi.status() == WL_CONNECTED;
  String body;
  body.reserve(500);
  body = F("{\"name\":\"anden-esp13\",\"platform\":\"esp8266\",\"version\":");
  body += jsonString(VERSION);
  body += F(",\"appId\":"); body += jsonString(APP_ID);
  body += F(",\"bootId\":"); body += bootId;
  body += F(",\"uptimeMs\":"); body += millis();
  body += F(",\"connected\":"); body += connected ? "true" : "false";
  body += F(",\"configured\":"); body += stationConfigured ? "true" : "false";
  body += F(",\"ssid\":"); body += jsonString(WiFi.SSID());
  body += F(",\"ip\":"); body += jsonString(connected ? WiFi.localIP().toString() : "");
  body += F(",\"apReady\":"); body += apReady ? "true" : "false";
  body += F(",\"apIp\":"); body += jsonString(WiFi.softAPIP().toString());
  body += F(",\"mdnsReady\":"); body += mdnsReady ? "true" : "false";
  body += F(",\"otaMaxBytes\":"); body += otaCapacity();
  body += F(",\"sketchBytes\":"); body += ESP.getSketchSize();
  body += F(",\"flashBytes\":"); body += ESP.getFlashChipRealSize();
  body += F(",\"layoutBytes\":"); body += ESP.getFlashChipSize();
  body += F(",\"freeHeap\":"); body += ESP.getFreeHeap();
  body += F(",\"uno\":"); body += uno.json();
  body += F(",\"storageReady\":");body+=rosterStorage?"true":"false";
  body += F(",\"dccImplemented\":true,\"trackPower\":false,\"dcc\":");body+=dccStatus();body+='}';
  sendJson(200, body);
}

void setup() {
  // Only checked protocol frames use the Uno UART. ROM startup noise is ignored.
  loadRoster();uno.attach(roster);uno.begin();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  bootId = uint32_t(random(0x7FFFFFFF));
  WiFi.hostname(HOST);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  apReady = WiFi.softAP(HOST);  // Open recovery/setup network, intentionally.
  stationConfigured = WiFi.SSID().length() > 0;
  WiFi.setAutoReconnect(stationConfigured);
  if (stationConfigured) WiFi.begin();

  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Content-Encoding","gzip");
    server.send_P(200,"text/html; charset=utf-8",reinterpret_cast<const char*>(SETUP_PAGE),sizeof(SETUP_PAGE));
  });
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/uno/ping", HTTP_POST, [] {
    bool ok=uno.query("PING");sendJson(ok?200:503,uno.json());
  });
  server.on("/api/uno/led", HTTP_POST, [] {
    if (!uno.query("PING")) {sendJson(503,uno.json());return;}
    if (!uno.outputsDisabled()) {
      sendJson(409,F("{\"error\":\"Uno enable pins must both be LOW for the LED test.\"}"));return;
    }
    bool ok=uno.query("LED");sendJson(ok?200:503,uno.json());
  });
  server.on("/setup/wifi", HTTP_POST, [] {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    if (!validSsid(ssid.c_str(), ssid.length()) ||
        !validWifiKey(password.c_str(), password.length())) {
      sendJson(400, F("{\"error\":\"Use a network name of 1–32 bytes and a blank key, an 8–63 character WPA key, or a 64 digit hexadecimal key.\"}"));
      return;
    }
    nextSsid = ssid; nextPassword = password;
    pendingConnect = true; pendingForget = false;
    applyAt = millis() + 750;
    sendJson(202, F("{\"message\":\"Settings accepted. Connecting shortly; check the network status above.\"}"));
  });
  server.on("/setup/wifi/forget", HTTP_POST, [] {
    pendingForget = true; pendingConnect = false;
    nextSsid = ""; nextPassword = "";
    applyAt = millis() + 750;
    sendJson(202, F("{\"message\":\"Saved Wi-Fi will be forgotten. The setup network stays available.\"}"));
  });
  // Tested core uploader handles flash staging, image checks, and reboot.
  // No authentication, as requested for this local prototype.
  updater.setup(&server, "/update");
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  rosterRoutes();
  server.begin();
}

void loop() {
  server.handleClient();
  uno.loop();
  if ((pendingConnect || pendingForget) && int32_t(millis() - applyAt) >= 0) {
    bool forget = pendingForget;
    pendingConnect = pendingForget = false;
    WiFi.persistent(true);  // Save only explicit user changes in SDK settings.
    if (forget) {
      WiFi.setAutoReconnect(false);
      WiFi.disconnect(false, true);
      stationConfigured = false;
    } else {
      WiFi.disconnect(false, false);
      WiFi.begin(nextSsid.c_str(), nextPassword.c_str());
      WiFi.setAutoReconnect(true);
      stationConfigured = true;
    }
    WiFi.persistent(false);
    nextSsid = ""; nextPassword = "";
  }
  if (!mdnsReady) {
    static uint32_t retryAt = 0;
    if (int32_t(millis() - retryAt) >= 0) {
      mdnsReady = MDNS.begin(HOST);
      if (mdnsReady) MDNS.addService("http", "tcp", 80);
      retryAt = millis() + 10000;
    }
  }
  if (mdnsReady) MDNS.update();
  delay(1);
}
