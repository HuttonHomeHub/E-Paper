#!/usr/bin/env bash
# Runs the host-side checks on the calendar date maths.
# Usage: tools/test.sh
set -euo pipefail
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cp tools/arduino_stub.h "$TMP/Arduino.h"
printf '#include <Arduino.h>\n' > "$TMP/Wire.h"
printf '#include <Arduino.h>\n' > "$TMP/SPI.h"

g++ -std=gnu++11 -w -I"$TMP" -Isrc -Ilib/WaveshareEPD \
    tools/date_test.cpp src/calendar_render.cpp \
    lib/WaveshareEPD/GUI_Paint.cpp lib/WaveshareEPD/font*.cpp \
    -o "$TMP/date_test"
"$TMP/date_test"
