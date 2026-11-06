#pragma once
#include "BrowserLease.h"
#include "FavoriteFunctions.h"
#include <AddressFinder.h>
#include <Arduino.h>
#include <LocoRoster.h>

#include <ESP8266WebServer.h>
class UnoLink;

class RosterControl {
public:
  RosterControl(ESP8266WebServer& webServer, UnoLink& unoLink);
  RosterControl(const RosterControl&) = delete;
  RosterControl& operator=(const RosterControl&) = delete;

  void begin();
  void seedLease(uint16_t seed);
  void registerRoutes();
  void loop();
  String dccStatus();
  bool storageReady() const;
  const char* stopReason() const;
  bool setupLocked() const;
  const LocoRoster& configuration() const;

private:
  ESP8266WebServer& server;
  UnoLink& uno;
  LocoRoster roster;
  FavoriteFunctions favorites;
  AddressFinder finder;
  BrowserLease browserLease;
  bool rosterStorage = false;
  const char* trackStopReason = "Track A output is disabled.";

  String finderStatus();
  bool applyFinder(const AddressFinder& value);
  void serviceFinder();
  static int rosterNumber(const String& text, int maximum);
  void loadRoster();
  bool saveRoster(const LocoRoster& value, const FavoriteFunctions* changed = nullptr);
  void serviceTrackLease();
  void trackRoutes();
  void finderRoutes();
  void favoriteRoutes();
  void rosterRoutes();
};
