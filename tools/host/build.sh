#!/usr/bin/env bash
# Builds the host-side renderer (tools/host/host_render.cpp) from the real
# drawing code. Needs g++ and zlib. Used by tools/preview.sh and tools/golden.sh.
# Usage: tools/host/build.sh <output-binary>
set -euo pipefail
cd "$(dirname "$0")/../.."

# tools/host/ supplies stand-ins for <Arduino.h>, <Wire.h> and <SPI.h>, which
# the vendored DEV_Config.h includes.
g++ -std=gnu++11 -O1 -w \
    -Itools/host -Ilib/WaveshareEPD -Ilib/CalendarCore -Ilib/CalendarRender \
    -Ilib/BinRender -Ilib/UiKit -Isrc \
    tools/host/host_render.cpp src/dummy_data.cpp \
    lib/CalendarCore/*.cpp lib/UiKit/*.cpp \
    lib/CalendarRender/*.cpp lib/BinRender/*.cpp \
    lib/WaveshareEPD/GUI_Paint.cpp lib/WaveshareEPD/font*.cpp \
    -lz -o "$1"
