#include "CabI2c.h"
#include <Arduino.h>

#if !defined(__AVR_ATmega328P__)
#error "The cab I2C transport requires an ATmega328P Uno"
#endif

namespace {
constexpr uint8_t TIMEOUT_MS = 4;

bool complete(uint32_t started, uint8_t expectedStatus) {
  // TWIE stays clear: poll the hardware flag while DCC/UART interrupts run.
  while (!(TWCR & _BV(TWINT)))
    if (uint32_t(millis() - started) >= TIMEOUT_MS)
      return false;
  return (TWSR & 0xF8) == expectedStatus;
}
} // namespace

void CabI2c::begin() {
  TWCR = 0;
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  TWSR = 0; // Prescaler 1.
  TWBR = (F_CPU / 400000UL - 16) / 2;
  TWCR = _BV(TWEN);
}

bool CabI2c::write(uint8_t address, const uint8_t* bytes, uint8_t count) {
  if (address > 127 || !bytes || !count || count > MAX_BYTES)
    return false;
  const uint32_t started = millis();
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
  bool ok = complete(started, 0x08); // START.
  if (ok) {
    TWDR = address << 1;
    TWCR = _BV(TWINT) | _BV(TWEN);
    ok = complete(started, 0x18); // Address + write acknowledged.
  }
  for (uint8_t i = 0; ok && i < count; ++i) {
    TWDR = bytes[i];
    TWCR = _BV(TWINT) | _BV(TWEN);
    ok = complete(started, 0x28); // Data acknowledged.
  }
  TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
  while (TWCR & _BV(TWSTO)) {
    if (uint32_t(millis() - started) >= TIMEOUT_MS) {
      ok = false;
      break;
    }
  }
  // Release the bus after a NACK, arbitration error or stalled device. The
  // display disables further writes on failure; the cab continues running.
  if (!ok)
    TWCR = 0;
  return ok;
}
