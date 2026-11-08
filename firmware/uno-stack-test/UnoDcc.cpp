#include "UnoDcc.h"
#include "BridgeBench.h"
#include <Arduino.h>
#include <UnoDccStream.h>
#include <util/atomic.h>

#if !defined(__AVR_ATmega328P__)
#error "This direct-port routing requires an ATmega328P Uno"
#endif
static_assert(F_CPU == 16000000UL, "Uno DCC timing requires a 16 MHz CPU");
namespace UnoDcc {
namespace {
// The main loop prepares schedules. Timer1 owns the waveform and DCC output edges.
UnoDccStream stream;
BridgeGuard bridgeGuard;
volatile bool running = false;
volatile uint16_t deadlineFaults = 0;
volatile bool dccBridge = false, bridgePending = false;
} // namespace
// DC can enable immediately. DCC leaves A disabled until the ISR reaches a
// packet boundary, so the first signal on the rails starts with a preamble.
bool armBridge(bool dcc) {
  bool armed = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (running) {
      PORTD &= ~_BV(PD3);
      bridgeGuard.disarm();
      bridgeGuard.expired = false;
      dccBridge = dcc;
      bridgePending = dcc;
      if (!dcc) {
        bridgeGuard.arm();
        PORTD |= _BV(PD3);
      }
      armed = true;
    }
  }
  return armed;
}
void disarmBridge() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    PORTD &= ~_BV(PD3);
    dccBridge = false;
    bridgePending = false;
    bridgeGuard.disarm();
    PORTB &= ~_BV(PB4);
  }
}
namespace {
// Drop track power before halting Timer1. Recovery requires a reset.
void timingFault() {
  PORTD &= ~_BV(PD3);
  dccBridge = false;
  bridgePending = false;
  TCCR1A = 0;
  TIMSK1 = 0;
  PORTB &= ~(_BV(PB2) | _BV(PB4));
  running = false;
  ++deadlineFaults;
}
} // namespace
void begin() {
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);
  uint8_t empty[1] = {0};
  stream.prepare(0, empty, 0);
  stream.commit();
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    TCNT1 = 0;
    OCR1B = stream.nextHalfTicks();
    TIFR1 = _BV(OCF1B) | _BV(TOV1);
    TCCR1A = _BV(COM1B0); // OC1B toggles in hardware at compare, normal mode.
    running = true;
    TIMSK1 = _BV(OCIE1B);
    TCCR1B = _BV(CS11); // 16 MHz / 8 gives 0.5 us timer ticks.
  }
}
bool set(uint8_t count, const uint8_t* addresses, uint8_t mask) {
  if (!running || count > UnoDccStream::CAPACITY || (count && !addresses))
    return false;
  // A signature confirms transport state; it is not an equality check between
  // legacy, stationary-control and extended-function schedules.
  if (!stream.prepare(count, addresses, mask))
    return false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    stream.commit();
  }
  // Report success only once the ISR has published the new bank. This bounded
  // wait also covers the case where a timer fault prevents publication.
  uint32_t start = millis();
  while (stream.pending != 255 && uint32_t(millis() - start) < 20) {
  }
  return stream.pending == 255;
}
bool control(uint8_t count, const uint8_t* addresses, uint8_t mask, uint8_t directions,
             const uint8_t* speeds) {
  if (!running || !stream.prepareControl(count, addresses, mask, directions, speeds))
    return false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    stream.commit();
  }
  uint32_t started = millis();
  while (stream.pending != 255 && uint32_t(millis() - started) < 20) {
  }
  return stream.pending == 255;
}
bool functions(uint8_t count, const uint8_t* addresses, uint8_t directions, const uint32_t* flags) {
  if (!running || !stream.prepareFunctions(count, addresses, directions, flags))
    return false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    stream.commit();
  }
  uint32_t started = millis();
  while (stream.pending != 255 && uint32_t(millis() - started) < 20) {
  }
  return stream.pending == 255;
}
bool find(uint8_t first, uint8_t last, uint8_t low, uint8_t high) {
  if (!running || !stream.prepareFinder(first, last, low, high))
    return false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    stream.commit();
  }
  uint32_t started = millis();
  while (stream.pending != 255 && uint32_t(millis() - started) < 20) {
  }
  return stream.pending == 255;
}
void renewGuard() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (dccBridge && !bridgePending && !bridgeGuard.expired)
      bridgeGuard.arm();
  }
}
bool isRunning() {
  return running;
}
bool guardExpired() {
  return bridgeGuard.expired;
}
bool bridgeStartPending() {
  return bridgePending;
}
bool bridgeIsActive() {
  return dccBridge;
}
void startStopped() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    stream.startStopped();
  }
}
Status status() {
  Status s;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    const auto& bank = stream.banks[stream.active];
    s.count = bank.count;
    s.mask = bank.mask;
    s.signature = bank.signature;
    s.faults = deadlineFaults;
    s.packets = stream.packets;
    s.running = running;
  }
  return s;
}
} // namespace UnoDcc
ISR(TIMER1_COMPB_vect) {
  const uint16_t edge = OCR1B;
  if (UnoDcc::dccBridge && (!UnoDcc::bridgePending || UnoDcc::stream.atPacketStart())) {
    // Mirror D10 at a fixed phase after the compare, before variable packet work.
    // Waiting absorbs normal UART/Timer0 ISR latency; a late mirror fails OFF.
    const uint16_t target = edge + UnoDcc::BRIDGE_DELAY_TICKS;
    uint16_t now;
    do {
      now = TCNT1;
    } while (int16_t(now - target) < 0);
    if (uint16_t(now - target) > UnoDcc::BRIDGE_LATE_TICKS) {
      UnoDcc::timingFault();
      return;
    }
    if (PINB & _BV(PB2))
      PORTB |= _BV(PB4);
    else
      PORTB &= ~_BV(PB4);
    if (UnoDcc::bridgePending) {
      UnoDcc::bridgePending = false;
      UnoDcc::bridgeGuard.arm();
      PORTD |= _BV(PD3);
    }
  }
  if (UnoDcc::bridgeGuard.tick()) {
    PORTD &= ~_BV(PD3);
    UnoDcc::dccBridge = false;
    UnoDcc::bridgePending = false;
    PORTB &= ~_BV(PB4);
  }
  // Schedule from the hardware edge to keep ISR latency out of the bit period.
  // A signed delta handles timer wrap and rejects a compare too close to arm.
  uint16_t due = edge + UnoDcc::stream.nextHalfTicks();
  if (int16_t(due - TCNT1) <= 8)
    UnoDcc::timingFault();
  else
    OCR1B = due;
}
