#!/bin/sh
# build_direct.sh — Quaza PS5 payload direct build (no cmake)
# Builds stubs fresh every run with correct .sprx SONAME, then compiles and links.
set -e

PAYLOAD="$(cd "$(dirname "$0")" && pwd)"
SDK="$HOME/ps5-payload-sdk"
SDK_STUBS="$SDK/sce_stubs"
SDK_CRT="$SDK/crt"
SDK_LIBC="$SDK/libc"
BUILD="$PAYLOAD/build_direct"
CLANG="clang"
LDLLD="ld.lld"

mkdir -p "$BUILD" "$SDK_STUBS"

if [ ! -f "$PAYLOAD/tile_pkg_embed.h" ]; then
  echo "[error] tile_pkg_embed.h not found" >&2
  exit 1
fi
echo "[info] tile_pkg_embed.h: $(wc -c < "$PAYLOAD/tile_pkg_embed.h") bytes"

# =============================================================================
# [0/3] Rebuild stubs with correct .sprx SONAME
#
# Root cause of ELF not loading: hand-built stubs lacked -Wl,-soname so
# DT_NEEDED entries said ".so" instead of ".sprx". The PS5 dynamic linker
# only knows .sprx and silently refuses to load the ELF.
#
# Reference ELFs (elf-arsenal, game-compressor, pldmgr) all use .sprx names.
# Rebuild every stub we use here so the result is always correct.
# =============================================================================
mkstub() {
  # mkstub <soname.sprx> <libname.so> [sym ...]
  # Two-step: clang compiles .c->.o (no link), ld.lld links .o->.so directly.
  # Avoids clang's internal linker lookup failing on Termux ($PATH issue).
  soname="$1"; out="$SDK_STUBS/$2"; shift 2
  tmp="${TMPDIR:-$BUILD}"
  src="$tmp/_ps5stub_$.c"
  obj="$tmp/_ps5stub_$.o"
  printf '' > "$src"
  for s; do printf 'void %s(void){}\n' "$s" >> "$src"; done
  $CLANG --target=x86_64-sie-ps5 --sysroot="$SDK" \
    -fPIC -nostdlib -ffreestanding -c "$src" -o "$obj"
  $LDLLD -m elf_x86_64 -shared -nostdlib \
    "--soname=$soname" -o "$out" "$obj"
  rm -f "$src" "$obj"
  echo "  stub $soname"
}

echo "==> [0/3] Building stubs..."

# kernel: pthreads, POSIX fd I/O, notification, misc syscalls
mkstub libkernel_web.sprx libkernel_web.so \
  sceKernelSendNotificationRequest sceKernelGetAppInfo sceKernelUsleep \
  pthread_create pthread_join pthread_detach \
  pthread_mutex_init pthread_mutex_lock pthread_mutex_unlock pthread_mutex_destroy \
  open close dup2 read write lseek fstat stat mkdir unlink chmod fchmod \
  fcntl ioctl execve \
  sleep usleep getpid geteuid getenv getcwd \
  getifaddrs freeifaddrs \
  gettimeofday clock_gettime

# libc: stdio + string + memory
mkstub libSceLibcInternal.sprx libSceLibcInternal.so \
  printf fprintf snprintf sprintf vfprintf vsnprintf \
  puts fputs fputc fgets fflush fopen fclose fread fwrite \
  fseek fseeko ftell ftello feof ferror fileno \
  memset memcpy memmove memchr memcmp \
  strlen strncpy strcpy strcat strncat strcmp strncmp \
  strstr strchr strrchr strerror perror \
  strtol strtoll strtod atoi atol \
  malloc calloc realloc free abort exit \
  asprintf basename dirname \
  isalnum isalpha isspace islower isupper isxdigit \
  __error __isthreaded in6addr_any

# network: BSD socket functions (game-compressor pattern)
mkstub libSceNet.sprx libSceNet.so \
  socket bind listen accept connect \
  send recv sendto recvfrom setsockopt getsockopt getsockname \
  __inet_addr __inet_aton __inet_ntop __inet_pton \
  htons htonl ntohs ntohl

# PS5 user/system services
mkstub libSceUserService.sprx libSceUserService.so \
  sceUserServiceInitialize sceUserServiceGetLoginUserIdList \
  sceUserServiceGetUserName sceUserServiceTerminate

mkstub libSceSystemService.sprx libSceSystemService.so \
  sceSystemServiceInitialize sceSystemServiceLaunchApp \
  sceSystemServiceLaunchWebBrowser sceSystemServiceParamGetString

# app install
mkstub libSceAppInstUtil.sprx libSceAppInstUtil.so \
  sceAppInstUtilInitialize sceAppInstUtilAppInstallPkg \
  sceAppInstUtilAppRecover sceAppInstUtilAppUnInstall \
  sceAppInstUtilGetAppInfo

# =============================================================================
# [1/3] Compile
# Single-line CFLAGS — avoids dash backslash-newline issues.
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
# Library order matches working reference ELFs (elf-arsenal, game-compressor).
# No libSceLncUtil — none of the reference payloads use it.
# =============================================================================
echo "==> [2/3] Linking..."
$LDLLD -m elf_x86_64 \
  --sysroot="$SDK" \
  -pie \
  -L"$SDK_STUBS" \
  "$SDK_CRT/crt1.o" \
  $OBJS \
  "$SDK_LIBC/libc.a" \
  -lkernel_web \
  -lSceLibcInternal \
  -lSceNet \
  -lSceUserService \
  -lSceSystemService \
  -lSceAppInstUtil \
  --allow-shlib-undefined \
  -o "$BUILD/quaza_payload.elf"

echo "==> [3/3] Done!"
echo "  ELF: $BUILD/quaza_payload.elf"
ls -lh "$BUILD/quaza_payload.elf"
