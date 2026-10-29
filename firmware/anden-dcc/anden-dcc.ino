#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <LittleFS.h>
#include <SimpleMDNS.h>

// Arduino-Pico 6.3.1, Pico W, flash: 1 MB application + 1 MB LittleFS.
// Prototype: no GPIO is assigned to the motor shield.
const char* AP_NAME = "anden-dcc";
const char* VERSION = "0.2.0";
WebServer server(80);
HTTPUpdateServer updater;
String networkName, networkPassword;
bool storageReady = false, connectPending = false, mdnsReady = false;
bool wasConnected = false;
uint32_t lastAttempt = 0;

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
  Serial.printf("anden-dcc v%s; DCC disabled; AP http://192.168.42.1; ", VERSION);
  Serial.printf("storage=%s; configured=%s; ", storageReady ? "ready" : "error", networkName.length() ? "yes" : "no");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("LAN http://"); Serial.println(WiFi.localIP());
  } else Serial.println("LAN not connected");
}

const char PAGE[] = R"HTML(<!doctype html>
<html lang="es"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>anden-dcc</title>
<style>body{font:18px system-ui;background:#14232b;color:#eef4f5;margin:0;padding:24px}main{max-width:640px;margin:auto}h1{font-size:42px}section{background:#223741;padding:24px;border-radius:16px;margin:16px 0}a,button{display:inline-block;background:#ffc567;color:#14232b;padding:12px 18px;border:0;border-radius:8px;text-decoration:none;font:inherit;cursor:pointer}small{color:#bdcdd4}label{display:block;margin:16px 0}input{display:block;box-sizing:border-box;width:100%;padding:12px;margin-top:8px;font:inherit;border-radius:8px;border:1px solid #bdcdd4}#message{white-space:pre-wrap}a.secondary{background:#bdcdd4}</style>
<main><small>CENTRAL DEL DIORAMA · v0.2.0</small><h1>anden-dcc</h1>
<section><h2>Tu red Wi-Fi</h2><p id="status" role="status">Consultando conexión…</p>
<p id="address"></p><p><small>Compatible con redes de 2,4 GHz. La red abierta anden-dcc permanece disponible en 192.168.42.1.</small></p>
<form id="wifi"><label>Nombre de la red (SSID)<input name="ssid" maxlength="32" required autocomplete="off"></label>
<label>Contraseña de tu red<input name="password" type="password" maxlength="63" autocomplete="new-password"></label>
<button type="submit">Guardar y conectar</button></form>
<p><button id="forget" type="button">Olvidar red</button></p><p id="message" role="status"></p></section>
<section><h2>Central</h2><p><strong>Vías sin habilitar.</strong> Esta versión no genera DCC ni controla el shield.</p>
<a href="/update">Actualizar firmware</a><p><small>Sube un .bin compilado con la misma distribución de memoria.</small></p></section></main>
<script>
const statusLine=document.getElementById('status'),address=document.getElementById('address'),message=document.getElementById('message');
async function refresh(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();statusLine.textContent=s.connected?'Conectado a '+s.ssid:s.ssid?'Intentando conectar a '+s.ssid+'… Revisa los datos si no conecta.':'No hay ninguna red configurada.';address.replaceChildren();if(s.connected){const a=document.createElement('a');a.href='http://'+s.ip+'/';a.textContent='Abrir en tu red: '+s.ip;address.append(a);}if(!s.storageReady)message.textContent='No se pudo abrir la memoria de configuración.';}catch(e){statusLine.textContent='Sin respuesta. Reconecta a anden-dcc o abre la nueva IP desde tu red.';}}
document.getElementById('wifi').onsubmit=async e=>{e.preventDefault();message.textContent='Guardando…';try{const r=await fetch('/wifi/save',{method:'POST',body:new URLSearchParams(new FormData(e.target))});message.textContent=await r.text();if(r.ok)e.target.elements.password.value='';}catch(e){message.textContent='No se ha confirmado el guardado. Comprueba la conexión.';}};
document.getElementById('forget').onclick=async()=>{try{const r=await fetch('/wifi/forget',{method:'POST'});message.textContent=await r.text();await refresh();}catch(e){message.textContent='No se pudo confirmar el borrado.';}};
refresh();setInterval(refresh,3000);
</script></html>)HTML";

void setup() {
  Serial.begin(115200);
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
  server.on("/", HTTP_GET, []() { server.send(200, "text/html; charset=utf-8", PAGE); });
  server.on("/api/status", HTTP_GET, []() {
    bool connected = WiFi.status() == WL_CONNECTED;
    String body = "{\"name\":\"anden-dcc\",\"version\":" + jsonString(VERSION);
    body += ",\"dccImplemented\":false,\"trackPower\":false,\"connected\":";
    body += connected ? "true" : "false";
    body += ",\"storageReady\":"; body += storageReady ? "true" : "false";
    body += ",\"ssid\":" + jsonString(networkName);
    body += ",\"ip\":" + jsonString(connected ? WiFi.localIP().toString() : "");
    body += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", body);
  });
  server.on("/wifi/save", HTTP_POST, []() {
    String ssid = server.arg("ssid"), password = server.arg("password");
    if (!ssid.length() || ssid.length() > 32 || password.length() > 63 ||
        (password.length() && password.length() < 8) ||
        ssid.indexOf('\n') >= 0 || ssid.indexOf('\r') >= 0 ||
        password.indexOf('\n') >= 0 || password.indexOf('\r') >= 0) {
      server.send(400, "text/plain; charset=utf-8", "SSID: 1–32 bytes. Contraseña: 8–63 bytes, o vacía para una red abierta."); return;
    }
    if (!saveNetwork(ssid, password)) {
      server.send(500, "text/plain; charset=utf-8", "No se pudo guardar la configuración."); return;
    }
    networkName = ssid; networkPassword = password; connectPending = true;
    server.send(200, "text/plain; charset=utf-8", "Red guardada. Conectando… La conexión al punto de acceso puede interrumpirse al cambiar de canal.");
  });
  server.on("/wifi/forget", HTTP_POST, []() {
    if (!storageReady || (LittleFS.exists("/wifi.cfg") && !LittleFS.remove("/wifi.cfg"))) {
      server.send(500, "text/plain; charset=utf-8", "No se pudo borrar la configuración."); return;
    }
    networkName = ""; networkPassword = ""; connectPending = false;
    WiFi.disconnect(); WiFi.mode(WIFI_AP_STA);
    server.send(200, "text/plain; charset=utf-8", "Red olvidada. Sigue disponible anden-dcc.");
  });
  updater.setup(&server, "/update");
  server.begin();
  printStatus();
}

void loop() {
  server.handleClient();
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected) {
    if (mdnsReady) { MDNS.end(); mdnsReady = false; }
    if (connected) {
      mdnsReady = MDNS.begin("anden-dcc");
      if (mdnsReady) MDNS.addService("http", "tcp", 80);
    }
    printStatus(); wasConnected = connected;
  }
  if (networkName.length() && (connectPending || (!connected && uint32_t(millis()-lastAttempt) >= 30000))) {
    connectPending = false; lastAttempt = millis();
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginNoBlock(networkName.c_str(), networkPassword.length() ? networkPassword.c_str() : nullptr);
    // Core 6.3.1 changes its mode flag in beginNoBlock; retain AP during retries.
    WiFi.mode(WIFI_AP_STA);
  }
  if (mdnsReady) MDNS.update();
  if (Serial.available() && Serial.read() == 's') printStatus();
  delay(1);
}
