#!/usr/bin/env bash
# Layout regression check. Renders every scenario and compares it, pixel for
# pixel, with the reference image in test/golden/.
#
# Usage: tools/golden.sh             check; exits 1 on any difference and writes the
#                                    actual images to golden-actual/ to look at
#        tools/golden.sh --update    accept the current rendering as the new reference
#
# After an intended layout change, run --update, look at the changed images in
# test/golden/ (git shows image diffs), and commit them with the change.
# Needs g++ and zlib.
set -euo pipefail
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

bash tools/host/build.sh "$TMP/host_render"
NAMES="$("$TMP/host_render" --list | tr -d '\r')"

if [ "${1:-}" = "--update" ]; then
    mkdir -p test/golden
    for name in $NAMES; do
        "$TMP/host_render" "$name" "test/golden/$name.png" | tr -d '\r'
    done
    echo "reference images updated"
    exit 0
fi

rm -rf golden-actual
mkdir -p golden-actual
failed=0
for name in $NAMES; do
    if ! "$TMP/host_render" --check "$name" "test/golden/$name.png" "golden-actual/$name.png" | tr -d '\r'; then
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    echo
    echo "Layout differs from the reference images. The actual renderings are in"
    echo "golden-actual/. If the change is intended, run: tools/golden.sh --update"
    exit 1
fi
rmdir golden-actual
echo "all screens match their reference images"
