#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
"$root/tools/build-uno-stack.sh"
"$root/tools/build-esp13.sh"
