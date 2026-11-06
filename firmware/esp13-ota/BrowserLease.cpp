#include "BrowserLease.h"

void BrowserLease::seed(uint16_t value) {
  if (!token)
    sequence = value;
}

uint16_t BrowserLease::start(uint32_t now) {
  if (!++sequence)
    ++sequence;
  token = sequence;
  seen = kept = now;
  return token;
}

bool BrowserLease::accepts(uint16_t value, uint32_t now) const {
  return value && value == token && !expired(now);
}

bool BrowserLease::touch(uint16_t value, uint32_t now) {
  if (!accepts(value, now))
    return false;
  seen = now;
  return true;
}

bool BrowserLease::expired(uint32_t now) const {
  return token && uint32_t(now - seen) >= TIMEOUT_MS;
}

bool BrowserLease::due(uint32_t now) const {
  return token && !expired(now) && uint32_t(now - kept) >= KEEP_MS;
}

void BrowserLease::renewed(uint32_t now) {
  kept = now;
}

bool BrowserLease::active() const {
  return token != 0;
}

void BrowserLease::stop() {
  token = 0;
}
