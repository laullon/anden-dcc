#include "BridgeBench.h"
#include "UnoDcc.h"
#include <AndenStackLink.h>
#include <Arduino.h>
#include <DccFunctions.h>
#include <stdio.h>
#include <string.h>

constexpr char VERSION[] = "0.6.1";
constexpr uint8_t ENABLE_A = 3, ENABLE_B = 11, BRAKE_A = 9, BRAKE_B = 8, DIR_A = 12, DIR_B = 13;
namespace {
void serviceBridge(bool fresh = false);
bool startBridge(uint8_t mode);
void sendState(uint16_t sequence);
void command();

AndenLink::Receiver receiver;
BridgeBench bench;
uint8_t ledEdges = 0;
bool controlReady = false;
uint16_t rawA = 0;
uint32_t nextEdge = 0, lastSense = 0;

void serviceBridge(bool fresh) {
  uint32_t now = millis();
  if (fresh || (bench.active() && uint32_t(now - lastSense) >= 1)) {
    rawA = analogRead(A0);
    lastSense = now;
  }
  bool wasActive = bench.active();
  bench.poll(now, rawA, UnoDcc::isRunning(), UnoDcc::guardExpired());
  if (wasActive && !bench.active())
    UnoDcc::disarmBridge();
  digitalWrite(ENABLE_B, LOW);
  if (!bench.active()) {
    digitalWrite(ENABLE_A, LOW);
    digitalWrite(DIR_A, LOW);
  }
}

bool startBridge(uint8_t mode) {
  serviceBridge(true);
  if (digitalRead(ENABLE_A) || digitalRead(ENABLE_B) || bench.active())
    return false;
  if (!bench.start(mode, millis(), rawA, UnoDcc::isRunning()))
    return false;
  ledEdges = 0;
  digitalWrite(DIR_B, LOW);
  digitalWrite(DIR_A, mode == BridgeBench::DcForward
                          ? HIGH
                          : LOW); // Brake A remains LOW: opposite bridge inputs.
  if (mode == BridgeBench::Track) {
    UnoDcc::startStopped();
  }
  if (!UnoDcc::armBridge(mode >= BridgeBench::Dcc)) {
    bench.poll(millis(), rawA, false, false);
    serviceBridge();
    return false;
  }
  if (mode >= BridgeBench::Dcc) {
    uint32_t waiting = millis();
    while (bench.active() && UnoDcc::isRunning() && UnoDcc::bridgeStartPending() &&
           uint32_t(millis() - waiting) < 20)
      serviceBridge();
    if (UnoDcc::bridgeStartPending() || !UnoDcc::isRunning()) {
      bench.poll(millis(), rawA, false, false);
      UnoDcc::disarmBridge();
      serviceBridge();
      return false;
    }
  }
  return bench.active() && digitalRead(ENABLE_A);
}

void sendState(uint16_t sequence) {
  serviceBridge(true);
  uint8_t flags = (digitalRead(ENABLE_A) ? 1 : 0) | (digitalRead(ENABLE_B) ? 2 : 0) |
                  (digitalRead(BRAKE_A) ? 4 : 0) | (digitalRead(BRAKE_B) ? 8 : 0) |
                  (digitalRead(DIR_A) ? 16 : 0) | (digitalRead(DIR_B) ? 32 : 0);
  char body[AndenLink::MAX_BODY + 1], wire[AndenLink::MAX_BODY + 8];
  auto dcc = UnoDcc::status();
  snprintf(body, sizeof(body), "AD1,%u,STATE,%s,%u,%u,%u,%u,%u,%u,%u,%u,%lu,%u,%u,%u,%u,2",
           sequence, VERSION, unsigned(rawA), unsigned(analogRead(A1)), unsigned(flags),
           unsigned(ledEdges), unsigned(dcc.running), unsigned(dcc.count), unsigned(dcc.signature),
           unsigned(dcc.mask), static_cast<unsigned long>(dcc.packets), unsigned(dcc.faults),
           unsigned(bench.remaining(millis())), unsigned(bench.state), unsigned(bench.mode));
  size_t size = AndenLink::encode(wire, sizeof(wire), body);
  if (size)
    Serial.write(reinterpret_cast<const uint8_t*>(wire), size);
}

void command() {
  char* parts[30];
  size_t n = AndenLink::fields(receiver.body, parts, 30);
  if (n < 3 || strcmp(parts[0], "AD1"))
    return;
  uint16_t sequence;
  if (!AndenLink::number(parts[1], sequence) || !sequence)
    return;
  serviceBridge();
  if (!strcmp(parts[2], "BENCHSTOP") && n == 3) {
    bench.stop();
    UnoDcc::disarmBridge();
    serviceBridge();
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "TRACKON") && n == 3) {
    if (controlReady)
      startBridge(BridgeBench::Track);
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "KEEP") && n == 3) {
    serviceBridge(true);
    if (bench.renew(millis()))
      UnoDcc::renewGuard();
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "FIND") && n == 7) {
    if (bench.active() && bench.mode != BridgeBench::Track)
      return;
    uint16_t values[4];
    for (uint8_t i = 0; i < 4; ++i)
      if (!AndenLink::number(parts[3 + i], values[i]) || values[i] > 127)
        return;
    if (!UnoDcc::find(values[0], values[1], values[2], values[3]))
      return;
    controlReady = true;
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "CONTROLX")) {
    if (bench.active() && bench.mode != BridgeBench::Track)
      return;
    uint16_t count, directions;
    if (n < 5 || !AndenLink::number(parts[3], count) || count > 8 || n != 5 + count * 2 ||
        !AndenLink::number(parts[4], directions) || directions > 255)
      return;
    uint8_t addresses[8] = {0};
    uint32_t flags[8] = {0};
    for (uint8_t i = 0; i < count; ++i) {
      uint16_t a;
      if (!AndenLink::number(parts[5 + i * 2], a) || !a || a > 127 ||
          !AndenLink::number32(parts[6 + i * 2], flags[i], DccFunctions::MASK))
        return;
      addresses[i] = a;
    }
    if (!UnoDcc::functions(count, addresses, directions, flags))
      return;
    controlReady = true;
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "CONTROL")) {
    if (bench.active() && bench.mode != BridgeBench::Track)
      return;
    uint16_t count, mask, directions;
    if (n < 6 || !AndenLink::number(parts[3], count) || count > 8 || n != 6 + count * 2 ||
        !AndenLink::number(parts[4], mask) || mask > 255 ||
        !AndenLink::number(parts[5], directions) || directions > 255)
      return;
    uint8_t addresses[8] = {0}, speeds[8] = {0};
    for (uint8_t i = 0; i < count; ++i) {
      uint16_t a, s;
      if (!AndenLink::number(parts[6 + i * 2], a) || !a || a > 127 ||
          !AndenLink::number(parts[7 + i * 2], s) || s != 0)
        return;
      addresses[i] = a;
      speeds[i] = s;
    }
    if (!UnoDcc::control(count, addresses, mask, directions, speeds))
      return;
    controlReady = true;
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "DCCTEST") && n == 3) {
    startBridge(BridgeBench::Dcc);
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "PULSE") && n == 4) {
    uint16_t direction;
    if (!AndenLink::number(parts[3], direction) || direction > 1)
      return;
    startBridge(direction);
    sendState(sequence);
    return;
  }
  if (!strcmp(parts[2], "ROSTER")) {
    if (bench.active())
      return;
    uint16_t count;
    if (n < 4 || !AndenLink::number(parts[3], count) || count > 8 || n != 4 + count * 2)
      return;
    uint8_t addresses[8] = {0}, mask = 0;
    for (uint8_t i = 0; i < count; ++i) {
      uint16_t address, on;
      if (!AndenLink::number(parts[4 + i * 2], address) || !address || address > 127 ||
          !AndenLink::number(parts[5 + i * 2], on) || on > 1)
        return;
      addresses[i] = address;
      if (on)
        mask |= 1 << i;
    }
    if (!UnoDcc::set(count, addresses, mask))
      return;
    controlReady = false;
  } else if (n != 3)
    return;
  else if (!strcmp(parts[2], "LED")) {
    if (bench.active())
      return;
    digitalWrite(DIR_B, HIGH);
    ledEdges = 15;
    nextEdge = millis() + 120;
  } else if (strcmp(parts[2], "PING") && strcmp(parts[2], "STATUS"))
    return;
  serviceBridge();
  sendState(sequence);
}

} // namespace

void setup() {
  const uint8_t pins[] = {ENABLE_A, ENABLE_B, BRAKE_A, BRAKE_B, DIR_A, DIR_B};
  for (uint8_t pin : pins) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  Serial.begin(115200);
  UnoDcc::begin();
}

void loop() {
  serviceBridge();
  if (ledEdges && int32_t(millis() - nextEdge) >= 0) {
    --ledEdges;
    digitalWrite(DIR_B, ledEdges ? !digitalRead(DIR_B) : LOW);
    nextEdge = millis() + 120;
  }
  while (Serial.available()) {
    serviceBridge();
    int c = Serial.read();
    if (c >= 0 && receiver.feed(char(c)))
      command();
  }
}
