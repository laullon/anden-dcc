#!/usr/bin/env python3
"""Exercise the Uno OLED transport against modeled AVR TWI registers."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='anden-cab-i2c-') as directory:
    folder = Path(directory)
    (folder / 'Arduino.h').write_text(r"""
#pragma once
#include <stdint.h>
#include <cassert>
#include <vector>
#define __AVR_ATmega328P__ 1
#define F_CPU 16000000UL
#define _BV(bit) (1U << (bit))
#define TWINT 7
#define TWSTA 5
#define TWSTO 4
#define TWEN 2
#define TWIE 0
#define A4 18
#define A5 19
#define INPUT_PULLUP 2
extern uint8_t TWSR, TWBR, TWDR;
extern uint32_t clockMs;
extern int failure, phase;
extern unsigned starts, stops, initializedPins;
extern std::vector<uint8_t> sent;
inline uint32_t millis() { return clockMs++; }
inline void pinMode(uint8_t pin, uint8_t mode) {
  assert((pin == A4 || pin == A5) && mode == INPUT_PULLUP);
  ++initializedPins;
}
struct Control {
  uint8_t value = 0;
  operator uint8_t() const { return value; }
  Control& operator=(uint8_t next) {
    // A TWI interrupt would compete with the DCC Timer1 mirror deadline.
    assert(!(next & _BV(TWIE)));
    value = next;
    if (next & _BV(TWSTO)) {
      ++stops;
      value = failure == 4 ? next : _BV(TWEN);
    } else if (next & _BV(TWINT)) {
      if (failure == 5) {
        value &= ~_BV(TWINT); // Bus never completes the START.
      } else if (next & _BV(TWSTA)) {
        ++starts;
        phase = 0;
        TWSR = failure == 1 ? 0x38 : 0x08;
      } else {
        sent.push_back(TWDR);
        TWSR = phase++ == 0 ? (failure == 2 ? 0x20 : 0x18)
                            : (failure == 3 ? 0x30 : 0x28);
      }
    }
    return *this;
  }
};
extern Control TWCR;
""")
    (folder / 'transport.cpp').write_text(r"""
#include "Arduino.h"
#include "CabI2c.h"
uint8_t TWSR = 0, TWBR = 0, TWDR = 0;
uint32_t clockMs = 0;
int failure = 0, phase = 0;
unsigned starts = 0, stops = 0, initializedPins = 0;
std::vector<uint8_t> sent;
Control TWCR;
int main() {
  CabI2c::begin();
  assert(initializedPins == 2 && TWBR == 12 && TWSR == 0);
  const uint8_t bytes[] = {0x80, 0xB0, 0x40};
  assert(CabI2c::write(0x3C, bytes, sizeof(bytes)));
  assert((sent == std::vector<uint8_t>{0x78, 0x80, 0xB0, 0x40}));
  assert(starts == 1 && stops == 1);
  for (failure = 1; failure <= 5; ++failure) {
    CabI2c::begin();
    const uint32_t before = clockMs;
    const unsigned previousStops = stops;
    assert(!CabI2c::write(0x3C, bytes, sizeof(bytes)));
    assert(clockMs - before <= 6); // Stalled START/STOP is bounded.
    assert(stops == previousStops + 1 && uint8_t(TWCR) == 0);
  }
  failure = 5;
  clockMs = UINT32_MAX - 1;
  CabI2c::begin();
  const uint32_t rolloverStart = clockMs;
  assert(!CabI2c::write(0x3C, bytes, sizeof(bytes)));
  assert(uint32_t(clockMs - rolloverStart) <= 6);
  failure = 0;
  CabI2c::begin();
  assert(CabI2c::write(0x3C, bytes, sizeof(bytes)));
  const unsigned previousStarts = starts;
  assert(!CabI2c::write(128, bytes, 3));
  assert(!CabI2c::write(0x3C, nullptr, 3));
  assert(!CabI2c::write(0x3C, bytes, 0));
  assert(!CabI2c::write(0x3C, bytes, 32));
  assert(starts == previousStarts);
}
""")
    executable = folder / 'transport'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    '-I', str(folder), '-I', str(root / 'firmware/uno-stack-test'),
                    str(folder / 'transport.cpp'),
                    str(root / 'firmware/uno-stack-test/CabI2c.cpp'),
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('PASS: polled 400 kHz OLED writes keep TWIE clear, validate ACKs, stop on errors and bound stalled START/STOP.')
