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
TMP="${TMPDIR:-$BUILD}"

mkdir -p "$BUILD"

if [ ! -f "$PAYLOAD/tile_pkg_embed.h" ]; then
  echo "[error] tile_pkg_embed.h not found" >&2; exit 1
fi
echo "[info] tile_pkg_embed.h: $(wc -c < "$PAYLOAD/tile_pkg_embed.h") bytes"

# =============================================================================
# fix_stub <soname.sprx> <out_libname.so> [sym ...]
#
# Rebuilds a stub shared library with the correct SONAME and the listed
# exported symbols.  Two-step (clang -c then ld.lld -shared) avoids Termux's
# broken clang-as-linker invocation.
#
# Why we rebuild ALL six stubs here instead of relying on SDK originals:
#   The ps5-payload-dev/sdk git main now ships empty stubs (no exported
#   symbols).  The linker needs each symbol defined in *some* stub at link
#   time; at runtime the real .sprx on PS5 provides the implementation.
#   Rebuilding them ourselves is the only way to get a clean link.
# =============================================================================
fix_stub() {
  soname="$1"; out="$SDK_STUBS/$2"; shift 2
  src="$TMP/_stub_$$.c"; obj="$TMP/_stub_$$.o"
  printf '' > "$src"
  for s; do printf 'void %s(void){}\n' "$s" >> "$src"; done
  $CLANG --target=x86_64-sie-ps5 --sysroot="$SDK" \
    -fPIC -nostdlib -ffreestanding -c "$src" -o "$obj"
  $LDLLD -m elf_x86_64 -shared -nostdlib \
    "--soname=$soname" -o "$out" "$obj"
  rm -f "$src" "$obj"
  echo "  stub $soname"
}

echo "==> [0/3] Rebuilding stubs..."

# libkernel.sprx  — POSIX I/O, sockets, pthreads, sceKernel*
fix_stub libkernel.sprx libkernel.so \
  open close read write lseek dup dup2 \
  stat fstat lstat mkdir rmdir unlink rename chmod fchmod access \
  getpid geteuid getuid gettimeofday clock_gettime \
  getifaddrs freeifaddrs \
  socket connect bind listen accept send recv sendto recvfrom \
  setsockopt getsockopt getsockname getpeername \
  pthread_create pthread_join pthread_detach pthread_exit \
  pthread_mutex_init pthread_mutex_lock pthread_mutex_unlock pthread_mutex_destroy \
  pthread_cond_init pthread_cond_wait pthread_cond_signal \
  pthread_cond_broadcast pthread_cond_destroy \
  pthread_self pthread_attr_init pthread_attr_destroy pthread_attr_setstacksize \
  usleep sleep \
  sceKernelSendNotificationRequest sceKernelGetAppInfo sceKernelUsleep

# libSceLibcInternal.sprx  — libc (stdio, string, memory, ctype, stdlib)
fix_stub libSceLibcInternal.sprx libSceLibcInternal.so \
  printf fprintf snprintf sprintf vfprintf vprintf vsnprintf \
  puts fputs fputc fgetc fgets fflush \
  fopen fclose fread fwrite fseek fseeko ftell ftello \
  feof ferror fileno rewind fscanf sscanf \
  strlen strncpy strcpy strcat strncat \
  strcmp strncmp strstr strchr strrchr strdup strndup \
  memset memcpy memmove memchr memcmp bzero \
  malloc calloc realloc free \
  abort exit _exit \
  strtol strtoll strtoul strtoull strtod atoi atol atof \
  isalpha isalnum isspace isdigit isupper islower isxdigit isprint iscntrl \
  toupper tolower \
  strerror perror getcwd getenv asprintf \
  __error __isthreaded \
  __stderrp __stdinp __stdoutp \
  __inet_ntop __inet_pton __inet_addr __inet_aton \
  in6addr_any

# libSceUserService.sprx
fix_stub libSceUserService.sprx libSceUserService.so \
  sceUserServiceInitialize sceUserServiceFinalize sceUserServiceGetForegroundUser

# libSceSystemService.sprx  (was .so — SONAME was wrong)
fix_stub libSceSystemService.sprx libSceSystemService.so \
  sceSystemServiceInitialize sceSystemServiceLaunchApp \
  sceSystemServiceLaunchWebBrowser sceSystemServiceParamGetString

# libSceAppInstUtil.sprx  (was .so — SONAME was wrong)
fix_stub libSceAppInstUtil.sprx libSceAppInstUtil.so \
  sceAppInstUtilInitialize sceAppInstUtilAppInstallPkg \
  sceAppInstUtilAppRecover sceAppInstUtilAppUnInstall sceAppInstUtilGetAppInfo

# libSceSysmodule.sprx  — keep SDK original if it exists, else make a stub
if ! readelf -d "$SDK_STUBS/libSceSysmodule.so" 2>/dev/null | grep -q 'libSceSysmodule.sprx'; then
  fix_stub libSceSysmodule.sprx libSceSysmodule.so \
    sceSysmoduleLoadModule sceSysmoduleUnloadModule sceSysmoduleIsLoaded
fi

# =============================================================================
# [1/3] Compile
# =============================================================================
CFLAGS="--target=x86_64-sie-ps5 --sysroot=$SDK \
  -isystem $SDK/include -isystem $SDK/include/freebsd \
  -I$PAYLOAD -I$PAYLOAD/libprosperopkg/include \
  -O2 -std=gnu11 -ffreestanding -fno-builtin -nostdlib \
  -fPIC -fno-plt -fno-stack-protector \
  -Wall -Wno-incompatible-pointer-types"

echo "==> [1/3] Compiling..."
OBJS=""
for src in "$PAYLOAD"/*.c "$PAYLOAD"/libprosperopkg/src/*.c; do
  [ -f "$src" ] || continue
  base=$(basename "$src" .c)
  obj="$BUILD/${base}.o"
  echo "  cc $base.c"
  $CLANG $CFLAGS -c "$src" -o "$obj"
  OBJS="$OBJS $obj"
done

# =============================================================================
# [2/3] Link
# =============================================================================
echo "==> [2/3] Linking..."
# libc.a is NOT listed here: it is an import-stub archive whose printf.o etc.
# fight with our comprehensive dynamic stubs and cause "undefined symbol" errors.
# All libc symbols resolve at runtime from libSceLibcInternal.sprx (DT_NEEDED).
$LDLLD -m elf_x86_64 \
  --sysroot="$SDK" \
  -pie \
  -L"$SDK_STUBS" \
  "$SDK_CRT/crt1.o" \
  $OBJS \
  -lkernel \
  -lSceLibcInternal \
  -lSceAppInstUtil \
  -lSceSystemService \
  -lSceUserService \
  -lSceSysmodule \
  --allow-shlib-undefined \
  -o "$BUILD/quaza_payload.elf"

echo "==> [3/3] Done!"
echo "  ELF: $BUILD/quaza_payload.elf"
ls -lh "$BUILD/quaza_payload.elf"
