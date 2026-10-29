#pragma once
#include <stdint.h>

namespace DccSignal {
constexpr uint8_t PIN = 2; // Pico physical pin 4; adjacent GND is pin 3.
bool begin();
bool setF0(uint8_t address, bool enabled);
void stop();
bool ready();
bool running();
uint8_t address();
bool f0();
}
