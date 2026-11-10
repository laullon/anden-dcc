#include "CabControl.h"
#include "UnoDcc.h"
#include <Arduino.h>

namespace {
constexpr uint8_t ENCODER_A = 4, ENCODER_B = 5, BUTTON = 6;
constexpr uint16_t HOLD_MS = 700, RELEASE_MS = 25, MAX_MOVE_MS = 800;

uint8_t readPhase() {
  return (digitalRead(ENCODER_A) ? 2 : 0) | (digitalRead(ENCODER_B) ? 1 : 0);
}
} // namespace

void CabControl::begin() {
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);
  phase = readPhase();
  buttonLatched = digitalRead(BUTTON) == LOW;
  longHandled = buttonLatched;
  lastLow = millis();
}

void CabControl::roster(uint8_t size, const uint8_t* values) {
  count = size;
  uint8_t next = size ? values[0] : 0;
  for (uint8_t i = 0; i < size; ++i) {
    addresses[i] = values[i];
    if (values[i] == selected)
      next = selected;
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

uint8_t CabControl::step() const {
  return speed;
}

bool CabControl::stop() {
  if (!speed)
    return true;
  speed = 0;
  transitions = 0;
  return UnoDcc::motion(selected, 1);
}

bool CabControl::poll(bool trackPowered) {
  const uint32_t now = millis();
  const uint8_t current = readPhase();
  const uint8_t previous = phase;
  phase = current;

  if (speed && (!trackPowered || uint32_t(now - motionStarted) >= MAX_MOVE_MS))
    return stop();

  if (digitalRead(BUTTON) == LOW) {
    lastLow = now;
    transitions = 0;
    if (!buttonLatched) {
      buttonLatched = true;
      longHandled = false;
      pressedAt = now;
      // Stop on the first low sample. Only release is debounced.
      return speed ? stop() : !trackPowered || !count || UnoDcc::motion(selected, 1);
    }
    if (!longHandled && uint32_t(now - pressedAt) >= HOLD_MS) {
      longHandled = true;
      for (uint8_t i = 0; i < count; ++i) {
        if (addresses[i] == selected) {
          selected = addresses[(i + 1) % count];
          break;
        }
      }
    }
    return true;
  }

  if (buttonLatched) {
    if (uint32_t(now - lastLow) >= RELEASE_MS)
      buttonLatched = false;
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
  if (next < 0 || next > UnoDcc::MAX_CRAWL_STEP)
    return true;
  if (!UnoDcc::motion(selected, next ? next + 1 : 0))
    return false;
  if (!speed && next)
    motionStarted = now;
  speed = next;
  return true;
}
