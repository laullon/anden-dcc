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
  serviceTrackLease();
  serviceFinder();
}
bool RosterControl::storageReady() const {
  return rosterStorage;
}
const char* RosterControl::stopReason() const {
  return trackStopReason;
}
bool RosterControl::setupLocked() const {
  return finder.active() || uno.benchActive();
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
    uno.query("BENCHSTOP");
    finder.cancel(millis());
  }
  if (!finder.active()) {
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
  bool current = header == "anden-roster-3";
  if (!current && header != "anden-roster-2") {
    rosterStorage = false;
    return;
  }
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
      ok = loaded.loadRecord(line.c_str() + 2, false);
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
  File file = LittleFS.open("/locos.tmp", "w");
  if (!file)
    return false;
  String data = "anden-roster-3\n";
  for (uint8_t i = 0; i < value.count; ++i) {
    const auto& l = value.entries[i];
    data += "L\t" + String(l.id) + "\t" + String(l.address) + "\t" + LocoRoster::typeName(l.type) +
            "\t" + l.name + "\n";
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
String RosterControl::dccStatus() {
  String out = "{\"ready\":";
  out += uno.matches(roster) ? "true" : "false";
  out += ",\"functionsSupported\":";
  out += uno.functionsSupported() ? "true" : "false";
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
    const auto& l = roster.entries[i];
    out += "{\"id\":" + String(l.id) + ",\"address\":" + String(l.address) +
           ",\"name\":" + jsonString(l.name) +
           ",\"type\":" + jsonString(LocoRoster::typeName(l.type));
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
void RosterControl::serviceTrackLease() {
  if (!browserLease.active())
    return;
  if (browserLease.expired(millis())) {
    browserLease.stop();
    trackStopReason = "Browser connection lost (6 seconds).";
    uno.query("BENCHSTOP");
    return;
  }
  if (browserLease.due(millis())) {
    if (!uno.query("KEEP") || !uno.trackActive()) {
      browserLease.stop();
      trackStopReason = "Uno link or output stopped.";
      uno.query("BENCHSTOP");
      return;
    }
    browserLease.renewed(millis());
  }
}
void RosterControl::trackRoutes() {
  server.on("/api/track/on", HTTP_POST, [this] {
    if (finder.active() || !uno.query("PING") || !uno.trackSupported() || !uno.outputsDisabled()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Track firmware and a connected, stopped roster are required."));
      return;
    }
    if (!uno.syncRoster(roster) || !uno.query("TRACKON") || !uno.trackActive()) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Track start refused. Check current cutoff and timing status."));
      return;
    }
    uint16_t trackLease = browserLease.start(millis());
    trackStopReason = "Track A output is disabled.";
    HttpSupport::sendJson(server, 200, "{\"lease\":" + String(trackLease) + "}");
  });
  server.on("/api/track/keep", HTTP_POST, [this] {
    uint16_t lease;
    if (!AndenLink::number(server.arg("lease").c_str(), lease) ||
        !browserLease.accepts(lease, millis())) {
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Track session expired. Turn power on explicitly."));
      return;
    }
    if (!uno.query("KEEP") || !uno.trackActive()) {
      browserLease.stop();
      trackStopReason = "Uno link or output stopped.";
      HttpSupport::sendUnoError(server, uno, 409,
                                F("Track stopped. Check status before restarting."));
      return;
    }
    browserLease.touch(lease, millis());
    browserLease.renewed(millis());
    HttpSupport::sendJson(server, 200, F("{\"ok\":true}"));
  });
  server.on("/api/track/off", HTTP_POST, [this] {
    if (server.hasArg("lease")) {
      uint16_t lease;
      if (!AndenLink::number(server.arg("lease").c_str(), lease) ||
          !browserLease.accepts(lease, millis())) {
        HttpSupport::sendUnoError(server, uno, 409,
                                  F("This page no longer owns the track session."));
        return;
      }
    }
    browserLease.stop();
    trackStopReason =
        server.hasArg("lease") ? "Control page closed or reloaded." : "Power off requested.";
    bool ok = uno.query("BENCHSTOP") && uno.outputsDisabled();
    HttpSupport::sendJson(
        server, ok ? 200 : 503,
        ok ? F("{\"ok\":true}")
           : F("{\"error\":\"Stop not acknowledged; the local timeout still applies.\"}"));
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
    uno.holdRoster(true);
    if (!applyFinder(changed)) {
      uno.holdRoster(false);
      uno.query("BENCHSTOP");
      HttpSupport::sendUnoError(server, uno, 503,
                                F("Search schedule was not acknowledged; power stopped."));
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
      uno.query("BENCHSTOP");
      HttpSupport::sendUnoError(server, uno, 503, F("Search update failed; power stopped."));
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
        if (!applyFinder(finder))
          uno.query("BENCHSTOP");
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
    LocoRoster changed = roster;
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
    LocoRoster changed = roster;
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
    LocoRoster changed = roster;
    changed.entries[index].f0 = on == "1";
    if (!uno.syncRoster(changed)) {
      HttpSupport::sendJson(server, 503,
                            F("{\"error\":\"Uno did not acknowledge the DCC roster\"}"));
      return;
    }
    roster = changed;
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
    if (id < 0 || address < 0) {
      HttpSupport::sendJson(server, 400, F("{\"error\":\"Invalid ID or address\"}"));
      return;
    }
    LocoRoster changed = roster;
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
    LocoRoster changed = roster;
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

void RosterControl::seedLease(uint16_t seed) {
  browserLease.seed(seed);
}
