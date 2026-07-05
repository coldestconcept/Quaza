#!/bin/sh
# build_direct.sh — Quaza PS5 payload direct build (no cmake)
#
# Root-cause fix (2025-07):
#   ps5-payload-elfldr does NOT do dynamic linking for injected ELFs.
#   It only handles R_X86_64_RELATIVE relocations, then jumps to e_entry.
#   The SDK's crt/ layer (_start → __rtld_init) uses payload_args->sceKernelDlsym
#   to resolve every PLT/GOT entry before calling main(). Without it, every
#   imported function call immediately segfaults.
#
#   Fix: compile sdk/crt/*.c into the payload, use ENTRY(_start) in the
#   linker script, and stub SONAMEs must be .so (not .sprx) so rtld_open()
#   can find them in the name→handle table.

set -e

PAYLOAD="$(cd "$(dirname "$0")" && pwd)"
SDK="$HOME/ps5-payload-sdk"
SDK_STUBS="$SDK/sce_stubs"
SDK_CRT="$SDK/crt"
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
# fix_stub <soname.so> <out_libname.so> [sym ...]
#
# Builds a link-time stub .so with the correct SONAME.
# SONAMEs MUST end in .so (not .sprx): rtld_open() matches against .so names
# in its special-case table (libkernel_sys.so → libkernel handle,
# libSceLibcInternal.so → handle 0x2) and in sysmodtab[].
# At runtime the real .sprx on PS5 provides the implementation.
#
# Stub bodies are empty (just a label) — identical to john-tornblom's SDK
# stubs. The linker generates PLT/GOT entries; rtld resolves them at runtime.
# =============================================================================
fix_stub() {
  soname="$1"; out="$SDK_STUBS/$2"; shift 2
  src="$TMP/_stub_$$.c"
  printf '' > "$src"
  for s; do
    printf 'asm(".global %s\\n.type %s @function\\n%s:\\n");\n' \
      "$s" "$s" "$s" >> "$src"
  done
  # Compile as neutral x86_64 (avoids Termux aarch64 native-target issue)
  $CLANG -target x86_64 -c -ffreestanding -fno-builtin -nostdlib \
    -fPIC -fno-plt -o "$TMP/_stub_$$.o" "$src"
  $LDLLD -m elf_x86_64 -shared -nostdlib \
    "--soname=$soname" -o "$out" "$TMP/_stub_$$.o"
  rm -f "$src" "$TMP/_stub_$$.o"
  echo "  stub $soname"
}

echo "==> [0/4] Rebuilding stubs..."
# SONAMEs are .so — rtld_open() special-cases libkernel*.so → libkernel handle
#                   and libSceLibcInternal.so → handle 0x2

# libkernel_sys.so — rtld maps to libkernel handle (0x1 or 0x2001)
fix_stub libkernel_sys.so libkernel_sys.so \
  open close read write lseek dup dup2 \
  stat fstat lstat mkdir rmdir unlink rename chmod fchmod access \
  getpid geteuid getuid gettimeofday clock_gettime \
  usleep sleep time signal \
  opendir readdir closedir \
  socket connect bind listen accept send recv sendto recvfrom \
  setsockopt getsockopt getsockname getpeername \
  getifaddrs freeifaddrs \
  pthread_create pthread_join pthread_detach pthread_exit \
  pthread_mutex_init pthread_mutex_lock pthread_mutex_unlock pthread_mutex_destroy \
  pthread_cond_init pthread_cond_wait pthread_cond_signal \
  pthread_cond_broadcast pthread_cond_destroy \
  pthread_self pthread_attr_init pthread_attr_destroy pthread_attr_setstacksize \
  sceKernelSendNotificationRequest sceKernelGetAppInfo sceKernelUsleep \
  sceKernelLoadStartModule sceKernelStopUnloadModule sceKernelDlsym \
  sceKernelSpawn

# libkernel_web.so — rtld also maps to libkernel handle
fix_stub libkernel_web.so libkernel_web.so \
  sceKernelSendNotificationRequest sceKernelGetAppInfo sceKernelUsleep \
  sceKernelLoadStartModule sceKernelStopUnloadModule sceKernelDlsym \
  open close read write lseek \
  getpid gettimeofday usleep sleep \
  pthread_create pthread_join pthread_detach pthread_exit \
  pthread_mutex_init pthread_mutex_lock pthread_mutex_unlock pthread_mutex_destroy \
  pthread_self pthread_attr_init pthread_attr_destroy pthread_attr_setstacksize

# libSceNet.so — rtld: sysmodtab ID 0x8000001c, loaded via sceKernelLoadStartModule
fix_stub libSceNet.so libSceNet.so \
  socket connect bind listen accept send recv sendto recvfrom \
  setsockopt getsockopt getsockname getpeername \
  getifaddrs freeifaddrs

# libSceLibcInternal.so — rtld maps to handle 0x2 (full libc)
fix_stub libSceLibcInternal.so libSceLibcInternal.so \
  printf fprintf snprintf sprintf vfprintf vprintf vsnprintf \
  puts fputs fputc fgetc fgets fflush \
  fopen fclose fread fwrite fseek fseeko ftell ftello \
  feof ferror fileno rewind fscanf sscanf \
  strlen strncpy strcpy strcat strncat \
  strcmp strncmp strstr strchr strrchr strdup strndup \
  memset memcpy memmove memchr memcmp bzero \
  malloc calloc realloc free \
  abort exit _exit \
  strtol strtoll strtoul strtoull strtod strtok atoi atol atof \
  isalpha isalnum isspace isdigit isupper islower isxdigit isprint iscntrl \
  toupper tolower \
  strerror perror getcwd getenv setenv asprintf \
  __error __isthreaded \
  __stderrp __stdinp __stdoutp \
  __inet_ntop __inet_pton

