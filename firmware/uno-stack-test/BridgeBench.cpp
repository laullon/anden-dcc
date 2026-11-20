#include "BridgeBench.h"

bool BridgeBench::start(uint8_t requestedMode, uint32_t now, uint16_t raw, bool healthy) {
  if (requestedMode > Track || active())
    return false; // A second command cannot extend a running pulse.
  if (!healthy) {
    state = TimingFault;
    return false;
  }
  if (requestedMode != Track && raw >= RAW_LIMIT) {
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
  else if (mode != Track && raw >= RAW_LIMIT)
    state = RawTrip;
  else if (mode != Track && uint32_t(now - started) >= duration())
    state = Complete;
}

void BridgeBench::stop() {
  if (active())
    state = Stopped;
}

uint16_t BridgeBench::remaining(uint32_t now) const {
  if (mode == Track)
    return 0; // Latched track output has no connection lease or deadline.
  uint32_t elapsed = now - started;
  return active() && elapsed < duration() ? duration() - elapsed : 0;
}
