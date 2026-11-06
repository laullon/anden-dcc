#include "FirmwareUpdate.h"
#include "HttpSupport.h"
#include <Arduino.h>
#include <Updater.h>

FirmwareUpdate::FirmwareUpdate(ESP8266WebServer& webServer) : server(webServer) {
}

void FirmwareUpdate::registerRoutes() {
  server.on("/update", HTTP_GET, [this] {
    server.keepAlive(false);
    server.send(200, "text/html",
                F("<!doctype html><html lang=en><meta charset=utf-8>"
                  "<title>anden-dcc update</title>"
                  "<form method=post enctype=multipart/form-data>"
                  "<input type=file name=firmware accept=.bin required>"
                  "<button>Update firmware</button></form></html>"));
  });
  server.on("/update", HTTP_POST, [this] { respond(); }, [this] { upload(); });
}

void FirmwareUpdate::beginRequest(size_t capacity) {
  discard();
  failure = nullptr;
  seenFile = complete = false;
  image.begin(capacity);
}

void FirmwareUpdate::discard() {
  if (staging) {
    // One byte is always withheld until respond(). Even a capacity-sized upload
    // remains incomplete, so end(false) resets staging without an eboot command.
    Update.end(false);
    staging = false;
  }
  hasPending = false;
}

void FirmwareUpdate::fail(const char* reason) {
  if (!failure)
    failure = reason;
  discard();
}

void FirmwareUpdate::upload() {
  HTTPUpload& file = server.upload();
  if (file.status == UPLOAD_FILE_START) {
    if (failure)
      return;
    if (seenFile || file.name != "firmware") {
      fail("Upload exactly one firmware file; filesystem uploads are unsupported.");
      return;
    }
    seenFile = true;
    // The validator enforces the public capacity; use the same bound for staging.
    uint32_t free = ESP.getFreeSketchSpace();
    uint32_t capacity = free > 0x1000 ? (free - 0x1000) & 0xFFFFF000 : 0;
    capacity = min(capacity, uint32_t(FirmwareImage::LIMIT));
    if (!capacity || !Update.begin(capacity, U_FLASH)) {
      fail("Could not start firmware staging.");
      return;
    }
    staging = true;
  } else if (file.status == UPLOAD_FILE_WRITE && !failure) {
    if (!staging || complete || !image.append(file.buf, file.currentSize)) {
      fail(image.error() ? image.error() : "Unexpected firmware upload data.");
      return;
    }
    if (!file.currentSize)
      return;
    if ((hasPending && Update.write(&pending, 1) != 1) ||
        (file.currentSize > 1 &&
         Update.write(file.buf, file.currentSize - 1) != file.currentSize - 1)) {
      fail("Firmware staging write failed.");
      return;
    }
    pending = file.buf[file.currentSize - 1];
    hasPending = true;
  } else if (file.status == UPLOAD_FILE_END && !failure) {
    if (!staging || complete || file.totalSize != image.size() || !image.finish()) {
      fail(image.error() ? image.error() : "Incomplete firmware upload.");
      return;
    }
    complete = true;
  } else if (file.status == UPLOAD_FILE_ABORTED) {
    fail("Firmware upload interrupted; the running firmware was kept.");
  }
  delay(0);
}

void FirmwareUpdate::respond() {
  if (!failure && (!seenFile || !complete || !hasPending || !staging))
    fail("Upload one complete firmware file.");
  if (failure) {
    HttpSupport::sendJson(server, 400, "{\"error\":" + HttpSupport::jsonString(failure) + "}");
    return;
  }
  // No further multipart fields/files can arrive after the completion callback.
  // This is the only path which may commit the staged image and request a reboot.
  bool ok = Update.write(&pending, 1) == 1 && Update.end(true);
  hasPending = false;
  if (!ok) {
    fail("Firmware validation or flash write failed; update not committed.");
    HttpSupport::sendJson(server, 500, F("{\"error\":\"Firmware update failed.\"}"));
    return;
  }
  staging = false;
  server.client().setNoDelay(true);
  server.keepAlive(false);
  server.send(200, "text/plain", "Update Success! Rebooting...");
  delay(100);
  server.client().stop();
  ESP.restart();
}
