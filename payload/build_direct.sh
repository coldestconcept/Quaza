#!/bin/sh
# Direct build script -- no cmake, no try-compile probes.
# Compiles every .c file with clang then links with ld.lld.
set -e

SDK="$HOME/ps5-payload-sdk"
PAYLOAD="$HOME/Quaza/payload"
BUILD="$PAYLOAD/build_direct"

CLANG="clang"
LDLLD="ld.lld"

# Correct SDK layout:
#   crt1.o  -> $SDK/crt/crt1.o
#   libc.a  -> $SDK/libc/libc.a
#   stubs   -> $SDK/sce_stubs/libSce*.so
SDK_CRT="$SDK/crt"
SDK_LIBC="$SDK/libc"
SDK_STUBS="$SDK/sce_stubs"

# tile_pkg_embed.h is pre-built on Replit and committed to the repo.
# Verify it exists so a missing header gives a clear error.
if [ ! -f "$PAYLOAD/tile_pkg_embed.h" ]; then
  echo "ERROR: tile_pkg_embed.h not found -- run 'git pull origin main' first." >&2
  exit 1
fi
echo "[info] tile_pkg_embed.h: $(wc -c < "$PAYLOAD/tile_pkg_embed.h") bytes embedded"

mkdir -p "$BUILD"

# Single-line CFLAGS -- avoids backslash-newline issues in dash/sh
CFLAGS="--target=x86_64-sie-ps5 --sysroot=$SDK -isystem $SDK/include -isystem $SDK/include/freebsd -I$PAYLOAD -I$PAYLOAD/libprosperopkg/include -O3 -std=gnu11 -ffreestanding -fno-builtin -nostdlib -fPIC -fno-plt -fno-stack-protector -Wall"

echo "==> [1/2] Compiling..."
OBJS=""
for src in "$PAYLOAD"/*.c "$PAYLOAD"/libprosperopkg/src/*.c; do
  [ -f "$src" ] || continue
  base=$(basename "$src" .c)
  obj="$BUILD/${base}.o"
  echo "  cc $base.c"
  $CLANG $CFLAGS -c "$src" -o "$obj"
  OBJS="$OBJS $obj"
done

echo "==> [2/2] Linking..."
$LDLLD -m elf_x86_64 \
  --sysroot="$SDK" \
  -pie \
  -L"$SDK_STUBS" \
  -L"$SDK_CRT" \
  -L"$SDK_LIBC" \
  "$SDK_CRT/crt1.o" \
  $OBJS \
  "$SDK_LIBC/libc.a" \
  -lkernel \
  -lSceLibcInternal \
  -lSceSystemService \
  -lSceLncUtil \
  -lSceAppInstUtil \
  -lSceUserService \
  -lSceSysmodule \
  -o "$BUILD/quaza_payload.elf"

llvm-objcopy -O binary "$BUILD/quaza_payload.elf" "$BUILD/quaza_payload.bin"

echo ""
echo "Done!"
echo "  ELF: $BUILD/quaza_payload.elf"
echo "  BIN: $BUILD/quaza_payload.bin"
