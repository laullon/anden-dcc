#include "FirmwareUpdate.h"
#include "HttpSupport.h"
#include "RosterControl.h"
#include "SetupPage.h"
#include "StatusLed.h"
#include "UnoLink.h"
#include "WifiValidation.h"
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

using HttpSupport::jsonString;

// ESP Wi-Fi/web runtime. RosterControl owns the roster and output sessions.
// Keep the same flash layout for subsequent OTA builds.
constexpr char VERSION[] = "0.7.3";
constexpr char APP_ID[] = "ANDEN_ESP13_OTA_V1";
constexpr char HOST[] = "anden-esp13";
const IPAddress AP_IP(192, 168, 13, 1);
namespace {
void refreshStatusLed();
uint32_t otaCapacity();
void sendStatus();

UnoLink uno;
ESP8266WebServer server(80);
RosterControl rosterControl(server, uno);
FirmwareUpdate updater(server);
bool mdnsReady = false;
bool apReady = false;
bool stationConfigured = false;
bool pendingForget = false;
bool pendingConnect = false;
uint32_t bootId = 0;
uint32_t applyAt = 0;
String nextSsid, nextPassword;
StatusLed statusLed;
bool statusLedOn = false;

void refreshStatusLed() {
  bool on = statusLed.on(millis(), WiFi.status() == WL_CONNECTED);
  if (on != statusLedOn) {
    statusLedOn = on;
    digitalWrite(StatusLed::PIN, on ? LOW : HIGH);
  }
}

uint32_t otaCapacity() {
  uint32_t free = ESP.getFreeSketchSpace();
  uint32_t available = free > 0x1000 ? (free - 0x1000) & 0xFFFFF000 : 0;
  // Keep room for another image of the same size on the 1 MB layout.
  return min(available, uint32_t(448 * 1024));
}

void sendStatus() {
  refreshStatusLed();
  bool connected = WiFi.status() == WL_CONNECTED;
  String body;
  body.reserve(500);
  body = F("{\"name\":\"anden-esp13\",\"platform\":\"esp8266\",\"version\":");
  body += jsonString(VERSION);
  body += F(",\"appId\":");
  body += jsonString(APP_ID);
  body += F(",\"bootId\":");
  body += bootId;
  body += F(",\"uptimeMs\":");
  body += millis();
  body += F(",\"connected\":");
  body += connected ? "true" : "false";
  body += F(",\"configured\":");
  body += stationConfigured ? "true" : "false";
  body += F(",\"ssid\":");
  body += jsonString(WiFi.SSID());
  body += F(",\"ip\":");
  body += jsonString(connected ? WiFi.localIP().toString() : "");
  body += F(",\"apReady\":");
  body += apReady ? "true" : "false";
  body += F(",\"apIp\":");
  body += jsonString(WiFi.softAPIP().toString());
  body += F(",\"mdnsReady\":");
  body += mdnsReady ? "true" : "false";
  body += F(",\"otaMaxBytes\":");
  body += otaCapacity();
  body += F(",\"sketchBytes\":");
  body += ESP.getSketchSize();
  body += F(",\"flashBytes\":");
  body += ESP.getFlashChipRealSize();
  body += F(",\"layoutBytes\":");
  body += ESP.getFlashChipSize();
  body += F(",\"freeHeap\":");
  body += ESP.getFreeHeap();
  body += F(",\"led\":{\"pin\":16,\"activeLow\":true,\"on\":");
  body += statusLedOn ? "true" : "false";
  body += F(",\"pinHigh\":");
  body += digitalRead(StatusLed::PIN) == HIGH ? "true" : "false";
  body += F(",\"activityRequests\":");
  body += statusLed.requests();
  body += '}';
  body += F(",\"trackStopReason\":");
  body += jsonString(rosterControl.stopReason());
  body += F(",\"uno\":");
  body += uno.json();
  body += F(",\"storageReady\":");
  body += rosterControl.storageReady() ? "true" : "false";
  body += F(",\"dccImplemented\":true,\"trackPower\":");
  body += uno.dccPowerActive() ? "true" : "false";
  body += F(",\"dcc\":");
  body += rosterControl.dccStatus();
  body += '}';
  HttpSupport::sendJson(server, 200, body);
}

} // namespace

