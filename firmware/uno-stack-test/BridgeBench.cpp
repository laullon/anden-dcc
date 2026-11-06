#include "BridgeBench.h"

bool BridgeBench::start(uint8_t requestedMode, uint32_t now, uint16_t raw, bool healthy) {
  if (requestedMode > Track || active())
    return false; // A second command cannot extend a running pulse.
  if (!healthy) {
    state = TimingFault;
    return false;
  }
  if (raw >= RAW_LIMIT) {
    state = RawTrip;
    return false;
  }
  mode = requestedMode;
  direction = mode == DcForward;
  started = now;
  state = Running;
  return true;
}

void BridgeBench::poll(uint32_t now, uint16_t raw, bool healthy, bool guardExpired) {
  if (!active())
    return;
  if (!healthy)
    state = TimingFault;
  else if (guardExpired)
    state = GuardExpired;
  else if (raw >= RAW_LIMIT)
    state = RawTrip;
  else if (uint32_t(now - started) >= duration())
    state = mode == Track ? LeaseExpired : Complete;
}

bool BridgeBench::renew(uint32_t now) {
  if (!active() || mode != Track)
    return false;
  if (uint32_t(now - started) >= duration()) {
    state = LeaseExpired;
    return false;
  }
  started = now;
  return true;
}

void BridgeBench::stop() {
  if (active())
    state = Stopped;
}

uint16_t BridgeBench::remaining(uint32_t now) const {
  uint32_t elapsed = now - started;
  return active() && elapsed < duration() ? duration() - elapsed : 0;
}
