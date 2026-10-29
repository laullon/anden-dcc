#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
cli=${ARDUINO_CLI:-arduino-cli}
python3 tools/embed_web.py
python3 tests/check_dcc.py
"$cli" compile --fqbn rp2040:rp2040:rpipicow:flash=2097152_1048576 --output-dir build firmware/anden-dcc
