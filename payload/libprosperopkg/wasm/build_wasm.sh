#!/usr/bin/env bash
# Compiles libprosperopkg (the portable PKG-building engine that also runs
# on the PS5 payload) into WebAssembly, so the exact same PKG-building code
# can run inside a desktop/Android browser with no PS5 or payload needed.
#
# Output: www/wasm/prosperopkg.js + www/wasm/prosperopkg.wasm
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT_DIR="$SCRIPT_DIR/../../../www/wasm"
mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"

# Emscripten's default cache lives inside the read-only Nix store on Replit,
# so a plain build fails trying to write newly-built system libs there.
# Work around it with a small writable overlay cache dir:
#   - sysroot/ is symlinked straight to the Nix store's prebuilt sysroot
#     (read-only access is all we need for headers + prebuilt libs)
#   - symbol_lists/ and build/ are real writable dirs/files, seeded from
#     the Nix store copy so nothing needs to be rebuilt from scratch.
# -g -O0 additionally makes emcc pick the prebuilt *-debug.a system library
# variants (which Nix already ships), so no system libs need compiling.
NIX_EMCC="$(readlink -f "$(command -v emcc)")"
NIX_EM_SHARE="$(cd "$(dirname "$NIX_EMCC")/../share/emscripten" && pwd)"
NIX_CACHE="$NIX_EM_SHARE/cache"

EM_CACHE="/tmp/emscripten-overlay-cache"
if [ ! -e "$EM_CACHE/sysroot" ]; then
    rm -rf "$EM_CACHE"
    mkdir -p "$EM_CACHE/symbol_lists" "$EM_CACHE/build"
    ln -s "$NIX_CACHE/sysroot" "$EM_CACHE/sysroot"
    cp "$NIX_CACHE/sysroot_install.stamp" "$EM_CACHE/sysroot_install.stamp"
    cp "$NIX_CACHE"/symbol_lists/*.json "$EM_CACHE/symbol_lists/" 2>/dev/null || true
fi
export EM_CACHE

emcc \
    -g -O0 \
    -std=gnu11 \
    -I "$LIB_DIR/include" \
    "$LIB_DIR/src/pkg_builder.c" \
    "$LIB_DIR/src/pfs_builder.c" \
    "$LIB_DIR/src/pfs_directory.c" \
    "$LIB_DIR/src/pfs_inode.c" \
    "$LIB_DIR/src/npdrm.c" \
    "$LIB_DIR/src/utils.c" \
    "$SCRIPT_DIR/wasm_api.c" \
    -o "$OUT_DIR/prosperopkg.js" \
    -s MODULARIZE=1 \
    -s EXPORT_NAME=ProsperoPkgModule \
    -s EXPORTED_FUNCTIONS='["_wasm_pkg_build","_malloc","_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","FS"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s FORCE_FILESYSTEM=1 \
    -s ENVIRONMENT=web

echo "Built $OUT_DIR/prosperopkg.js + prosperopkg.wasm"
