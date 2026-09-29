#!/usr/bin/env bash
# Renders a screen to a PNG using the same drawing code the ESP32 runs, so
# layout changes can be checked without flashing hardware.
# Needs g++ and zlib.
#
# Usage: tools/preview.sh [scenario] [output.png]
#        tools/preview.sh --list          show the scenario names
# The default scenario is "calendar"; "bins" is the bin page with sample data.
# The default output is preview-<scenario>.png.
set -euo pipefail
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

bash tools/host/build.sh "$TMP/host_render"

if [ "${1:-}" = "--list" ]; then
    "$TMP/host_render" --list | tr -d '\r'
    exit 0
fi

SCENARIO="${1:-calendar}"
"$TMP/host_render" "$SCENARIO" "${2:-preview-$SCENARIO.png}"
