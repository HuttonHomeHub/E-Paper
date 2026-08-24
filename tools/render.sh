#!/usr/bin/env bash
# Renders the calendar screen to a PNG using the same code the ESP32 runs.
# Usage: tools/render.sh [output.png]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-calendar.png}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# The stub stands in for <Arduino.h>, which DEV_Config.h includes.
cp tools/arduino_stub.h "$TMP/Arduino.h"
printf '#include <Arduino.h>\n' > "$TMP/Wire.h"
printf '#include <Arduino.h>\n' > "$TMP/SPI.h"

g++ -std=gnu++11 -O1 -w \
    -I"$TMP" -Ilib/WaveshareEPD -Isrc \
    tools/host_render.cpp src/calendar_render.cpp src/dummy_data.cpp \
    lib/WaveshareEPD/GUI_Paint.cpp lib/WaveshareEPD/font*.cpp \
    -lz -o "$TMP/host_render"

"$TMP/host_render" "$OUT"
