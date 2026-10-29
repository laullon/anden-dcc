#!/usr/bin/env python3
"""Exercise actual service packet/address helpers; no simulated decoder readback."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'service.cpp'
    source.write_text(r'''
#include "ServiceMode.h"
#include <cassert>
#include <cstdio>
using namespace ServiceMode;
void check(Packet packet, uint16_t cv, uint8_t data, uint8_t command) {
  assert(packet.size == 4);
  assert(packet.bytes[0] == (0x70 | (command << 2) | ((cv - 1) >> 8)));
  assert(packet.bytes[1] == ((cv - 1) & 255));
  assert(packet.bytes[2] == data);
  assert((packet.bytes[0] ^ packet.bytes[1] ^ packet.bytes[2] ^ packet.bytes[3]) == 0);
}
int main() {
  static_assert(PREAMBLE_BITS >= 20);
  assert(!writeByte(0, 3).valid() && !writeByte(1025, 3).valid());
  assert(!bit(1, 8, false, false).valid());
  for (unsigned cv = 1; cv <= 1024; ++cv) {
    for (unsigned value = 0; value <= 255; ++value) {
      check(writeByte(cv, value), cv, value, 3);
      check(verifyByte(cv, value), cv, value, 1);
    }
    for (unsigned position = 0; position < 8; ++position)
      for (unsigned value = 0; value < 2; ++value)
        for (unsigned write = 0; write < 2; ++write) {
          auto p = bit(cv, position, value, write);
          check(p, cv, 0xe0 | (write << 4) | (value << 3) | position, 2);
        }
  }
  // Independent known vectors: CV1=3, CV29 bit5 clear, CV1024=255.
  auto p = writeByte(1,3);assert(p.bytes[0]==0x7c&&p.bytes[1]==0&&p.bytes[2]==3&&p.bytes[3]==0x7f);
  p=bit(29,5,false,true);assert(p.bytes[0]==0x78&&p.bytes[1]==0x1c&&p.bytes[2]==0xf5&&p.bytes[3]==0x91);
  p=verifyByte(1024,255);assert(p.bytes[0]==0x77&&p.bytes[1]==0xff&&p.bytes[2]==0xff&&p.bytes[3]==0x77);
  assert(reset().size == 3);
  for (unsigned cv29 = 0; cv29 <= 255; ++cv29) {
    for (unsigned value = 1; value <= 127; ++value) {
      auto plan=shortAddress(value);assert(plan.valid());
      check(plan.setAddress,1,value,3);check(plan.selectShort,29,0xf5,2);
      if (!(cv29 & 0x20)) {
        // Unrelated CV29 bits do not change short-address selection.
        auto decoded=address(cv29,value,193,44);
        assert(decoded.valid&&!decoded.extended&&decoded.value==value);
      }
    }
  }
  assert(!shortAddress(0).valid()&&!shortAddress(128).valid());
  assert(!address(0,0,0,0).valid&&!address(0,128,0,0).valid);
  for (unsigned value=1; value<=10239; ++value) {
    auto result=address(0x20,0,192+(value>>8),value&255);
    assert(result.valid&&result.extended&&result.value==value);
  }
  assert(!address(0x20,3,191,255).valid&&!address(0x20,3,232,0).valid);
  assert(!address(0x20,3,192,0).valid);
  assert(verifiedBit(false,false)==VerifyResult::NoAck);
  assert(verifiedBit(true,true)==VerifyResult::Ambiguous);
  assert(verifiedBit(true,false)==VerifyResult::Zero);
  assert(verifiedBit(false,true)==VerifyResult::One);
  std::puts("PASS: service byte/bit packets for every CV, checksums/vectors, short-address plans, long-address decoding and explicit no-ACK/ambiguous states. No hardware I/O is implemented by these helpers.");
}
''')
    executable = Path(directory) / 'service'
    subprocess.run(['c++', '-std=c++17', '-O2', '-I', str(root/'firmware/anden-dcc'), str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
