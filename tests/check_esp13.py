#!/usr/bin/env python3
"""Check ESP13 settings boundaries and reject incompatible OTA artifacts."""
from pathlib import Path
import os
import contextlib
import io
import json
from unittest.mock import patch
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
experiment = root / 'experiments/esp13-ota'
subprocess.run([sys.executable, str(experiment / 'embed.py'), '--check'], check=True)
sys.path.insert(0, str(experiment))
import ota
from ota import APP_ID, IMAGE_LIMIT, validate_image

valid = bytes([0xE9, 1, 3, 0x20]) + APP_ID
validate_image(valid)
for invalid in (b'', b'UF2'+APP_ID, bytes([0xE9, 1, 3, 0x40])+APP_ID,
                bytes([0xE9, 1, 3, 0x20])+b'other app', valid+b'\0'*IMAGE_LIMIT):
    try:
        validate_image(invalid)
    except ValueError:
        pass
    else:
        raise AssertionError('Accepted an incompatible image')
try:
    validate_image(valid, len(valid)-1)
except ValueError:
    pass
else:
    raise AssertionError('Ignored target update capacity')

# Exercise the real helper entry point: an old boot with the expected version
# must not be mistaken for a successful restart, and a Pico must never get POST.
with tempfile.TemporaryDirectory(prefix='anden-esp13-helper-') as tmp:
    image = Path(tmp) / 'esp13.bin'
    image.write_bytes(valid)
    target = dict(name='anden-esp13', platform='esp8266', appId=APP_ID.decode(),
                  version='0.1.0', layoutBytes=1048576, bootId=17, otaMaxBytes=IMAGE_LIMIT)
    calls = []
    count = [0]
    def response(url, **kwargs):
        if isinstance(url, str):
            count[0] += 1
            current = dict(target, bootId=18 if count[0] >= 3 else 17)
            return io.BytesIO(json.dumps(current).encode())
        calls.append(url)
        return io.BytesIO(b'Update Success! Rebooting...')
    argv = ['ota.py','--host','esp13.test','--file',str(image),'--expect-version','0.1.0']
    with patch.object(sys,'argv',argv), patch.object(ota.urllib.request,'urlopen',side_effect=response), \
         patch.object(ota.time,'sleep'), contextlib.redirect_stdout(io.StringIO()):
        ota.main()
    assert count[0] == 3 and len(calls) == 1
    assert calls[0].full_url == 'http://esp13.test/update'
    assert valid in calls[0].data
    calls.clear()
    def wrong_target(url, **kwargs):
        if not isinstance(url,str):
            calls.append(url)
            raise AssertionError('Uploaded to the wrong device')
        return io.BytesIO(json.dumps(dict(target,name='anden-dcc',platform='rp2040')).encode())
    with patch.object(sys,'argv',argv), patch.object(ota.urllib.request,'urlopen',side_effect=wrong_target), \
         contextlib.redirect_stdout(io.StringIO()):
        try:
            ota.main()
        except ValueError:
            pass
        else:
            raise AssertionError('Accepted the Pico as an ESP13 target')
    assert not calls

with tempfile.TemporaryDirectory(prefix='anden-esp13-check-') as tmp:
    source = Path(tmp) / 'check.cpp'
    source.write_text(r'''
#include <cassert>
#include <string>
#include "WifiValidation.h"
#include "StatusLed.h"
int main() {
  StatusLed led;
  assert(led.on(0,false)); assert(led.on(499,false));
  assert(!led.on(500,false)); assert(!led.on(999,false)); assert(led.on(1000,false));
  assert(led.on(500,true));
  assert(!led.request("GET","/api/status",0));
  assert(!led.request("HEAD","/api/status",0));
  assert(led.requests()==0 && led.on(0,true));
  assert(led.request("GET","/",100));
  assert(!led.on(100,true)); assert(!led.on(219,true)); assert(led.on(220,true));
  assert(led.request("POST","/dcc/f0",300));
  assert(led.request("POST","/locos/save",350));
  assert(!led.on(469,true)); assert(led.on(470,true)); assert(led.requests()==3);
  // Activity expiry must remain correct through the millis() rollover.
  assert(led.request("GET","/missing",UINT32_MAX-15));
  assert(!led.on(103,true)); assert(led.on(104,true));
  assert(!led.request("GET","/api/status",200)); assert(led.requests()==4);
  assert(led.request("POST","/setup/wifi",600)); assert(!led.on(719,false));
  assert(!led.on(720,false)); assert(led.on(1000,false));
  assert(validSsid("x",1));
  std::string ssid(32,'x'); assert(validSsid(ssid.data(),ssid.size()));
  ssid+='x'; assert(!validSsid(ssid.data(),ssid.size()));
  assert(!validSsid("",0)); assert(!validSsid("a\nb",3));
  assert(!validSsid("a\0b",3));
  std::string utf8="café"; assert(validSsid(utf8.data(),utf8.size()));
  assert(validWifiKey("",0)); assert(!validWifiKey("1234567",7));
  assert(validWifiKey("12345678",8));
  std::string key(63,'z'); assert(validWifiKey(key.data(),key.size()));
  key+='z'; assert(!validWifiKey(key.data(),key.size()));
  key=std::string(64,'F'); assert(validWifiKey(key.data(),key.size()));
  key+='F'; assert(!validWifiKey(key.data(),key.size()));
  assert(!validWifiKey("1234\n678",8));
  assert(!validWifiKey("1234\0" "678",8));
}
''')
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++11', '-I', str(experiment),
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: ESP13 LED heartbeat/request pulses/quiet polling/rollover, embedded page, credential boundaries, app/platform/layout/size OTA guards, wrong-target refusal and verified-reboot helper flow.')
