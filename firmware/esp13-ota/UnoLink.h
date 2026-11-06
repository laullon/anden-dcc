#pragma once
#include <AndenStackLink.h>
#include <Arduino.h>

class LocoRoster;

class UnoLink {
public:
  void begin();
  void attach(const LocoRoster& value);
  void holdRoster(bool hold);
  void invalidate();
  void loop();
  bool matches(const LocoRoster& value) const;
  bool syncRoster(const LocoRoster& value);
  bool functionsSupported() const;
  bool trackSupported() const;
  bool trackActive() const;
  bool dccReady() const;
  bool benchSupported() const;
  bool benchActive() const;
  uint8_t benchResult() const;
  uint8_t benchMode() const;
  bool dccBenchSupported() const;
  bool dccPowerActive() const;
  uint16_t statusSignature() const;
  uint32_t packetCount() const;
  uint16_t faultCount() const;

  bool query(const char* command);
  bool online() const;
  bool outputsDisabled() const;
  String json() const;

private:
  bool finderHold = false;
  AndenLink::Receiver receiver;
  uint16_t sequence = 0, adcA = 0, adcB = 0, flags = 0, ledEdges = 0;
  uint32_t lastPoll = 0, lastSeen = 0, roundTrip = 0, timeouts = 0;
  bool known = false;
  char version[12] = {0};
  const LocoRoster* desired = nullptr;
  bool dirty = true, capable = false, dccRunning = false;
  uint16_t dccCount = 0, dccMask = 0, dccSignature = 0, faults = 0;
  uint32_t packets = 0;
  bool benchCapable = false, dccBenchCapable = false, trackCapable = false,
       functionsCapable = false;
  uint16_t benchRemaining = 0;
  uint8_t benchState = 0, mode = 0;
  bool accept(uint16_t expected);
};