# libSceUserService.so — rtld: sysmodtab ID 0x80000011
fix_stub libSceUserService.so libSceUserService.so \
  sceUserServiceInitialize sceUserServiceFinalize sceUserServiceGetForegroundUser

# libSceSystemService.so — rtld: sysmodtab ID 0x80000010
fix_stub libSceSystemService.so libSceSystemService.so \
  sceSystemServiceInitialize sceSystemServiceLaunchApp \
  sceSystemServiceLaunchWebBrowser sceSystemServiceParamGetString

# libSceAppInstUtil.so — rtld: sysmodtab ID 0x80000014
fix_stub libSceAppInstUtil.so libSceAppInstUtil.so \
  sceAppInstUtilInitialize sceAppInstUtilAppInstallPkg \
  sceAppInstUtilAppRecover sceAppInstUtilAppUnInstall sceAppInstUtilGetAppInfo

# =============================================================================
# [1/4] Compile CRT layer (SDK startup: _start, rtld, klog, kernel, patch, env)
#
# The crt/ files provide:
#   _start()     — ELF entry called by elfldr; clears .bss, calls __rtld_init,
#                  then calls main()
#   __rtld_init  — walks _DYNAMIC, loads DT_NEEDED libs via sceKernelDlsym +
#                  sceKernelLoadStartModule, patches all R_X86_64_GLOB_DAT /
#                  R_X86_64_JMP_SLOT GOT entries
#   klog_*       — kernel log helpers (visible in /dev/klog, readable via klogsrv)
#   kernel_*     — kernel R/W helpers via rw_pair sockets (from payload_args)
#   patch_*      — patches kernel ucred caps, enables ptrace
#   env_*        — parses envp into setenv() calls
# =============================================================================
CRT_CFLAGS="--target=x86_64-sie-ps5 \
  -isystem $SDK/include -isystem $SDK/include/freebsd \
  -I$PAYLOAD/crt \
  -O0 -g -std=gnu11 -ffreestanding -fno-builtin -nostdlib \
  -fPIC -fno-plt -fno-stack-protector \
  -Wall -Wno-unused-function"

echo "==> [1/4] Compiling CRT layer..."
CRT_OBJS=""
for src in "$PAYLOAD/crt/crt.c" \
           "$PAYLOAD/crt/rtld.c" \
           "$PAYLOAD/crt/klog.c" \
           "$PAYLOAD/crt/kernel.c" \
           "$PAYLOAD/crt/mdbg.c" \
           "$PAYLOAD/crt/patch.c" \
           "$PAYLOAD/crt/env.c"; do
  [ -f "$src" ] || { echo "  [warn] missing $src — skip"; continue; }
  base=$(basename "$src" .c)
  obj="$BUILD/crt_${base}.o"
  echo "  cc crt/$base.c"
  $CLANG $CRT_CFLAGS -c "$src" -o "$obj"
  CRT_OBJS="$CRT_OBJS $obj"
done

# =============================================================================
# [2/4] Compile payload sources
# =============================================================================
CFLAGS="--target=x86_64-sie-ps5 \
  -isystem $SDK/include -isystem $SDK/include/freebsd \
  -I$PAYLOAD -I$PAYLOAD/crt -I$PAYLOAD/libprosperopkg/include \
  -O2 -std=gnu11 -ffreestanding -fno-builtin -nostdlib \
  -fPIC -fno-plt -fno-stack-protector \
  -Wall -Wno-incompatible-pointer-types"

echo "==> [2/4] Compiling payload sources..."
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
# [3/4] Link
#
# CRT objects go FIRST so _start is found before main.
# Linker script (payload.ld) is the SDK's elf_x86_64.x with ENTRY(_start).
# No -z max-page-size: SDK uses CONSTANT(MAXPAGESIZE) = 4096 (x86_64 FreeBSD).
# Stub library names must match the SONAME we set above (.so extension).
# =============================================================================
echo "==> [3/4] Linking..."
$LDLLD -m elf_x86_64 \
  --sysroot="$SDK" \
  -pie \
  -T "$PAYLOAD/payload.ld" \
  -L"$SDK_STUBS" \
  $CRT_OBJS \
  $OBJS \
  -lkernel_sys \
  -lkernel_web \
  -lSceNet \
  -lSceLibcInternal \
  -lSceUserService \
  -lSceSystemService \
  -lSceAppInstUtil \
  --allow-shlib-undefined \
  -o "$BUILD/quaza_payload.elf"

echo "==> [4/4] Done!"
echo "  ELF: $BUILD/quaza_payload.elf"
ls -lh "$BUILD/quaza_payload.elf"
echo ""
echo "  Program headers (expect: 4 PT_LOADs + PT_DYNAMIC):"
readelf -l "$BUILD/quaza_payload.elf" 2>/dev/null | grep -E 'LOAD|DYNAMIC|Type|Entry' || true
