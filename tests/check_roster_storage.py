#!/usr/bin/env python3
"""Exercise production filesystem initialization against fake flash/LittleFS."""
from native_support import ROOT
from pathlib import Path
import os
import subprocess
import tempfile

with tempfile.TemporaryDirectory(prefix='anden-storage-') as directory:
    folder = Path(directory)
    (folder / 'Arduino.h').write_text(r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
struct FakeEsp {
    uint32_t size = 4096;
    uint32_t getFlashChipRealSize() { return size; }
};
extern FakeEsp ESP;
void yield();
''')
    (folder / 'flash_hal.h').write_text(r'''
#pragma once
#include <stdint.h>
extern uint32_t filesystemStart, filesystemEnd;
#define FS_start filesystemStart
#define FS_end filesystemEnd
#define FS_PHYS_ADDR ((uint32_t)FS_start - 0x40200000)
#define FS_PHYS_SIZE ((uint32_t)(FS_end - FS_start))
#define FLASH_HAL_OK 0
int32_t flash_hal_read(uint32_t address, uint32_t size, uint8_t* bytes);
''')
    (folder / 'LittleFS.h').write_text(r'''
#pragma once
#include <deque>
#include <cassert>
struct LittleFSConfig {
    bool autoFormat = true;
    void setAutoFormat(bool enabled) { autoFormat = enabled; }
};
struct FakeLittleFS {
    std::deque<bool> mounts;
    unsigned begins = 0, formats = 0;
    bool configResult = true, formatResult = true;
    bool setConfig(const LittleFSConfig& value) { assert(!value.autoFormat); return configResult; }
    bool begin() { ++begins; assert(!mounts.empty()); bool ok=mounts.front();mounts.pop_front();return ok; }
    bool format() { ++formats;return formatResult; }
};
extern FakeLittleFS LittleFS;
''')
    source = folder / 'storage.cpp'
    source.write_text(r'''
#include "RosterStorage.h"
#include "Arduino.h"
#include "LittleFS.h"
#include "flash_hal.h"
#include <algorithm>
#include <vector>
#include <cstring>
FakeEsp ESP;
FakeLittleFS LittleFS;
uint32_t filesystemStart,filesystemEnd;
std::vector<uint8_t> flash;
int failedRead = -1;
unsigned reads = 0, yields = 0;
void yield() { ++yields; }
int32_t flash_hal_read(uint32_t address, uint32_t size, uint8_t* bytes) {
    assert(address>=FS_PHYS_ADDR&&size<=FS_PHYS_SIZE-(address-FS_PHYS_ADDR));
    assert(address+size<=flash.size());
    if(int(reads++)==failedRead)return -1;
    std::memcpy(bytes,flash.data()+address,size);
    return FLASH_HAL_OK;
}
void reset() {
    LittleFS = {};ESP.size=4096;
    filesystemStart=0x40200100;filesystemEnd=filesystemStart+527;
    flash.assign(4096,0); // Existing bytes outside the FS must not prevent first-use initialization.
    std::fill(flash.begin()+FS_PHYS_ADDR,flash.begin()+FS_PHYS_ADDR+FS_PHYS_SIZE,0xFF);
    failedRead=-1;reads=yields=0;
    LittleFS.mounts={false,true};
}
int main() {
    reset();LittleFS.mounts={true};assert(mountRosterStorage());
    assert(LittleFS.begins==1&&!LittleFS.formats&&!reads);
    reset();assert(mountRosterStorage());
    assert(LittleFS.begins==2&&LittleFS.formats==1&&reads==5&&yields==5);
    for(unsigned offset:{0u,127u,128u,263u,511u,526u}) {
        reset();flash[FS_PHYS_ADDR+offset]=0xFE;
        assert(!mountRosterStorage());assert(LittleFS.begins==1&&!LittleFS.formats);
        assert(flash[FS_PHYS_ADDR+offset]==0xFE);
    }
    for(unsigned chunk:{0u,1u,4u}) {
        reset();failedRead=chunk;assert(!mountRosterStorage());
        assert(LittleFS.begins==1&&!LittleFS.formats);
    }
    reset();LittleFS.formatResult=false;assert(!mountRosterStorage());
    assert(LittleFS.begins==1&&LittleFS.formats==1);
    reset();LittleFS.mounts={false,false};assert(!mountRosterStorage());
    assert(LittleFS.begins==2&&LittleFS.formats==1);
    reset();LittleFS.configResult=false;assert(!mountRosterStorage());
    assert(!LittleFS.begins&&!LittleFS.formats&&!reads);
    reset();filesystemEnd=filesystemStart;assert(!mountRosterStorage());assert(!LittleFS.formats&&!reads);
    reset();filesystemEnd=filesystemStart-1;assert(!mountRosterStorage());assert(!LittleFS.formats&&!reads);
    reset();filesystemStart=0x401FFFFF;assert(!mountRosterStorage());assert(!LittleFS.formats&&!reads);
    reset();filesystemEnd=0x40200000+ESP.size+1;assert(!mountRosterStorage());assert(!LittleFS.formats&&!reads);
}
''')
    executable = folder / 'storage'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I', str(folder), '-I', str(ROOT / 'firmware/esp13-ota'), str(source),
                    str(ROOT / 'firmware/esp13-ota/RosterStorage.cpp'), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('PASS: real filesystem initialization, erased-region-only first-use format, existing-data/read-failure preservation and invalid flash-region refusal.')
