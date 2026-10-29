#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <LittleFS.h>
#include <SimpleMDNS.h>
#include <lwip_wrap.h>
#include "DccSignal.h"
#include "DccPacket.h"
#include "WebAssets.h"

// Arduino-Pico 6.3.1, Pico W, flash: 1 MB application + 1 MB LittleFS.
// Logic-analyzer prototype: GP2 carries DCC; no power-stage pins are assigned.
const char* AP_NAME = "anden-dcc";
const char* VERSION = "0.4.0";
WebServer server(80);
HTTPUpdateServer updater;
String networkName, networkPassword;
bool storageReady = false, connectPending = false;
bool accessPointRunning = false;
bool mdnsReady = false;
bool wasConnected = false;
bool ledOn = true;
uint32_t lastAttempt = 0;
uint32_t mdnsAttempt = 0;
bool webActivity = false;
uint32_t lastWebActivity = 0;
constexpr uint32_t WEB_BLINK_MS = 120;

void signalWebActivity() {
  webActivity = true;
  lastWebActivity = millis();
  // Switch immediately, even if sending the response takes a while.
  ledOn = false;
  digitalWrite(LED_BUILTIN, LOW);
}

void stopMdns() {
  if (!mdnsReady) return;
  LWIPMutex networkLock;
  MDNS.end();
  mdnsReady = false;
}

void startMdns() {
  // Start only with the STA interface active and DHCP complete. Protect the
  // whole interface traversal as well as the individual wrapped lwIP calls.
  LWIPMutex networkLock;
  mdnsReady = MDNS.begin(AP_NAME);
  if (mdnsReady) MDNS.addService("http", "tcp", 80);
}

String jsonString(const String& value) {
  String result = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    unsigned char c = value[i];
    if (c == '"' || c == '\\') { result += '\\'; result += char(c); }
    else if (c < 32) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      result += escaped;
    } else result += char(c);
  }
  return result + "\"";
}

bool saveNetwork(const String& ssid, const String& password) {
  if (!storageReady) return false;
  File file = LittleFS.open("/wifi.tmp", "w");
  if (!file) return false;
  size_t expected = ssid.length() + password.length() + 2;
  size_t written = file.print(ssid + "\n" + password + "\n");
  file.close();
  if (written != expected) { LittleFS.remove("/wifi.tmp"); return false; }
  return LittleFS.rename("/wifi.tmp", "/wifi.cfg");
}

void loadNetwork() {
  if (!storageReady) return;
  File file = LittleFS.open("/wifi.cfg", "r");
  if (!file) return;
  if (file.size() > 98) { file.close(); return; }
  String ssid = file.readStringUntil('\n');
  String password = file.readStringUntil('\n');
  file.close();
  if (ssid.length() > 0 && ssid.length() <= 32 && password.length() <= 63) {
    networkName = ssid;
    networkPassword = password;
    connectPending = true;
  }
}

void printStatus() {
  Serial.printf("anden-dcc v%s; DCC GPIO2; track power disabled; AP=%s; mDNS=%s; ", VERSION,
                accessPointRunning ? "192.168.42.1" : "off", mdnsReady ? "ready" : "off");
  Serial.printf("storage=%s; configured=%s; ", storageReady ? "ready" : "error", networkName.length() ? "yes" : "no");
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP().isSet()) {
    Serial.print("LAN http://"); Serial.println(WiFi.localIP());
  } else Serial.println("LAN not connected");
}

