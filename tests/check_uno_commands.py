#!/usr/bin/env python3
"""Exercise the Uno sketch's UART commands with simulated hardware and DCC API."""
from native_support import ROOT, library_sources
from pathlib import Path
import os
import subprocess
import tempfile

with tempfile.TemporaryDirectory(prefix='anden-uno-commands-') as directory:
    folder = Path(directory)
    (folder / 'Arduino.h').write_text(r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string>
#include <deque>
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define INPUT_PULLUP 2
#define A0 14
#define A1 15
extern uint32_t clockMs;
extern uint8_t pins[20];
inline uint32_t millis() { return clockMs; }
inline void pinMode(uint8_t pin, uint8_t mode) { if (mode == INPUT_PULLUP) pins[pin] = HIGH; }
inline void digitalWrite(uint8_t pin, uint8_t value) { pins[pin] = value; }
inline int digitalRead(uint8_t pin) { return pins[pin]; }
inline int analogRead(uint8_t) { return 0; }
struct FakeSerial {
    std::deque<uint8_t> input;
    std::string output;
    void begin(unsigned long) {}
    int available() { return input.size(); }
    int read() { if (input.empty()) return -1; int c=input.front(); input.pop_front(); return c; }
    size_t write(const uint8_t* bytes, size_t count) { output.append(reinterpret_cast<const char*>(bytes), count); return count; }
};
extern FakeSerial Serial;
''')
    source = folder / 'commands.cpp'
    source.write_text(r'''
#include "Arduino.h"
#include "UnoDcc.h"
#include "UnoDccStream.h"
#include "CabDisplay.h"
#include <cassert>
uint32_t clockMs = 0;
uint8_t pins[20] = {};
FakeSerial Serial;
void CabDisplay::begin() {}
void CabDisplay::update(uint8_t, uint8_t) {}
namespace UnoDcc {
UnoDccStream schedule;
bool running = false;
void begin() { running = true; }
bool isRunning() { return running; }
bool guardExpired() { return false; }
bool bridgeStartPending() { return false; }
bool bridgeIsActive() { return pins[3]; }
unsigned broadcastStops = 0;
void startStopped() { ++broadcastStops; }
bool armBridge(bool) { pins[3] = 1; return true; }
void disarmBridge() { pins[3] = 0; }
void renewGuard() {}
Status status() { const auto& b = schedule.banks[schedule.active]; return {b.count,b.mask,b.signature,0,0,running}; }
bool set(uint8_t count, const uint8_t* addresses, uint8_t mask) { return schedule.prepare(count, addresses, mask); }
bool control(uint8_t count, const uint8_t* addresses, uint8_t mask, uint8_t directions, const uint8_t* speeds) {
    return schedule.prepareControl(count, addresses, mask, directions, speeds);
}
bool functions(uint8_t count, const uint8_t* addresses, uint8_t directions, const uint32_t* flags, uint8_t cabMask) {
    return schedule.prepareFunctions(count, addresses, directions, flags, cabMask);
}
bool motion(uint8_t address, uint8_t speedCode) {
    if (!schedule.prepareMotion(address, speedCode)) return false;
    schedule.active ^= 1;
    return true;
}
bool find(uint8_t first, uint8_t last, uint8_t low, uint8_t high) { return schedule.prepareFinder(first,last,low,high); }
}
#include "uno-stack-test.ino"
void send(const char* body, bool reply = true) {
    char frame[AndenLink::MAX_BODY+8];
    auto count=AndenLink::encode(frame,sizeof(frame),body);assert(count);
    Serial.output.clear();
    for(size_t i=0;i<count;++i) Serial.input.push_back(frame[i]);
    loop();
    assert(!Serial.output.empty() == reply);
}
void prepare() { send("AD1,1,CONTROLX,1,1,3,0"); }
void turnOn() { send("AD1,2,TRACKON");assert(pins[3]);send("AD1,3,BENCHSTOP");assert(!pins[3]); }
int main() {
    setup();
    assert(pins[7] == LOW);
    send("AD1,1,TRACKON");assert(!pins[3]);
    prepare();turnOn();
    // Rejected legacy roster requests must preserve the last valid control state.
    const char* invalid[]={"AD1,4,ROSTER", "AD1,4,ROSTER,9", "AD1,4,ROSTER,1,128,0",
        "AD1,4,ROSTER,2,3,0,3,1", "AD1,4,ROSTER,1,3,2", "AD1,4,ROSTER,1,3,0,extra"};
    for(const char* body:invalid) { prepare();send(body,false);turnOn(); }
    // A successful legacy update does invalidate stationary CONTROL readiness.
    prepare();send("AD1,4,ROSTER,1,4,0");send("AD1,5,TRACKON");assert(!pins[3]);
    prepare();turnOn();
    // Both command routes must retain their distinct schedules even when their
    // transport signatures collide (actual wrapper/ISR coverage is separate).
    send("AD1,6,CONTROLX,1,1,3,122242");
    const auto& functionBank=UnoDcc::schedule.banks[UnoDcc::schedule.active^1];
    assert(functionBank.functionMode&&functionBank.signature==52175);
    send("AD1,7,ROSTER,1,3,0");
    const auto& legacyBank=UnoDcc::schedule.banks[UnoDcc::schedule.active^1];
    assert(!legacyBank.functionMode&&legacyBank.signature==52175&&legacyBank.slots==1);
    // Bad directions/functions and motion requests cannot turn an output on.
    send("AD1,6,CONTROLX,1,2,3,0",false);
    send("AD1,6,CONTROLX,1,1,3,536870912",false);
    send("AD1,6,CONTROL,1,0,1,3,1",false);
    assert(!pins[3]);

    // Only slots 0 and 2 are on the diorama. Reject bits beyond the roster.
    send("AD1,8,CONTROLX,3,7,3,1,4,0,6,8,8",false);
    send("AD1,9,CONTROLX,3,7,3,1,4,0,6,8,5");
    UnoDcc::schedule.active ^= 1;
    auto rotate = [](bool clockwise) {
        const int phases[] = {2,0,1};
        for (int i = 0; i < 4; ++i) {
            const int phase = i == 3 ? 3 : phases[clockwise ? i : 2-i];
            pins[4] = bool(phase & 2); pins[5] = bool(phase & 1);
            ++clockMs; loop();
        }
    };
    auto packetSpeed = [](unsigned index) {
        return UnoDcc::schedule.banks[UnoDcc::schedule.active].functionPackets[index][1][2] & 127;
    };
    rotate(true); assert(cab.step() == 0);
    send("AD1,10,TRACKON");
    assert(pins[3] && cab.address() == 3 && pins[7] == LOW);
    for (int i = 0; i < 5; ++i) rotate(true);
    assert(cab.step() == 3 && packetSpeed(0) == 4 && packetSpeed(1) == 0 && pins[7] == HIGH);
    rotate(false); assert(cab.step() == 2 && packetSpeed(0) == 3);
    const unsigned starts = UnoDcc::broadcastStops;
    pins[2] = LOW; loop();
    assert(cab.step() == 0 && packetSpeed(0) == 1 && pins[3] && pins[7] == LOW);
    assert(cab.address() == 3 && UnoDcc::broadcastStops == starts+1);
    rotate(true); assert(cab.step() == 0 && UnoDcc::broadcastStops == starts+1);
    pins[2] = HIGH; loop();
    rotate(true); assert(cab.step() == 0); // Release bounce cannot restart motion.
    clockMs += 25; loop();

    pins[6] = LOW; loop(); assert(cab.address() == 6 && cab.step() == 0);
    clockMs += 50; loop(); assert(cab.address() == 6); // One change per press.
    pins[6] = HIGH; clockMs += 25; loop();
    rotate(true); assert(cab.step() == 1 && packetSpeed(2) == 2);
    pins[6] = LOW; loop();
    assert(cab.address() == 3 && cab.step() == 0 && packetSpeed(2) == 1);
    pins[6] = HIGH; clockMs += 25; loop();
    rotate(true);
    clockMs += 800; loop(); assert(cab.step() == 0 && packetSpeed(0) == 1 && pins[3] && pins[7] == LOW);
    rotate(true);
    send("AD1,11,CONTROLX,3,5,3,1,4,0,6,8,4"); // Remove selected address 3.
    UnoDcc::schedule.active ^= 1;
    assert(cab.step() == 0 && cab.address() == 6 && packetSpeed(0) == 0 && pins[7] == LOW);
    rotate(true); assert(cab.step() == 1);
    clockMs += 2000; loop();
    assert(cab.step() == 0 && !pins[3] && packetSpeed(2) == 1 && pins[7] == LOW);
    send("AD1,12,TRACKON");
    assert(pins[3] && cab.step() == 0 && packetSpeed(2) == 1);
    send("AD1,13,FIND,1,10,1,5");
    UnoDcc::schedule.active ^= 1;
    rotate(true); assert(cab.step() == 0 && cab.address() == 0);
    send("AD1,14,BENCHSTOP");
    send("AD1,15,CONTROLX,1,1,3,0"); // Legacy snapshot opts in nobody.
    UnoDcc::schedule.active ^= 1;
    send("AD1,16,TRACKON");
    rotate(true); assert(cab.address() == 0 && cab.step() == 0 && pins[3]);
    const unsigned idleStops = UnoDcc::broadcastStops;
    pins[2] = LOW; loop();
    assert(UnoDcc::broadcastStops == idleStops+1 && pins[3]);
}
''')
    executable = folder / 'commands'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-I', str(folder),
                    '-I', str(ROOT / 'firmware/uno-stack-test'),
                    '-I', str(ROOT / 'libraries/AndenStackLink/src'), str(source),
                    str(ROOT / 'firmware/uno-stack-test/BridgeBench.cpp'),
                    str(ROOT / 'firmware/uno-stack-test/CabControl.cpp'), *library_sources(),
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('PASS: real Uno UART parser, transactional malformed-command rejection, filtered cab selection, dedicated broadcast STOP, speed cap, timeout, roster/direction changes and default-off outputs.')
