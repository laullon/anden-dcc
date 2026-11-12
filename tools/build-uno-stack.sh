#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
cli=${ARDUINO_CLI:-arduino-cli}
python3 tools/check_core.py --cli "$cli" arduino:avr@1.8.7
python3 tests/check_cpp_interfaces.py
python3 tests/check_stack_link.py
python3 tests/check_uno_dcc.py
python3 tests/check_functions.py
python3 tests/check_bridge_bench.py
python3 tests/check_uno_bridge_route.py
python3 tests/check_uno_commands.py
python3 tests/check_cab_display.py
python3 tests/check_cab_i2c.py
python3 tests/check_uno_track.py
python3 tests/check_finder.py
"$cli" compile --fqbn arduino:avr:uno --libraries libraries \
  --output-dir build/uno-stack-test firmware/uno-stack-test
