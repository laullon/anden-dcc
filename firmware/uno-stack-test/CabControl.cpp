#include "CabControl.h"
#include "UnoDcc.h"
#include <Arduino.h>
#include <string.h>

namespace {
constexpr uint8_t ENCODER_A = 4, ENCODER_B = 5, SELECT = 6, STOP = 2;
constexpr uint16_t RELEASE_MS = 25;

uint8_t readPhase() {
  return (digitalRead(ENCODER_A) ? 2 : 0) | (digitalRead(ENCODER_B) ? 1 : 0);
}
} // namespace

void CabControl::begin() {
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(SELECT, INPUT_PULLUP);
  pinMode(STOP, INPUT_PULLUP);
  phase = readPhase();
  stopLatched = digitalRead(STOP) == LOW;
  stopLowAt = millis();
  selectLatched = digitalRead(SELECT) == LOW;
  selectLowAt = millis();
}

void CabControl::roster(uint8_t size, const uint8_t* values, uint8_t cabMask) {
  count = 0;
  memset(names, 0, sizeof(names));
  uint8_t next = 0;
  for (uint8_t i = 0; i < size; ++i) {
    if (!(cabMask & (1 << i)))
      continue;
    addresses[count++] = values[i];
    if (!next || values[i] == selected)
      next = values[i];
  }
  selected = next;
  transitions = 0;
}

void CabControl::clearRoster() {
  count = speed = 0;
  transitions = 0;
}

uint8_t CabControl::address() const {
  return count ? selected : 0;
}

int8_t CabControl::step() const {
  return speed;
}

const char* CabControl::name() const {
  for (uint8_t i = 0; i < count; ++i)
    if (addresses[i] == selected)
      return names[i];
  return "";
}

bool CabControl::setName(uint8_t address, const char* hex) {
  for (uint8_t i = 0; i < count; ++i)
    if (addresses[i] == address)
      return CabLabel::decode(names[i], hex);
  return false;
}

bool CabControl::stop() {
  if (!speed)
    return true;
  speed = 0;
  transitions = 0;
  // A timing fault has already disabled the bridge; recovery resets packets.
  return !UnoDcc::isRunning() || UnoDcc::motion(selected, 1);
}

bool CabControl::takePowerToggle() {
  const bool requested = powerToggle;
  powerToggle = false;
  return requested;
}

bool CabControl::poll(bool trackPowered) {
  const uint32_t now = millis();
  const uint8_t current = readPhase();
  const uint8_t previous = phase;
  phase = current;

  if (digitalRead(STOP) == LOW) {
    stopLowAt = now;
    transitions = 0;
    if (!stopLatched) {
      stopLatched = true;
      // Main owns bridge power; emit one request per debounced press.
      powerToggle = true;
      return true;
    }
    return true;
  }
  if (stopLatched) {
    if (uint32_t(now - stopLowAt) >= RELEASE_MS)
      stopLatched = false;
    return true;
  }

  if (speed && !trackPowered)
    return stop();

  if (digitalRead(SELECT) == LOW) {
    selectLowAt = now;
    transitions = 0;
    if (!selectLatched) {
      selectLatched = true;
      if (!stop())
        return false;
      for (uint8_t i = 0; i < count; ++i) {
        if (addresses[i] == selected) {
          selected = addresses[(i + 1) % count];
          break;
        }
      }
    }
    return true;
  }
  if (selectLatched) {
    if (uint32_t(now - selectLowAt) >= RELEASE_MS)
      selectLatched = false;
    return true;
  }
  if (!trackPowered || !count) {
    transitions = 0;
    return true;
  }
  if (current == previous)
    return true;
  if ((current ^ previous) == 3) {
    transitions = 0;
    return true;
  }

  // Count a whole quadrature cycle so contact bounce does not add speed.
  const uint8_t clockwise[] = {1, 3, 0, 2};
  transitions += clockwise[previous] == current ? 1 : -1;
  if (transitions > -4 && transitions < 4)
    return true;

  const int next = int(speed) + (transitions > 0 ? 1 : -1);
  transitions = 0;
  if (next < -int(UnoDcc::MAX_CRAWL_STEP) || next > UnoDcc::MAX_CRAWL_STEP)
    return true;
  const uint8_t magnitude = next < 0 ? -next : next;
  // Zero is a real stop before reversing. Retain the last direction at zero.
  const bool forward = next == 0 ? speed >= 0 : next > 0;
  // Preserve the lowest crawl speed; space the higher levels farther apart.
  const uint8_t code = magnitude ? 2 + (magnitude - 1) * UnoDcc::CRAWL_STEP_SPACING : 0;
  if (!UnoDcc::motion(selected, code, forward))
    return false;
  speed = next;
  return true;
}
