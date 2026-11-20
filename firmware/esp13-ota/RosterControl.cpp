#include "RosterControl.h"
#include "HttpSupport.h"
#include "RosterStorage.h"
#include "UnoLink.h"
#include <DccFunctions.h>
#include <LittleFS.h>
#include <memory>
#include <new>

using HttpSupport::jsonString;

RosterControl::RosterControl(ESP8266WebServer& webServer, UnoLink& unoLink)
    : server(webServer), uno(unoLink) {
}
void RosterControl::begin() {
  // Allow several full-range sweeps of F0 and zero-speed packets to settle.
  finder.settleMs = 10000;
  loadRoster();
  uno.attach(roster);
}
void RosterControl::registerRoutes() {
  rosterRoutes();
  favoriteRoutes();
  trackRoutes();
  finderRoutes();
}
void RosterControl::loop() {
  serviceFinder();
}
bool RosterControl::storageReady() const {
  return rosterStorage;
}
const char* RosterControl::stopReason() const {
  return trackStopReason;
}
bool RosterControl::setupLocked() const {
  return finder.active() || uno.benchActive() || uno.trackRequested();
}
const LocoRoster& RosterControl::configuration() const {
  return roster;
}

String RosterControl::finderStatus() {
  return "{\"stage\":" + jsonString(finder.name()) +
         ",\"active\":" + (finder.active() ? "true" : "false") +
         ",\"step\":" + String(finder.step) + ",\"first\":" + String(finder.first) +
         ",\"last\":" + String(finder.last) + ",\"low\":" + String(finder.low) +
         ",\"high\":" + String(finder.high) + ",\"middle\":" + String(finder.middle()) +
         ",\"result\":" + String(finder.result) + ",\"waitMs\":" + String(finder.wait(millis())) +
         "}";
}
// Keep addressing the original range. The lit subset changes per question,
// and a zero lower bound tells the Uno to switch all search lights off.
bool RosterControl::applyFinder(const AddressFinder& value) {
  unsigned low = 0, high = 0;
  if (value.stage == AddressFinder::Stage::VerifyOn) {
    low = value.first;
    high = value.last;
  } else if (value.stage == AddressFinder::Stage::Split) {
    low = value.low;
    high = value.middle();
  } else if (value.stage == AddressFinder::Stage::ConfirmOn)
    low = high = value.low;
  String command = "FIND," + String(value.first) + "," + String(value.last) + "," + String(low) +
                   "," + String(high);
  if (!uno.query(command.c_str()))
    return false;
  char spec[] = {char(value.first), char(value.last), char(low), char(high)};
  // The checked query acknowledges only after the new packet bank is active.
  return uno.statusSignature() == AndenLink::crc16(spec, 4);
}
void RosterControl::serviceFinder() {
  if (!finder.active())
    return;
  if (!uno.trackActive() && finder.stage != AddressFinder::Stage::Cleanup)
    finder.cancel(millis());
  if (finder.tick(millis()) && !applyFinder(finder)) {
    finder.cancel(millis());
  }
  if (!finder.active()) {
    // Cleanup finished. Restore the saved roster and resume background sync.
    uno.holdRoster(false);
    uno.invalidate();
    uno.syncRoster(roster);
  }
}
int RosterControl::rosterNumber(const String& text, int maximum) {
  uint16_t value;
  if (!AndenLink::number(text.c_str(), value) || !value || value > maximum)
    return -1;
  return value;
}
void RosterControl::loadRoster() {
  roster.save(0, 3, "", "electric");
  roster.save(0, 4, "", "diesel");
  rosterStorage = mountRosterStorage();
  if (!rosterStorage || !LittleFS.exists("/locos.cfg"))
    return;
  File file = LittleFS.open("/locos.cfg", "r");
  if (!file || file.size() > 8192) {
    rosterStorage = false;
    return;
  }
  String header = file.readStringUntil('\n');
  const bool hasPresence = header == "anden-roster-4";
  const bool current = hasPresence || header == "anden-roster-3";
  if (!current && header != "anden-roster-2") {
    rosterStorage = false;
    return;
  }
  // Parse into temporary models. A bad record keeps the defaults and disables
  // saves so the existing file remains available for recovery.
  LocoRoster loaded;
  std::unique_ptr<FavoriteFunctions> loadedFavorites(new (std::nothrow) FavoriteFunctions);
  if (!loadedFavorites) {
    rosterStorage = false;
    return;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    bool ok = false;
    if (!current)
      ok = loaded.loadRecord(line.c_str(), false);
    else if (line.startsWith("L\t"))
      ok = loaded.loadRecord(line.c_str() + 2, false, hasPresence);
    else if (line.startsWith("F\t"))
      ok = loadedFavorites->loadRecord(line.c_str() + 2);
    if (!ok) {
      rosterStorage = false;
      return;
    }
  }
  for (uint8_t id = 1; id <= 8; ++id)
    if (loadedFavorites->locos[id - 1].count && loaded.findId(id) < 0) {
      rosterStorage = false;
      return;
    }
  roster = loaded;
  favorites = *loadedFavorites;
}
bool RosterControl::saveRoster(const LocoRoster& value, const FavoriteFunctions* changed) {
  if (!rosterStorage)
    return false;
  const auto& savedFavorites = changed ? *changed : favorites;
  // Write both models together. Replace the saved file only after a full write.
  File file = LittleFS.open("/locos.tmp", "w");
  if (!file)
    return false;
  String data = "anden-roster-4\n";
  for (uint8_t i = 0; i < value.count; ++i) {
    const auto& l = value.entries[i];
    data += "L\t" + String(l.id) + "\t" + String(l.address) + "\t" + LocoRoster::typeName(l.type) +
            "\t" + String(l.onDiorama ? 1 : 0) + "\t" + l.name + "\n";
  }
  for (uint8_t i = 0; i < value.count; ++i) {
    uint8_t id = value.entries[i].id;
    const auto& set = savedFavorites.locos[id - 1];
    for (uint8_t j = 0; j < set.count; ++j)
      data += "F\t" + String(id) + "\t" + String(set.entries[j].number) + "\t" +
              set.entries[j].name + "\n";
  }
  bool ok = file.print(data) == data.length();
  file.close();
  if (ok)
    ok = LittleFS.rename("/locos.tmp", "/locos.cfg");
  if (!ok)
    LittleFS.remove("/locos.tmp");
  return ok;
}
LocoRoster RosterControl::runtimeRoster() const {
  // Keep the configured snapshot intact for signature matching. Status and
  // explicit edits use a copy with the Uno's actual encoder direction.
  LocoRoster current = roster;
  uint8_t directions;
  if (uno.readDirections(roster, directions))
    for (uint8_t i = 0; i < current.count; ++i)
      current.entries[i].forward = directions & (1 << i);
  return current;
}

