#!/usr/bin/env python3
"""Exercise the production HTTP upload lifecycle without writing device flash."""
from pathlib import Path
import os
import subprocess
import tempfile
from esp_image_fixture import make_image

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / 'firmware/esp13-ota'

with tempfile.TemporaryDirectory(prefix='anden-http-update-') as directory:
    folder = Path(directory)
    (folder / 'image.bin').write_bytes(make_image())
    (folder / 'full.bin').write_bytes(make_image(size=448 * 1024))
    (folder / 'Arduino.h').write_text(r'''
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
using String = std::string;
using std::min;
#define F(x) x
inline void delay(unsigned) {}
struct FakeEsp {
  unsigned restarts = 0;
  uint32_t getFreeSketchSpace() { return 512 * 1024; }
  void restart() { ++restarts; }
};
extern FakeEsp ESP;
''')
    (folder / 'ESP8266WebServer.h').write_text(r'''
#pragma once
#include "Arduino.h"
#include <functional>
enum { HTTP_GET, HTTP_POST };
enum { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload {
  int status = UPLOAD_FILE_START;
  String name;
  uint8_t* buf = nullptr;
  size_t currentSize = 0, totalSize = 0;
};
struct FakeClient { void setNoDelay(bool) {} void stop() {} };
struct ESP8266WebServer {
  std::function<void()> post, receive;
  HTTPUpload file;
  FakeClient connection;
  int code = 0;
  String body;
  void on(const char*, int method, std::function<void()> handler,
          std::function<void()> upload = nullptr) {
    if (method == HTTP_POST) { post = handler; receive = upload; }
  }
  HTTPUpload& upload() { return file; }
  FakeClient& client() { return connection; }
  void keepAlive(bool) {}
  void send(int status, const char*, const String& value) { code = status; body = value; }
};
''')
    (folder / 'Updater.h').write_text(r'''
#pragma once
#include "Arduino.h"
#define U_FLASH 0
struct FakeUpdater {
  bool running = false, failBegin = false, failWrite = false, failEnd = false;
  unsigned commits = 0;
  size_t capacity = 0, written = 0;
  bool begin(size_t size, int command) {
    if (failBegin || running || command != U_FLASH) return false;
    running = true; capacity = size; written = 0; return true;
  }
  size_t write(uint8_t*, size_t size) {
    if (!running || failWrite || written + size > capacity) return 0;
    written += size; return size;
  }
  bool end(bool evenIfRemaining = false) {
    bool ok = running && written && !failEnd && (evenIfRemaining || written == capacity);
    running = false; if (ok) ++commits; return ok;
  }
};
extern FakeUpdater Update;
''')
    (folder / 'test.cpp').write_text(r'''
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include "FirmwareUpdate.h"
#include "HttpSupport.h"
#include "Updater.h"
FakeEsp ESP;
FakeUpdater Update;
namespace HttpSupport {
String jsonString(const String& text) { return "\"" + text + "\""; }
void sendJson(ESP8266WebServer& server, int code, const String& text) {
  server.send(code, "application/json", text);
}
}
int main(int argc, char** argv) {
  assert(argc == 3);
  std::ifstream file(argv[1], std::ios::binary);
  std::vector<uint8_t> valid((std::istreambuf_iterator<char>(file)), {});
  assert(!valid.empty());
  ESP8266WebServer server;
  FirmwareUpdate uploader(server);
  uploader.registerRoutes();
  auto start = [&](const char* name = "firmware") {
    server.file.status = UPLOAD_FILE_START; server.file.name = name; server.receive();
  };
  auto write = [&](std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); i += 71) {
      server.file.status = UPLOAD_FILE_WRITE;
      server.file.buf = data.data() + i;
      server.file.currentSize = std::min(size_t(71), data.size() - i);
      server.receive();
    }
    server.file.status = UPLOAD_FILE_END; server.file.totalSize = data.size(); server.receive();
  };
  auto reset = [&] {
    uploader.beginRequest(FirmwareImage::LIMIT);
    server.code = 0;
  };
  reset(); server.post(); assert(server.code == 400 && !Update.commits);
  reset(); start("filesystem"); write(valid); server.post();
  assert(server.code == 400 && !Update.commits);
  // A valid first part must not be committed before remaining parts are parsed.
  reset(); start(); write(valid); assert(Update.running && !Update.commits);
  start(); write(valid); server.post(); assert(server.code == 400 && !Update.commits);
  reset(); start(); auto corrupt = valid; corrupt.back() ^= 1; write(corrupt); server.post();
  assert(server.code == 400 && !Update.running && !Update.commits);
  reset(); start(); auto truncated = valid; truncated.pop_back(); write(truncated); server.post();
  assert(server.code == 400 && !Update.commits);
  reset(); start(); server.file.status = UPLOAD_FILE_ABORTED; server.receive();
  assert(!Update.running && !Update.commits);
  // Abandoning a completed file before the HTTP completion callback cannot commit.
  reset(); start(); write(valid); reset(); assert(!Update.running && !Update.commits);
  reset(); Update.failBegin = true; start(); write(valid); server.post();
  Update.failBegin = false; assert(server.code == 400 && !Update.commits);
  reset(); start(); Update.failWrite = true; write(valid); server.post();
  Update.failWrite = false; assert(server.code == 400 && !Update.running && !Update.commits);
  reset(); start(); write(valid); Update.failEnd = true; server.post();
  Update.failEnd = false; assert(server.code == 500 && !Update.commits && !ESP.restarts);
  // Full-capacity failures must also discard without accidentally committing.
  std::ifstream fullFile(argv[2], std::ios::binary);
  std::vector<uint8_t> full((std::istreambuf_iterator<char>(fullFile)), {});
  assert(full.size() == FirmwareImage::LIMIT);
  reset(); start(); write(full); assert(!Update.commits);
  start("filesystem"); server.post(); assert(server.code == 400 && !Update.commits);
  full.back() ^= 1; reset(); start(); write(full); server.post();
  assert(server.code == 400 && !Update.running && !Update.commits);
  // A second request can recover after every failure above.
  reset(); start(); write(valid); assert(!Update.commits); server.post();
  assert(server.code == 200 && Update.commits == 1 && ESP.restarts == 1);
}
''')
    executable = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    '-I', str(folder), '-I', str(FIRMWARE), str(folder / 'test.cpp'),
                    str(FIRMWARE / 'FirmwareUpdate.cpp'), str(FIRMWARE / 'FirmwareImage.cpp'),
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable), str(folder / 'image.bin'), str(folder / 'full.bin')], check=True)
print('PASS: production HTTP updater rejects filesystem/multiple/missing/corrupt/truncated/aborted uploads and flash failures; commits and reboots only after a valid complete request.')
