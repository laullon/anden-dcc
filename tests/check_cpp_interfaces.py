#!/usr/bin/env python3
"""Compile public interfaces alone and link them from independent consumers."""
from pathlib import Path
import os
import subprocess
import tempfile
from native_support import ROOT, library_sources

headers = [p.name for p in (ROOT / 'libraries/AndenStackLink/src').glob('*.h')]
headers += ['BrowserLease.h', 'FavoriteFunctions.h', 'StatusLed.h', 'WifiValidation.h',
            'BridgeBench.h', 'UnoDcc.h', 'CabI2c.h', 'CabDisplay.h', 'CabControl.h',
            'FirmwareImage.h', 'RosterStorage.h']
includes = ['-I', str(ROOT / 'libraries/AndenStackLink/src'),
            '-I', str(ROOT / 'firmware/esp13-ota'),
            '-I', str(ROOT / 'firmware/uno-stack-test')]
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='anden-interfaces-') as directory:
    folder = Path(directory)
    for header in headers:
        source = folder / 'standalone.cpp'
        source.write_text('#include "' + header + '"\n')
        subprocess.run([compiler, '-std=c++11', '-Wall', '-Wextra', '-Werror',
                        *includes, '-fsyntax-only', str(source)], check=True)
    shared = ''.join('#include "' + name + '"\n' for name in headers)
    (folder / 'first.cpp').write_text(shared + '''
BrowserLease makeLease() {
    BrowserLease lease;
    lease.seed(41);
    lease.start(1000);
    return lease;
}
uint16_t frameCrc() { return AndenLink::crc16("123456789", 9); }
''')
    (folder / 'second.cpp').write_text(shared + '''
#include <cassert>
BrowserLease makeLease();
uint16_t frameCrc();
int main() {
    auto lease = makeLease();
    assert(lease.accepts(42, 1001));
    assert(frameCrc() == AndenLink::crc16("123456789", 9));
    LocoRoster roster;
    assert(roster.save(0, 3, "Test") == LocoRoster::Result::Ok);
    FavoriteFunctions favorites;
    assert(favorites.save(1, 0, "Headlights"));
    StatusLed led;
    assert(led.on(1000, true));
    assert(validSsid("Test", 4));
    BridgeBench bench;
    assert(bench.start(BridgeBench::Track, 0, 0, true));
}
''')
    sources = library_sources() + [str(ROOT / 'firmware/esp13-ota' / (name + '.cpp'))
                                  for name in ['BrowserLease', 'FavoriteFunctions', 'StatusLed',
                                               'WifiValidation', 'FirmwareImage']]
    sources.append(str(ROOT / 'firmware/uno-stack-test/BridgeBench.cpp'))
    executable = folder / 'interfaces'
    subprocess.run([compiler, '-std=c++11', '-Wall', '-Wextra', '-Werror', *includes,
                    str(folder / 'first.cpp'), str(folder / 'second.cpp'), *sources,
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('PASS: standalone public headers and independent consumers link production C++ objects without duplicate definitions.')
