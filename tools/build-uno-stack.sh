#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
cli=${ARDUINO_CLI:-arduino-cli}
python3 tests/check_stack_link.py
python3 tests/check_uno_dcc.py
python3 tests/check_functions.py
python3 tests/check_bridge_bench.py
python3 tests/check_uno_bridge_route.py
python3 tests/check_uno_track.py
python3 tests/check_finder.py
"$cli" compile --fqbn arduino:avr:uno --libraries libraries \
  --output-dir build/uno-stack-test experiments/uno-stack-test
