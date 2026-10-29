#!/usr/bin/env python3
"""Check the real C++ encoder and assembled PIO opcodes against DCC frames."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SKETCH = ROOT / 'firmware/anden-dcc'

# Execute the actual encoder on the host, keeping expected frame construction
# separate from its packed-word implementation.
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'codec.cpp'
    executable = Path(directory) / 'codec'
    source.write_text('''#include "DccPacket.h"
#include <cstdio>
int main() {
  for (int address = 1; address <= 127; ++address)
    for (bool on : {false, true})
      std::printf("%d %d %u\\n", address, on, DccPacket::word(address, on));
}
'''.replace('#include <cstdio>', '#include <cstdio>\n#include <initializer_list>'))
    subprocess.run(['c++', '-std=c++17', '-I', str(SKETCH), str(source), '-o', str(executable)], check=True)
    rows = subprocess.check_output([str(executable)], text=True).splitlines()

header = (SKETCH / 'dcc_tx.pio.h').read_text()
section = header.split('dcc_tx_program_instructions[] = {', 1)[1].split('};', 1)[0]
program = [int(value, 16) for value in re.findall(r'0x([0-9a-fA-F]{4}),', section)]


def simulate(words, frames=3):
    """Small RP2040 instruction model; time comes from machine delay fields."""
    fifo = list(words)
    x = y = isr = osr = 0
    pc = cycles = pin = 0
    edges = []
    for _ in range(100000):
        instruction = program[pc]
        operation = instruction >> 13
        argument = instruction & 0xff
        following = (pc + 1) % len(program)
        if operation == 4:  # pull noblock: fallback copies X into OSR
            assert argument == 0x80
            osr = fifo.pop(0) if fifo else x
        elif operation == 5:  # mov, including nop (mov y,y)
            source = {1: x, 2: y, 6: isr, 7: osr}[argument & 7]
            assert (argument >> 3) & 3 == 0  # no invert/reverse
            destination = argument >> 5
            if destination == 1: x = source
            elif destination == 2: y = source
            elif destination == 6: isr = source
            else: raise AssertionError('Unexpected MOV')
        elif operation == 7:  # set
            destination, value = argument >> 5, argument & 31
            if destination == 0:
                if value != pin:
                    edges.append((cycles * 2, value))
                    pin = value
            elif destination == 1: x = value
            else: raise AssertionError('Unexpected SET')
        elif operation == 3:  # out y,1, MSB first
            assert argument == 0x41
            y = osr >> 31
            osr = (osr << 1) & 0xffffffff
        elif operation == 0:
            condition, target = argument >> 5, argument & 31
            if condition == 0: taken = True
            elif condition == 2:
                taken = x != 0
                x = (x - 1) & 0xffffffff
            elif condition == 3: taken = y == 0
            else: raise AssertionError('Unexpected JMP')
            if taken: following = target
        else: raise AssertionError('Unexpected PIO instruction')
        cycles += 1 + ((instruction >> 8) & 31)
        pc = following
        if len(edges) >= frames * 44 * 2 + 1: break
    assert len(edges) >= frames * 44 * 2 + 1
    bits = []
    for index in range(frames * 44):
        high = edges[index * 2 + 1][0] - edges[index * 2][0]
        low = edges[index * 2 + 2][0] - edges[index * 2 + 1][0]
        assert high == low and high in (58, 100), (index, high, low)
        bits.append(int(high == 58))
    return [bits[i:i+44] for i in range(0, len(bits), 44)]


def expected(address, on):
    instruction = 0x90 if on else 0x80
    data = [address, instruction, address ^ instruction]
    frame = [1] * 16
    for byte in data:
        frame += [0] + [(byte >> shift) & 1 for shift in range(7, -1, -1)]
    return frame + [1]


words = {}
for row in rows:
    address, on, word = map(int, row.split())
    words[address, on] = word
    assert simulate([word]) == [expected(address, on)] * 3
# FIFO command update and indefinite fallback must switch only at a boundary.
assert simulate([words[3, 0], words[3, 1]]) == [expected(3, 0), expected(3, 1), expected(3, 1)]
assert words[3, 0] == 0x034020c0
assert words[3, 1] == 0x034824c0
print('PASS: 254 address/F0 combinations; repeat and boundary update; all half-bits exactly 58/100 us.')

# Exercise the production roster and feed its real multi-address schedule into
# the assembled-PIO model above. This catches loss of another loco's state.
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'roster.cpp'
    executable = Path(directory) / 'roster'
    source.write_text(r'''
#include "LocoRoster.h"
#include <cassert>
#include <cstdio>
#include <string>
using Result = LocoRoster::Result;
void output(const LocoRoster& roster) {
  uint32_t words[8]; roster.schedule(words);
  for (auto word : words) std::printf("%u ", word);
  std::puts("");
}
int main() {
  LocoRoster r;
  output(r); // Empty roster generates idle packets, never stale loco state.
  assert(r.save(0, 3, "Evening express") == Result::Ok);
  assert(r.save(0, 4, "Shunter") == Result::Ok);
  r.entries[0].f0 = true;
  assert(!r.entries[1].f0);
  output(r);
  assert(r.save(0, 127, "Third train") == Result::Ok);
  r.entries[2].f0 = true;
  output(r);
  assert(r.save(1, 3, "Renamed") == Result::Ok && r.entries[0].f0);
  assert(r.save(1, 4, "Duplicate") == Result::Duplicate && r.entries[0].address == 3);
  assert(r.save(1, 5, "New decoder") == Result::Ok && !r.entries[0].f0);
  assert(r.entries[2].f0); // Changing one decoder cannot change another.
  assert(r.save(8, 8, "Missing") == Result::Missing);
  assert(r.save(0, 0, "Invalid") == Result::Invalid);
  assert(r.save(0, 128, "Invalid") == Result::Invalid);
  assert(r.save(0, 9, "Line\nbreak") == Result::Invalid);
  assert(r.save(0, 9, std::string(49, 'a').c_str()) == Result::Invalid);
  assert(r.remove(2)); assert(!r.remove(2));
  output(r);
  assert(r.save(0, 6, "Reused ID") == Result::Ok);
  assert(r.entries[2].id == 2);
  for (uint8_t a = 10; r.count < 8; ++a) assert(r.save(0, a, "More trains") == Result::Ok);
  assert(r.save(0, 90, "Too many") == Result::Full);
  assert(r.save(1, 5, std::string(48, 'a').c_str()) == Result::Ok);
  for (uint8_t i = 0; i < r.count; ++i) r.entries[i].f0 = i % 2;
  output(r);
  while (r.count) assert(r.remove(r.entries[0].id));
  output(r);
}
''')
    subprocess.run(['c++', '-std=c++17', '-I', str(SKETCH), str(source), '-o', str(executable)], check=True)
    schedules = subprocess.check_output([str(executable)], text=True).splitlines()

for schedule in schedules:
    ring = list(map(int, schedule.split()))
    assert len(ring) == 8
    wanted = []
    for word in ring:
        address, instruction = word >> 24, (word >> 15) & 255
        frame = [1] * 16
        for byte in [address, instruction, address ^ instruction]:
            frame += [0] + [(byte >> shift) & 1 for shift in range(7, -1, -1)]
        wanted.append(frame + [1])
    assert simulate(ring * 2, frames=16) == wanted * 2
print('PASS: roster independence, rename/address reset, duplicates, limits, removal/ID reuse, empty idle and 2/3/8-loco PIO schedules.')
