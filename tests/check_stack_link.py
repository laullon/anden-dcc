#!/usr/bin/env python3
"""Exercise the shared UART framing against corruption and startup noise."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
include=root/'libraries/AndenStackLink/src'
with tempfile.TemporaryDirectory(prefix='anden-stack-link-') as tmp:
    source=Path(tmp)/'check.cpp'
    source.write_text(r'''
#include <cassert>
#include <cstring>
#include <string>
#include "AndenStackLink.h"
using namespace AndenLink;
bool receive(Receiver& rx,const std::string& wire) {
  bool result=false;for(char c:wire)if(rx.feed(c))result=true;return result;
}
int main() {
  assert(crc16("123456789",9)==0x29B1);
  char wire[MAX_BODY+8];
  size_t size=encode(wire,sizeof(wire),"AD1,42,LED");assert(size);
  Receiver rx;
  assert(!receive(rx,std::string(wire,size-1)));
  assert(rx.feed('\n'));assert(!strcmp(rx.body,"AD1,42,LED"));
  assert(rx.validFrames==1);
  // Every single-bit corruption of the command body must be rejected.
  for(size_t i=1;i<size-6;++i)for(unsigned bit=0;bit<8;++bit) {
    Receiver bad;std::string corrupt(wire,size);corrupt[i]^=char(1<<bit);
    assert(!receive(bad,corrupt));
    assert(receive(bad,std::string(wire,size))); // resynchronizes afterward
  }
  Receiver noise;
  assert(!receive(noise,std::string("ROM boot noise\xff\r\n")));
  assert(!receive(noise,"@"+std::string(180,'x')+"\n"));
  assert(receive(noise,std::string(wire,size)));assert(noise.badFrames>0);
  Receiver repeated;
  assert(receive(repeated,std::string(wire,size)+std::string(wire,size)));
  assert(repeated.validFrames==2);
  std::string maximum(MAX_BODY,'x');
  size=encode(wire,sizeof(wire),maximum.c_str());assert(size);
  Receiver max;assert(receive(max,std::string(wire,size)));
  assert(!encode(wire,sizeof(wire),(maximum+'x').c_str()));
  assert(!encode(wire,8,"AD1,1,PING"));
  assert(!encode(wire,sizeof(wire),"AD1,1,LED*BAD"));
  uint16_t numberValue;
  assert(number("65535",numberValue)&&numberValue==65535);
  assert(!number("65536",numberValue));assert(!number("-1",numberValue));
  assert(!number("1x",numberValue));assert(!number("",numberValue));
  char command[]="AD1,42,PING";char* parts[3];
  assert(fields(command,parts,3)==3);assert(!strcmp(parts[2],"PING"));
  char extra[]="AD1,42,PING,extra";assert(fields(extra,parts,3)==0);
  char empty[]="AD1,,PING";assert(fields(empty,parts,3)==0);
}
''')
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-I',str(include),
                    str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: stack UART CRC vector, corrupted commands, noise/overflow recovery, boundaries and numeric fields.')
