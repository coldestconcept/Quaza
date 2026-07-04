#!/bin/sh
# build_direct.sh — Quaza PS5 payload direct build (no cmake)
set -e

PAYLOAD="$(cd "$(dirname "$0")" && pwd)"
SDK="$HOME/ps5-payload-sdk"
SDK_STUBS="$SDK/sce_stubs"
SDK_CRT="$SDK/crt"
SDK_LIBC="$SDK/libc"
BUILD="$PAYLOAD/build_direct"
CLANG="clang"
LDLLD="ld.lld"

mkdir -p "$BUILD"

if [ ! -f "$PAYLOAD/tile_pkg_embed.h" ]; then
  echo "[error] tile_pkg_embed.h not found" >&2
  exit 1
fi
echo "[info] tile_pkg_embed.h: $(wc -c < "$PAYLOAD/tile_pkg_embed.h") bytes"

# =============================================================================
# Restore SDK stubs that were overwritten by a previous bad build run.
# Earlier runs rebuilt libSceLibcInternal.so etc. with minimal stubs,
# removing symbols like printf/memset/calloc that libc.a depends on.
# git checkout restores them to the SDK originals; harmless if already good.
# =============================================================================
echo "==> [0a/3] Restoring SDK stubs via git..."
if git -C "$SDK" checkout -- sce_stubs/ 2>/dev/null; then
  echo "  restored from SDK git"
else
  echo "  SDK is not a git repo — downloading stubs from GitHub..."
  SDK_RAW="https://raw.githubusercontent.com/ps5-payload-dev/sdk/main/sce_stubs"
  for lib in libSceLibcInternal.so libkernel.so libSceUserService.so libSceSysmodule.so; do
    curl -fsSL "$SDK_RAW/$lib" -o "$SDK_STUBS/$lib" && echo "  fetched $lib" || echo "  warn: could not fetch $lib"
  done
fi

# =============================================================================
# [0/3] Fix the two hand-built stubs that have wrong SONAME
#
# The SDK already provides correct stubs for:
#   libkernel.so        -> SONAME: libkernel.sprx
#   libSceLibcInternal  -> SONAME: libSceLibcInternal.sprx
#   libSceUserService   -> SONAME: libSceUserService.sprx
#   libSceSysmodule     -> SONAME: libSceSysmodule.sprx
#
# These two were hand-built without -Wl,-soname so their SONAME was wrong:
#   libSceSystemService.so  had SONAME: libSceSystemService.so  (WRONG)
#   libSceAppInstUtil.so    had SONAME: libSceAppInstUtil.so    (WRONG)
#
# Rebuild only those two with the correct .sprx SONAME.
# Two-step compile: clang -c -> ld.lld -shared, avoids Termux linker PATH issue.
# =============================================================================
fix_stub() {
  soname="$1"; out="$SDK_STUBS/$2"; shift 2
  tmp="${TMPDIR:-$BUILD}"
  src="$tmp/_fix_stub_$$.c"
  obj="$tmp/_fix_stub_$$.o"
  printf '' > "$src"
  for s; do printf 'void %s(void){}\n' "$s" >> "$src"; done
  $CLANG --target=x86_64-sie-ps5 --sysroot="$SDK" \
    -fPIC -nostdlib -ffreestanding -c "$src" -o "$obj"
  $LDLLD -m elf_x86_64 -shared -nostdlib \
    "--soname=$soname" -o "$out" "$obj"
  rm -f "$src" "$obj"
  echo "  fixed $soname"
}

echo "==> [0/3] Fixing stub SONAMEs..."
fix_stub libSceSystemService.sprx libSceSystemService.so \
  sceSystemServiceInitialize sceSystemServiceLaunchApp \
  sceSystemServiceLaunchWebBrowser sceSystemServiceParamGetString

fix_stub libSceAppInstUtil.sprx libSceAppInstUtil.so \
  sceAppInstUtilInitialize sceAppInstUtilAppInstallPkg \
  sceAppInstUtilAppRecover sceAppInstUtilAppUnInstall \
  sceAppInstUtilGetAppInfo

# =============================================================================
# [1/3] Compile
# =============================================================================
CFLAGS="--target=x86_64-sie-ps5 --sysroot=$SDK -isystem $SDK/include -isystem $SDK/include/freebsd -I$PAYLOAD -I$PAYLOAD/libprosperopkg/include -O2 -std=gnu11 -ffreestanding -fno-builtin -nostdlib -fPIC -fno-plt -fno-stack-protector -Wall -Wno-incompatible-pointer-types"

echo "==> [1/3] Compiling..."
OBJS=""
for src in "$PAYLOAD"/*.c "$PAYLOAD"/libprosperopkg/src/*.c; do
  if [ ! -f "$src" ]; then continue; fi
  base=$(basename "$src" .c)
  obj="$BUILD/${base}.o"
  echo "  cc $base.c"
  $CLANG $CFLAGS -c "$src" -o "$obj"
  OBJS="$OBJS $obj"
done

# =============================================================================
# [2/3] Link
# Same as the working 448KB build except:
#   - removed -lSceLncUtil (none of the reference payloads use it)
# =============================================================================
echo "==> [2/3] Linking..."
$LDLLD -m elf_x86_64 \
  --sysroot="$SDK" \
  -pie \
  -L"$SDK_STUBS" \
  "$SDK_CRT/crt1.o" \
  $OBJS \
  "$SDK_LIBC/libc.a" \
  -lkernel \
  -lSceLibcInternal \
  -lSceAppInstUtil \
  -lSceSystemService \
  --allow-shlib-undefined \
  -o "$BUILD/quaza_payload.elf"

echo "==> [3/3] Done!"
echo "  ELF: $BUILD/quaza_payload.elf"
ls -lh "$BUILD/quaza_payload.elf"
