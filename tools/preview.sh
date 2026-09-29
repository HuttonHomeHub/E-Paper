#!/usr/bin/env bash
# Renders a screen to a PNG using the same drawing code the ESP32 runs, so
# layout changes can be checked without flashing hardware.
# Needs g++ and zlib. Usage: tools/preview.sh [calendar|bins] [output.png]
set -euo pipefail
cd "$(dirname "$0")/.."
PAGE="${1:-calendar}"
OUT="${2:-preview-$PAGE.png}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# tools/host/ supplies stand-ins for <Arduino.h>, <Wire.h> and <SPI.h>, which
# the vendored DEV_Config.h includes.
g++ -std=gnu++11 -O1 -w \
    -Itools/host -Ilib/WaveshareEPD -Ilib/CalendarCore -Ilib/CalendarRender \
    -Ilib/BinRender -Ilib/UiKit -Isrc \
    tools/host/host_render.cpp src/dummy_data.cpp \
    lib/CalendarCore/*.cpp lib/UiKit/*.cpp \
    lib/CalendarRender/*.cpp lib/BinRender/*.cpp \
    lib/WaveshareEPD/GUI_Paint.cpp lib/WaveshareEPD/font*.cpp \
    -lz -o "$TMP/preview"

"$TMP/preview" "$PAGE" "$OUT"
