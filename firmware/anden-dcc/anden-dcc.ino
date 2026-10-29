#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>

// Arduino-Pico (Earle Philhower), Raspberry Pi Pico W.
// Select a flash layout with at least 1 MB LittleFS for OTA staging.
// Prototype only: no GPIO is assigned to the motor shield yet.
const char* AP_NAME = "anden-dcc";

WebServer server(80);
HTTPUpdateServer updater;

const char PAGE[] = R"HTML(<!doctype html>
<html lang="es"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>anden-dcc</title>
<style>body{font:18px system-ui;background:#14232b;color:#eef4f5;margin:0;padding:32px}main{max-width:640px;margin:auto}h1{font-size:42px}section{background:#223741;padding:24px;border-radius:16px}a{display:inline-block;background:#ffc567;color:#14232b;padding:12px 18px;border-radius:8px;text-decoration:none}small{color:#bdcdd4}</style>
<main><small>CENTRAL DEL DIORAMA · v0.1.0</small><h1>anden-dcc</h1>
<section><h2>Primera parada: conectados</h2>
<p>Wi-Fi y panel web en marcha.</p>
<p><strong>Vías sin habilitar.</strong> Esta versión no genera DCC ni controla el shield.</p>
<a href="/update">Actualizar firmware</a></section>
<p><small>Actualizaciones: archivo .bin compilado para Pico W con la misma distribución de memoria. No subir un .uf2.</small></p></main></html>)HTML";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 42, 1), IPAddress(192, 168, 42, 1), IPAddress(255, 255, 255, 0));
  if (!WiFi.softAP(AP_NAME)) {
    // No web service if the access point failed; reset to retry.
    while (true) { delay(1000); }
  }
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html; charset=utf-8", PAGE);
  });
  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", "{\"name\":\"anden-dcc\",\"version\":\"0.1.0\",\"dccImplemented\":false,\"trackPower\":false}");
  });
  updater.setup(&server, "/update");
  server.begin();
  Serial.print("anden-dcc: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  if (Serial.available() && Serial.read() == 's') {
    Serial.print("anden-dcc v0.1.0 ready; DCC disabled; http://");
    Serial.println(WiFi.softAPIP());
  }
  delay(1);
}
