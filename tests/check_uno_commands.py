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
#define A0 14
#define A1 15
extern uint32_t clockMs;
extern uint8_t pins[20];
inline uint32_t millis() { return clockMs; }
inline void pinMode(uint8_t, uint8_t) {}
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
#include <cassert>
uint32_t clockMs = 0;
uint8_t pins[20] = {};
FakeSerial Serial;
namespace UnoDcc {
UnoDccStream schedule;
bool running = false;
void begin() { running = true; }
bool isRunning() { return running; }
bool guardExpired() { return false; }
bool bridgeStartPending() { return false; }
bool bridgeIsActive() { return pins[3]; }
void startStopped() {}
bool armBridge(bool) { pins[3] = 1; return true; }
void disarmBridge() { pins[3] = 0; }
void renewGuard() {}
Status status() { const auto& b = schedule.banks[schedule.active]; return {b.count,b.mask,b.signature,0,0,running}; }
bool set(uint8_t count, const uint8_t* addresses, uint8_t mask) { return schedule.prepare(count, addresses, mask); }
bool control(uint8_t count, const uint8_t* addresses, uint8_t mask, uint8_t directions, const uint8_t* speeds) {
    return schedule.prepareControl(count, addresses, mask, directions, speeds);
}
bool functions(uint8_t count, const uint8_t* addresses, uint8_t directions, const uint32_t* flags) {
    return schedule.prepareFunctions(count, addresses, directions, flags);
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
}
''')
    executable = folder / 'commands'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-I', str(folder),
                    '-I', str(ROOT / 'firmware/uno-stack-test'),
                    '-I', str(ROOT / 'libraries/AndenStackLink/src'), str(source),
                    str(ROOT / 'firmware/uno-stack-test/BridgeBench.cpp'), *library_sources(),
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('PASS: real Uno UART parser, transactional malformed-command rejection, stationary control readiness and default-off outputs.')
