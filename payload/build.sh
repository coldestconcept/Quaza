#!/bin/bash
# Quaza PS5 Payload — build script
#
# Two methods:
#   1. CMake (recommended, uses the SDK toolchain file)
#   2. Direct make (fallback — set PS5_PAYLOAD_SDK and run `make` in payload/)
#
# Prerequisites:
#   - PS5 Payload SDK installed (https://github.com/ps5-payload-dev/sdk)
#   - clang + ld.lld available on PATH
#   - CMake 3.2+  (for method 1)
#
# Termux one-liner to install dependencies:
#   pkg install clang cmake lld

set -e

# ── SDK path ─────────────────────────────────────────────────────────────────
# Override with:  PS5_PAYLOAD_SDK=/your/path ./build.sh
PS5_PAYLOAD_SDK="${PS5_PAYLOAD_SDK:-$HOME/target-sdk/ps5-payload-sdk}"

if [ ! -d "$PS5_PAYLOAD_SDK" ]; then
    echo "ERROR: PS5 Payload SDK not found at $PS5_PAYLOAD_SDK"
    echo "Install it:"
    echo "  git clone https://github.com/ps5-payload-dev/sdk.git"
    echo "  cd sdk && make && make DESTDIR=$PS5_PAYLOAD_SDK install"
    exit 1
fi

TOOLCHAIN="$PS5_PAYLOAD_SDK/toolchain/prospero.cmake"
if [ ! -f "$TOOLCHAIN" ]; then
    echo "ERROR: Toolchain file not found: $TOOLCHAIN"
    exit 1
fi

export PS5_PAYLOAD_SDK

# ── Build ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DPS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK" \
    -DCMAKE_BUILD_TYPE=Release \
    "$SCRIPT_DIR"

cmake --build . -- -j"$(nproc 2>/dev/null || echo 2)"

echo ""
echo "✓ Build complete: build/quaza_payload.elf"
file quaza_payload.elf 2>/dev/null || true
