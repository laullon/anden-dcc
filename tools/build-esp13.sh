#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
cli=${ARDUINO_CLI:-arduino-cli}
python3 firmware/esp13-ota/embed.py
python3 tests/check_cpp_interfaces.py
python3 tests/check_esp13.py
python3 tests/check_stack_link.py
python3 tests/check_uno_dcc.py
python3 tests/check_functions.py
"$cli" compile --libraries libraries --fqbn esp8266:esp8266:generic:eesz=1M64,FlashMode=dout,xtal=80,baud=115200 \
  --output-dir build/esp13-ota firmware/esp13-ota
python3 - <<'PY'
import hashlib, json, re, sys
from pathlib import Path
root = Path.cwd()
sys.path.insert(0, str(root / 'firmware/esp13-ota'))
from ota import validate_image
image = root / 'build/esp13-ota/esp13-ota.ino.bin'
data = image.read_bytes()
validate_image(data)
source = (root / 'firmware/esp13-ota/esp13-ota.ino').read_text()
manifest = {
    'target': 'anden-esp13',
    'version': re.search(r'VERSION\[\]\s*=\s*"([^"]+)"', source)[1],
    'core': 'esp8266:esp8266@3.1.2',
    'fqbn': 'esp8266:esp8266:generic:eesz=1M64,FlashMode=dout,xtal=80,baud=115200',
    'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
}
(image.parent / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('ESP13 artifact validated:', len(data), 'bytes; SHA-256', manifest['sha256'])
PY