String dccStatus() {
  char bytes[9];
  snprintf(bytes, sizeof(bytes), "%02X %02X %02X", DccSignal::address(),
           DccPacket::instruction(DccSignal::f0()), DccPacket::checksum(DccSignal::address(), DccSignal::f0()));
  String body = "{\"ready\":";
  body += DccSignal::ready() ? "true" : "false";
  body += ",\"running\":"; body += DccSignal::running() ? "true" : "false";
  body += ",\"pin\":2,\"address\":" + String(DccSignal::address());
  body += ",\"f0\":"; body += DccSignal::f0() ? "true" : "false";
  body += ",\"bytes\":" + jsonString(bytes) + "}";
  return body;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  // Do not auto-format: a mount failure must not erase saved credentials.
  LittleFSConfig fsConfig;
  fsConfig.setAutoFormat(false);
  LittleFS.setConfig(fsConfig);
  storageReady = LittleFS.begin();
  // A virgin device has no filesystem. Format only if the reserved region is erased.
  if (!storageReady) {
    extern uint8_t _FS_start;
    const uint8_t* flash = &_FS_start;
    bool erased = true;
    for (size_t i = 0; i < 4096; ++i) if (flash[i] != 0xff) { erased = false; break; }
    if (erased && LittleFS.format()) storageReady = LittleFS.begin();
  }
  loadNetwork();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192,168,42,1), IPAddress(192,168,42,1), IPAddress(255,255,255,0));
  if (!WiFi.softAP(AP_NAME)) while (true) delay(1000);
  accessPointRunning = true;
  DccSignal::begin();
  server.addHook([](const String& method, const String& url, WiFiClient*, HTTPServer::ContentTypeFunction) {
    // Background status refreshes should not look like user activity.
    bool statusPoll = method == "GET" && (url == "/api/status" || url.startsWith("/api/status?"));
    if (!statusPoll) signalWebActivity();
    if (method == "POST" && url == "/update") DccSignal::stop();
    return HTTPServer::CLIENT_REQUEST_CAN_CONTINUE;
  });
  server.on("/", HTTP_GET, []() { server.send(200, "text/html; charset=utf-8", PAGE); });
  server.on("/icon.svg", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.send(200, "image/svg+xml", ICON_SVG);
  });
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Location", "/?setup=firmware");
    server.send(303, "text/plain", "Open Setup to update firmware.");
  });
  server.on("/api/status", HTTP_GET, []() {
    bool connected = WiFi.status() == WL_CONNECTED && WiFi.localIP().isSet();
    String body = "{\"name\":\"anden-dcc\",\"version\":" + jsonString(VERSION);
    body += ",\"dccImplemented\":true,\"trackPower\":false,\"dcc\":" + dccStatus();
    body += ",\"connected\":";
    body += connected ? "true" : "false";
    body += ",\"mdnsReady\":"; body += mdnsReady ? "true" : "false";
    body += ",\"storageReady\":"; body += storageReady ? "true" : "false";
    body += ",\"ssid\":" + jsonString(networkName);
    body += ",\"ip\":" + jsonString(connected ? WiFi.localIP().toString() : "");
    body += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", body);
  });
  server.on("/dcc/f0", HTTP_POST, []() {
    String target = server.arg("address"), value = server.arg("on");
    bool valid = target.length() > 0 && target.length() <= 3;
    for (size_t i = 0; i < target.length(); ++i) valid &= target[i] >= '0' && target[i] <= '9';
    int address = target.toInt();
    if (!valid || address < 1 || address > 127 || (value != "0" && value != "1")) {
      server.send(400, "text/plain; charset=utf-8", "Address must be 1–127. F0 must be 0 or 1."); return;
    }
    if (!DccSignal::setF0(address, value == "1")) {
      server.send(503, "text/plain; charset=utf-8", "Signal generator unavailable or queue full. Try again."); return;
    }
    server.send(200, "application/json", dccStatus());
  });
  server.on("/wifi/save", HTTP_POST, []() {
    String ssid = server.arg("ssid"), password = server.arg("password");
    if (!ssid.length() || ssid.length() > 32 || password.length() > 63 ||
        (password.length() && password.length() < 8) ||
        ssid.indexOf('\n') >= 0 || ssid.indexOf('\r') >= 0 ||
        password.indexOf('\n') >= 0 || password.indexOf('\r') >= 0) {
      server.send(400, "text/plain; charset=utf-8", "SSID must be 1–32 bytes. Password must be 8–63 bytes, or empty for an open network."); return;
    }
    if (!saveNetwork(ssid, password)) {
      server.send(500, "text/plain; charset=utf-8", "Could not save the network settings."); return;
    }
    networkName = ssid; networkPassword = password; connectPending = true;
    server.send(200, "text/plain; charset=utf-8", "Network saved. Connecting. When anden-dcc disappears, open the assigned IP from your network.");
  });
  server.on("/wifi/forget", HTTP_POST, []() {
    if (!storageReady || (LittleFS.exists("/wifi.cfg") && !LittleFS.remove("/wifi.cfg"))) {
      server.send(500, "text/plain; charset=utf-8", "Could not erase the network settings."); return;
    }
    networkName = ""; networkPassword = ""; connectPending = false;
    stopMdns();
    WiFi.disconnect(); WiFi.mode(WIFI_AP_STA);
    if (!accessPointRunning) accessPointRunning = WiFi.softAP(AP_NAME);
    server.send(200, "text/plain; charset=utf-8", "Network forgotten. Connect to anden-dcc to configure it again.");
  });
  updater.setup(&server, "/update");
  server.begin();
  printStatus();
}

void loop() {
  server.handleClient();
  bool connected = WiFi.status() == WL_CONNECTED && WiFi.localIP().isSet();
  // Web requests briefly interrupt the steady connected LED. The normal
  // disconnected heartbeat resumes after the same non-blocking activity pulse.
  if (webActivity && uint32_t(millis() - lastWebActivity) >= WEB_BLINK_MS) webActivity = false;
  bool desiredLed = !webActivity && (connected || ((millis() / 500) % 2 == 0));
  if (desiredLed != ledOn) {
    ledOn = desiredLed;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
  }
  if (connected != wasConnected) {
    stopMdns();
    mdnsAttempt = millis();
    // Use the radio exclusively for the router once DHCP has completed.
    if (connected && accessPointRunning) {
      WiFi.disconnectAP();
      WiFi.mode(WIFI_STA);
      accessPointRunning = false;
    }
    printStatus(); wasConnected = connected;
  }
  if (networkName.length() && (connectPending || (!connected && uint32_t(millis()-lastAttempt) >= 30000))) {
    stopMdns();
    connectPending = false; lastAttempt = millis();
    if (!accessPointRunning) {
      WiFi.mode(WIFI_AP_STA);
      accessPointRunning = WiFi.softAP(AP_NAME);
    }
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginNoBlock(networkName.c_str(), networkPassword.length() ? networkPassword.c_str() : nullptr);
    // Core 6.3.1 changes its mode flag in beginNoBlock; retain AP during retries.
    WiFi.mode(WIFI_AP_STA);
  }
  if (connected && !connectPending && !accessPointRunning && !mdnsReady &&
      uint32_t(millis() - mdnsAttempt) >= 1000) {
    mdnsAttempt = millis();
    startMdns();
    printStatus();
  }
  if (mdnsReady) MDNS.update();
  if (Serial.available() && Serial.read() == 's') printStatus();
  delay(1);
}