String RosterControl::dccStatus() {
  const LocoRoster current = runtimeRoster();
  String out = "{\"ready\":";
  out += uno.matches(roster) ? "true" : "false";
  out += ",\"functionsSupported\":";
  out += uno.functionsSupported() ? "true" : "false";
  out += ",\"cabFilterSupported\":";
  out += uno.cabFilterSupported() ? "true" : "false";
  out += ",\"maxFunction\":28,\"maxFavorites\":8";
  out += ",\"running\":";
  out += uno.dccReady() ? "true" : "false";
  out += ",\"pin\":10,\"bridgePin\":12,\"halfOneUs\":58,\"halfZeroUs\":100,\"maxLocos\":8,"
         "\"trackPower\":";
  out += uno.dccPowerActive() ? "true" : "false";
  out += ",\"packets\":" + String(uno.packetCount());
  out += ",\"timingFaults\":" + String(uno.faultCount()) + ",\"locos\":[";
  for (uint8_t i = 0; i < roster.count; ++i) {
    if (i)
      out += ',';
    const auto& l = current.entries[i];
    out += "{\"id\":" + String(l.id) + ",\"address\":" + String(l.address) +
           ",\"name\":" + jsonString(l.name) +
           ",\"type\":" + jsonString(LocoRoster::typeName(l.type));
    out += ",\"onDiorama\":";
    out += l.onDiorama ? "true" : "false";
    out += ",\"forward\":";
    out += l.forward ? "true" : "false";
    out += ",\"speed\":0";
    out += ",\"f0\":";
    out += l.f0 ? "true" : "false";
    uint32_t flags = LocoRoster::functionMask(l);
    out += ",\"functions\":" + String(flags) + ",\"favorites\":[";
    const auto& set = favorites.locos[l.id - 1];
    for (uint8_t j = 0; j < set.count; ++j) {
      if (j)
        out += ',';
      out += "{\"number\":" + String(set.entries[j].number) +
             ",\"name\":" + jsonString(set.entries[j].name) + "}";
    }
    out += ']';
    uint8_t instruction = DccFunctions::groupOne(flags);
    char bytes[32];
    snprintf(bytes, sizeof(bytes), ",\"bytes\":[\"%02X\",\"%02X\",\"%02X\"]}", l.address,
             instruction, l.address ^ instruction);
    out += bytes;
  }
  return out + "],\"finder\":" + finderStatus() + "}";
}
void RosterControl::trackRoutes() {
  server.on("/api/track/on", HTTP_POST, [this] {
    // An explicit ON is idempotent, including the local recovery pause.
    if (uno.query("PING") && uno.trackRequested()) {
      HttpSupport::sendJson(server, 200, F("{\"ok\":true}"));
      return;
    }
    if (finder.active() || !uno.online() || !uno.trackSupported() || !uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Track firmware and a connected, stopped roster are required."));
      return;
    }
    if (!uno.dccReady() && (!uno.query("DCCRESET") || !uno.dccReady())) {
      HttpSupport::sendUnoError(server, uno, 409, F("DCC timer could not restart. Check the controller."));
      return;
    }
    roster = runtimeRoster();
    if (!uno.syncRoster(roster) || !uno.query("TRACKON") || !uno.trackRequested()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Track start refused. Check current cutoff and timing status."));
      return;
    }
    trackStopReason = "Track A output is disabled.";
    HttpSupport::sendJson(server, 200, F("{\"ok\":true}"));
  });
  // Compatibility with old tabs: this endpoint is read-only, never a lease.
  server.on("/api/track/keep", HTTP_POST, [this] {
    const bool ok = uno.query("STATUS");
    HttpSupport::sendJson(server, ok ? 200 : 503,
                         ok ? F("{\"ok\":true}") : F("{\"error\":\"Uno unavailable.\"}"));
  });
  server.on("/api/track/off", HTTP_POST, [this] {
    // Old firmware pages send a lease-bearing beacon on reload. Reject it so
    // an old tab cannot silently undo the user's latched power selection.
    if (server.hasArg("lease")) {
      HttpSupport::sendJson(server, 409, F("{\"error\":\"Reload this page to use manual power control.\"}"));
      return;
    }
    trackStopReason = "Power off requested.";
    bool ok = uno.query("BENCHSTOP") && uno.outputsDisabled();
    HttpSupport::sendJson(
        server, ok ? 200 : 503,
        ok ? F("{\"ok\":true}")
           : F("{\"error\":\"Power off not acknowledged. Check the controller or use the cab button.\"}"));
  });
}
void RosterControl::finderRoutes() {
  server.on("/finder/start", HTTP_POST, [this] {
    if (finder.active() || !uno.query("PING") || !uno.trackActive()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Power on for the stationary test before starting a search."));
      return;
    }
    int first = rosterNumber(server.arg("first"), 127),
        last = rosterNumber(server.arg("last"), 127);
    if (first < 0 || last < first) {
      HttpSupport::sendJson(
          server, 400,
          F("{\"error\":\"Use an ordered range from 1 to 127. Address 0 is broadcast.\"}"));
      return;
    }
    AddressFinder changed = finder;
    if (!changed.start(first, last, millis())) {
      HttpSupport::sendUnoError(server, uno, 409, F("Search unavailable."));
      return;
    }
    roster = runtimeRoster();
    // Suppress automatic roster sync until the search and its cleanup finish.
    uno.holdRoster(true);
    if (!applyFinder(changed)) {
      uno.holdRoster(false);
      HttpSupport::sendUnoError(server, uno, 503,
                                F("Search schedule was not acknowledged. Refresh status before trying again."));
      return;
    }
    finder = changed;
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  server.on("/finder/answer", HTTP_POST, [this] {
    serviceFinder();
    String answer = server.arg("answer");
    if (server.arg("step") != String(finder.step) ||
        (answer != "on" && answer != "off" && answer != "retry")) {
      HttpSupport::sendUnoError(server, uno, 409, F("Refresh the current search question."));
      return;
    }
    AddressFinder changed = finder;
    bool ok = answer == "retry" ? changed.retry(finder.step, millis())
                                : changed.answer(answer == "on", finder.step, millis());
    if (!ok) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Wait for the lights to settle before answering."));
      return;
    }
    if (!applyFinder(changed)) {
      HttpSupport::sendUnoError(server, uno, 503, F("Search update was not acknowledged. Refresh status before trying again."));
      return;
    }
    finder = changed;
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  server.on("/finder/cancel", HTTP_POST, [this] {
    if (server.arg("step") != String(finder.step)) {
      HttpSupport::sendUnoError(server, uno, 409, F("Refresh the current search first."));
      return;
    }
    if (finder.active()) {
      if (finder.stage != AddressFinder::Stage::Cleanup) {
        finder.cancel(millis());
        applyFinder(finder); // A lost reply must not change the power selection.
      }
    } else
      finder.dismiss();
    HttpSupport::sendJson(server, 200, dccStatus());
  });
}
void RosterControl::favoriteRoutes() {
  server.on("/dcc/function", HTTP_POST, [this] {
    if (finder.active()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Finish or cancel the address search first."));
      return;
    }
    int address = rosterNumber(server.arg("address"), 127);
    uint16_t fn;
    String on = server.arg("on");
    if (address < 0 || !AndenLink::number(server.arg("function").c_str(), fn) || fn > 28 ||
        (on != "0" && on != "1")) {
      HttpSupport::sendUnoError(server, uno, 400, F("Choose F0 to F28 and On or Off."));
      return;
    }
    if (fn && !uno.functionsSupported()) {
      HttpSupport::sendUnoError(server, uno, 501, F("Install Uno 0.6.0 to test F1-F28."));
      return;
    }
    int index = roster.findAddress(address);
    if (index < 0) {
      HttpSupport::sendUnoError(server, uno, 404, F("Locomotive not found."));
      return;
    }
    LocoRoster changed = runtimeRoster();
    LocoRoster::setFunction(changed.entries[index], fn, on == "1");
    if (!uno.syncRoster(changed)) {
      HttpSupport::sendUnoError(server, uno, 503, F("Uno did not acknowledge the function."));
      return;
    }
    roster = changed;
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  auto favoriteAction = [this](bool remove) {
    if (finder.active()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Finish or cancel the address search first."));
      return;
    }
    int id = rosterNumber(server.arg("id"), 8);
    uint16_t fn;
    if (id < 0 || roster.findId(id) < 0 || !AndenLink::number(server.arg("function").c_str(), fn) ||
        fn > 28) {
      HttpSupport::sendUnoError(server, uno, 400, F("Invalid locomotive or function."));
      return;
    }
    std::unique_ptr<FavoriteFunctions> changed(new (std::nothrow) FavoriteFunctions(favorites));
    if (!changed) {
      HttpSupport::sendUnoError(server, uno, 500, F("Not enough memory to save favorites."));
      return;
    }
    String name = server.arg("name");
    name.trim();
    bool ok = remove ? changed->remove(id, fn) : changed->save(id, fn, name.c_str());
    if (!ok) {
      HttpSupport::sendUnoError(
          server, uno, 400,
          F("Use a name of 1-24 UTF-8 bytes, and at most eight favorites per loco."));
      return;
    }
    if (!saveRoster(roster, changed.get())) {
      HttpSupport::sendUnoError(server, uno, 500, F("Could not save favorite functions."));
      return;
    }
    favorites = *changed;
    HttpSupport::sendJson(server, 200, dccStatus());
  };
  server.on("/locos/favorite", HTTP_POST, [favoriteAction] { favoriteAction(false); });
  server.on("/locos/favorite/remove", HTTP_POST, [favoriteAction] { favoriteAction(true); });
}
void RosterControl::rosterRoutes() {
  server.on("/dcc/direction", HTTP_POST, [this] {
    if (finder.active()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Finish or cancel the address search first."));
      return;
    }
    int address = rosterNumber(server.arg("address"), 127);
    String forward = server.arg("forward");
    if (!uno.trackSupported()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Install the new Uno firmware first."));
      return;
    }
    int index = address < 0 ? -1 : roster.findAddress(address);
    if (index < 0 || (forward != "0" && forward != "1")) {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"Invalid locomotive or direction\"}"));
      return;
    }
    LocoRoster changed = runtimeRoster();
    changed.entries[index].forward = forward == "1";
    if (!uno.syncRoster(changed)) {
      HttpSupport::sendUnoError(server, uno, 503, F("Uno did not acknowledge direction."));
      return;
    }
    roster = changed;
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  server.on("/dcc/f0", HTTP_POST, [this] {
    if (finder.active()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Finish or cancel the address search first."));
      return;
    }
    int address = rosterNumber(server.arg("address"), 127);
    String on = server.arg("on");
    if (address < 0 || (on != "0" && on != "1")) {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"Invalid address or F0 state\"}"));
      return;
    }
    int index = roster.findAddress(address);
    if (index < 0) {
      HttpSupport::sendJson(server, 404, F("{\"error\":\"Loco not in roster\"}"));
      return;
    }
    LocoRoster changed = runtimeRoster();
    changed.entries[index].f0 = on == "1";
    if (!uno.syncRoster(changed)) {
      HttpSupport::sendJson(server, 503,
                            F("{\"error\":\"Uno did not acknowledge the DCC roster\"}"));
      return;
    }
    roster = changed;
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  server.on("/locos/presence", HTTP_POST, [this] {
    if (finder.active() || !uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Finish the search and turn track power off before changing the track selection."));
      return;
    }
    const int id = rosterNumber(server.arg("id"), 8);
    const int index = id > 0 ? roster.findId(id) : -1;
    const String presence = server.arg("onDiorama");
    if (index < 0 || (presence != "0" && presence != "1")) {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"Choose an existing loco and onDiorama 0 or 1.\"}"));
      return;
    }
    // Change only presence; a stale browser cannot overwrite a renamed loco.
    LocoRoster changed = runtimeRoster();
    changed.entries[index].onDiorama = presence == "1";
    if (!saveRoster(changed)) {
      HttpSupport::sendJson(server, 500, F("{\"error\":\"Could not save roster\"}"));
      return;
    }
    roster = changed;
    uno.invalidate();
    uno.syncRoster(roster);
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  server.on("/locos/save", HTTP_POST, [this] {
    if (finder.active()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Finish or cancel the address search first."));
      return;
    }
    if (!uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Turn track power off before editing the roster."));
      return;
    }
    int id = server.arg("id").length() ? rosterNumber(server.arg("id"), 8) : 0;
    int address = rosterNumber(server.arg("address"), 127);
    String name = server.arg("name"), type = server.arg("type");
    name.trim();
    const bool setPresence = server.hasArg("onDiorama");
    const String presence = server.arg("onDiorama");
    if (setPresence && presence != "0" && presence != "1") {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"On track must be 0 or 1.\"}"));
      return;
    }
    if (id < 0 || address < 0) {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"Invalid ID or address\"}"));
      return;
    }
    LocoRoster changed = runtimeRoster();
    auto result =
        changed.save(id, address, name.c_str(), server.hasArg("type") ? type.c_str() : nullptr);
    if (result != LocoRoster::Result::Ok) {
      HttpSupport::sendJson(
          server,
          result == LocoRoster::Result::Invalid   ? 400
          : result == LocoRoster::Result::Missing ? 404
                                                  : 409,
          F("{\"error\":\"Invalid name/type, duplicate address or roster full\"}"));
      return;
    }
    if (setPresence)
      changed.entries[changed.findAddress(address)].onDiorama = presence == "1";
    if (!saveRoster(changed)) {
      HttpSupport::sendJson(server, 500, F("{\"error\":\"Could not save roster\"}"));
      return;
    }
    roster = changed;
    uno.invalidate();
    uno.syncRoster(roster);
    HttpSupport::sendJson(server, 200, dccStatus());
  });
  server.on("/locos/remove", HTTP_POST, [this] {
    if (finder.active()) {
      HttpSupport::sendUnoError(server, uno, 409, F("Finish or cancel the address search first."));
      return;
    }
    if (!uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Turn track power off before editing the roster."));
      return;
    }
    int id = rosterNumber(server.arg("id"), 8);
    LocoRoster changed = runtimeRoster();
    if (id < 0) {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"Invalid ID\"}"));
      return;
    }
    if (!changed.remove(id)) {
      HttpSupport::sendJson(server, 404, F("{\"error\":\"Loco not found\"}"));
      return;
    }
    std::unique_ptr<FavoriteFunctions> changedFavorites(new (std::nothrow)
                                                            FavoriteFunctions(favorites));
    if (!changedFavorites) {
      HttpSupport::sendUnoError(server, uno, 500, F("Not enough memory to save roster."));
      return;
    }
    changedFavorites->clear(id);
    if (!saveRoster(changed, changedFavorites.get())) {
      HttpSupport::sendJson(server, 500, F("{\"error\":\"Could not save roster\"}"));
      return;
    }
    favorites = *changedFavorites;
    roster = changed;
    uno.invalidate();
    uno.syncRoster(roster);
    HttpSupport::sendJson(server, 200, dccStatus());
  });
}