void setup() {
  digitalWrite(StatusLed::PIN, HIGH); // Active LOW: initialize OFF before output mode.
  pinMode(StatusLed::PIN, OUTPUT);
  refreshStatusLed();
  // Only checked protocol frames use the Uno UART. ROM startup noise is ignored.
  rosterControl.begin();
  uno.begin();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  bootId = uint32_t(random(0x7FFFFFFF));
  rosterControl.seedLease(uint16_t(bootId));
  WiFi.hostname(HOST);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  apReady = WiFi.softAP(HOST); // Open recovery/setup network, intentionally.
  stationConfigured = WiFi.SSID().length() > 0;
  WiFi.setAutoReconnect(stationConfigured);
  if (stationConfigured)
    WiFi.begin();

  server.addHook([](const String& method, const String& url, WiFiClient*,
                    ESP8266WebServer::ContentTypeFunction) {
    if (statusLed.request(method.c_str(), url.c_str(), millis())) {
      // Show the request immediately, including while a handler waits for the Uno.
      statusLedOn = false;
      digitalWrite(StatusLed::PIN, HIGH);
    }
    if (method == "POST" &&
        (url == "/update" || url == "/setup/wifi" || url == "/setup/wifi/forget") &&
        rosterControl.setupLocked()) {
      HttpSupport::sendJson(
          server, 409,
          F("{\"error\":\"Finish the search and turn track power off before setup changes.\"}"));
      return ESP8266WebServer::CLIENT_REQUEST_IS_HANDLED;
    }
    // The hook runs before multipart parsing, so reset upload state here.
    if (method == "POST" && url == "/update")
      updater.beginRequest(otaCapacity());
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });
  server.on("/", HTTP_GET, [] {
    server.keepAlive(false);
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html; charset=utf-8", reinterpret_cast<const char*>(SETUP_PAGE),
                  SETUP_PAGE_SIZE);
  });
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/uno/ping", HTTP_POST, [] {
    bool ok = uno.query("PING");
    HttpSupport::sendJson(server, ok ? 200 : 503, uno.json());
  });
  server.on("/api/uno/led", HTTP_POST, [] {
    if (!uno.query("PING")) {
      HttpSupport::sendJson(server, 503, uno.json());
      return;
    }
    if (!uno.outputsDisabled()) {
      HttpSupport::sendJson(
          server, 409, F("{\"error\":\"Uno enable pins must both be LOW for the LED test.\"}"));
      return;
    }
    bool ok = uno.query("LED");
    HttpSupport::sendJson(server, ok ? 200 : 503, uno.json());
  });
  server.on("/api/uno/bridge", HTTP_POST, [] {
    String direction = server.arg("direction");
    if (direction != "0" && direction != "1") {
      HttpSupport::sendUnoError(server, uno, 400, F("Direction must be 0 or 1."));
      return;
    }
    if (!uno.query("PING")) {
      HttpSupport::sendUnoError(server, uno, 503, F("The Uno did not reply."));
      return;
    }
    if (!uno.benchSupported()) {
      HttpSupport::sendUnoError(server, uno, 501, F("Install the Uno bench-test firmware first."));
      return;
    }
    if (!uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("An output is already enabled. Stop the test first."));
      return;
    }
    String command = "PULSE," + direction;
    if (!uno.query(command.c_str())) {
      HttpSupport::sendUnoError(server, uno, 503, F("The Uno did not acknowledge the pulse."));
      return;
    }
    if (!uno.benchActive()) {
      HttpSupport::sendUnoError(
          server, uno, 409,
          F("Pulse refused or cut off. Check the bench status and raw A0 reading."));
      return;
    }
    HttpSupport::sendJson(server, 200, uno.json());
  });
  server.on("/api/uno/dcc-test", HTTP_POST, [] {
    if (!uno.query("PING")) {
      HttpSupport::sendUnoError(server, uno, 503, F("The Uno did not reply."));
      return;
    }
    if (!uno.dccBenchSupported()) {
      HttpSupport::sendUnoError(server, uno, 501,
                                F("Install the Uno DCC bench-test firmware first."));
      return;
    }
    if (!uno.outputsDisabled() || !uno.matches(rosterControl.configuration())) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Wait for outputs off and the roster to synchronize."));
      return;
    }
    if (!uno.query("DCCTEST")) {
      HttpSupport::sendUnoError(server, uno, 503, F("The Uno did not acknowledge the DCC test."));
      return;
    }
    if (!uno.dccPowerActive()) {
      HttpSupport::sendUnoError(
          server, uno, 409, F("DCC test refused or cut off. Check the bench and timing status."));
      return;
    }
    HttpSupport::sendJson(server, 200, uno.json());
  });
  server.on("/api/uno/bench-stop", HTTP_POST, [] {
    if (!uno.query("BENCHSTOP")) {
      HttpSupport::sendUnoError(server, uno, 503,
                                F("Stop not acknowledged. The local pulse timeout still applies."));
      return;
    }
    if (!uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409, F("An enable pin is still HIGH."));
      return;
    }
    HttpSupport::sendJson(server, 200, uno.json());
  });
  server.on("/setup/wifi", HTTP_POST, [] {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    if (!validSsid(ssid.c_str(), ssid.length()) ||
        !validWifiKey(password.c_str(), password.length())) {
      HttpSupport::sendJson(server, 400,
                            F("{\"error\":\"Use a network name of 1–32 bytes and a blank key, an "
                              "8–63 character WPA key, or a 64 digit hexadecimal key.\"}"));
      return;
    }
    nextSsid = ssid;
    nextPassword = password;
    pendingConnect = true;
    pendingForget = false;
    applyAt = millis() + 750;
    HttpSupport::sendJson(server, 202,
                          F("{\"message\":\"Settings accepted. Connecting shortly; check the "
                            "network status above.\"}"));
  });
  server.on("/setup/wifi/forget", HTTP_POST, [] {
    pendingForget = true;
    pendingConnect = false;
    nextSsid = "";
    nextPassword = "";
    applyAt = millis() + 750;
    HttpSupport::sendJson(
        server, 202,
        F("{\"message\":\"Saved Wi-Fi will be forgotten. The setup network stays available.\"}"));
  });
  // Firmware-only staging validates the complete image before committing it.
  // No authentication, as requested for this local prototype.
  updater.registerRoutes();
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  rosterControl.registerRoutes();
  server.begin();
  server.getServer().setNoDelay(true);
}

void loop() {
  refreshStatusLed();
  server.handleClient();
  rosterControl.loop();
  uno.loop();
  refreshStatusLed();
  // Give the HTTP response time to leave before changing the station link.
  if ((pendingConnect || pendingForget) && int32_t(millis() - applyAt) >= 0) {
    bool forget = pendingForget;
    pendingConnect = pendingForget = false;
    WiFi.persistent(true); // Save only explicit user changes in SDK settings.
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
    nextSsid = "";
    nextPassword = "";
  }
  if (!mdnsReady) {
    static uint32_t retryAt = 0;
    if (int32_t(millis() - retryAt) >= 0) {
      mdnsReady = MDNS.begin(HOST);
      if (mdnsReady)
        MDNS.addService("http", "tcp", 80);
      retryAt = millis() + 10000;
    }
  }
  if (mdnsReady)
    MDNS.update();
  delay(1);
}
