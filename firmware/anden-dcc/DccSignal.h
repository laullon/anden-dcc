#pragma once
#include <stdint.h>
#include "LocoRoster.h"
#include "AddressFinder.h"

namespace DccSignal {
constexpr uint8_t PIN = 2; // Pico physical pin 4; adjacent GND is pin 3.
bool begin(const LocoRoster& roster);
bool setRoster(const LocoRoster& roster, bool resume = false);
bool setFinder(const AddressFinder& finder);
void stop();
bool ready();
bool running();
}
