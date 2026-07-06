#!/bin/bash
# gen_stubs.sh — build PS5 library stubs for linking without the full SDK.
#
# Creates x86_64 shared objects with SONAME set to the real PS5 .sprx name so
# the linker produces correct DT_NEEDED entries and GLOB_DAT relocations.
# The stubs are never shipped to the PS5; only the payload ELF is.
# At runtime, rtld.c loads the real .sprx and resolves every symbol.
#
# Run once from payload/:   bash stubs/gen_stubs.sh
# The main Makefile calls this automatically when stubs/built is missing.

set -e
cd "$(dirname "$0")"

CC="${CC:-clang}"
# Compile stubs for x86_64 — Termux ARM64 clang supports this cross-target.
STUB_CC="$CC -target x86_64-linux-gnu -shared -fPIC -nostdlib -Wl,--build-id=none"

# ── helper: emit an assembly file and compile it into a stub .so ──────────────
make_stub() {
    local name="$1"   # library basename, e.g. libSceLibcInternal
    local soname="$2" # real PS5 name,  e.g. libSceLibcInternal.sprx
    shift 2
    local syms=("$@")

    # Symbols that are data (variables), not functions — must be emitted as
    # .data entries with GLOB_DAT relocations, not function stubs, or the
    # linker/rtld will treat them as callable code.
    local data_syms="errno stdin stdout stderr"

    local asm_file="${name}.s"
    {
        printf '.text\n'
        printf '.macro stub n\n'
        printf '.global \\n\n'
        printf '.type   \\n, @function\n'
        printf '\\n: xor %%eax,%%eax; ret\n'
        printf '.endm\n\n'
        for sym in "${syms[@]}"; do
            if [[ " $data_syms " != *" $sym "* ]]; then
                printf 'stub %s\n' "$sym"
            fi
        done
    } > "$asm_file"

    # Data symbols (errno, stdin, stdout, stderr) need a proper .data entry
    # instead of a function stub — errno is a 4-byte int, the FILE* globals
    # are pointer-sized (8 bytes on x86_64).
    {
        printf '\n.data\n'
        for sym in "${syms[@]}"; do
            if [[ " $data_syms " == *" $sym "* ]]; then
                if [[ "$sym" == "errno" ]]; then
                    printf '.global %s\n%s: .long 0\n' "$sym" "$sym"
                else
                    printf '.global %s\n%s: .quad 0\n' "$sym" "$sym"
                fi
            fi
        done
    } >> "$asm_file"

    $STUB_CC -Wl,-soname,"$soname" -o "${name}.so" "$asm_file"
    rm -f "$asm_file"
    echo "  ✓  ${name}.so  (SONAME=${soname})"
}

echo "Building PS5 stub shared libs..."

# ── libSceLibcInternal.so — standard C library ────────────────────────────────
# SONAME must end in .so — rtld.c strips last 2 chars and appends "sprx".
# libSceLibcInternal.so is special-cased in rtld.c → handle 0x2 directly.
# errno is NOT a global on PS5/BSD; use __error() → *__error() indirection.
make_stub libSceLibcInternal libSceLibcInternal.so \
    printf fprintf sprintf snprintf vprintf vfprintf vsnprintf \
    fflush perror puts fopen fclose fread fwrite ftell fseek rewind \
    fgets fputs fputc fgetc feof ferror clearerr sscanf fdopen fileno \
    malloc calloc realloc free \
    atoi atol atof strtol strtoul strtoll strtoull strtod \
    exit abort qsort bsearch abs labs rand srand getenv setenv unsetenv \
    memset memcpy memmove memcmp memchr \
    strcmp strncmp strcasecmp strncasecmp \
    strcpy strncpy strcat strncat strlen strdup strndup \
    strstr strchr strrchr strtok strtok_r strerror strspn strcspn strpbrk \
    open close read write lseek dup dup2 pipe \
    access unlink rmdir rename symlink readlink getcwd chdir \
    sleep usleep getpid getuid geteuid \
    stat fstat lstat mkdir chmod fchmod umask \
    fcntl \
    opendir readdir closedir rewinddir \
    pthread_create pthread_join pthread_detach pthread_exit pthread_self \
    pthread_mutex_init pthread_mutex_lock pthread_mutex_unlock \
    pthread_mutex_trylock pthread_mutex_destroy \
    pthread_cond_init pthread_cond_wait pthread_cond_signal \
    pthread_cond_broadcast pthread_cond_destroy \
    pthread_attr_init pthread_attr_destroy pthread_attr_setstacksize \
    pthread_key_create pthread_key_delete pthread_getspecific pthread_setspecific \
    signal sigaction sigemptyset sigfillset sigaddset sigdelset sigprocmask \
    kill raise \
    time clock gettimeofday localtime gmtime mktime strftime difftime nanosleep \
    clock_gettime \
    isdigit isxdigit isalpha isalnum isupper islower isspace ispunct \
    isprint iscntrl toupper tolower \
    __error stdin stdout stderr

# ── libSceNet.so — networking ─────────────────────────────────────────────────
make_stub libSceNet libSceNet.so \
    socket bind listen accept connect \
    send recv sendto recvfrom \
    setsockopt getsockopt shutdown select poll \
    getifaddrs freeifaddrs \
    inet_ntop inet_pton inet_addr \
    htons htonl ntohs ntohl \
    getaddrinfo freeaddrinfo gethostbyname

# ── libkernel.so — PS5 kernel ─────────────────────────────────────────────────
# Special-cased in rtld.c: uses libkernel_handle directly, no .so→.sprx transform.
make_stub libkernel libkernel.so \
    sceKernelSendNotificationRequest \
    sceKernelDlsym \
    sceKernelLoadStartModule \
    sceKernelStopUnloadModule \
    getpid

# ── libSceUserService.so ──────────────────────────────────────────────────────
make_stub libSceUserService libSceUserService.so \
    sceUserServiceInitialize \
    sceUserServiceTerminate

# ── libSceSystemService.so ────────────────────────────────────────────────────
make_stub libSceSystemService libSceSystemService.so \
    sceSystemServiceLaunchApp \
    sceSystemServiceGetStatus

# ── libSceAppInstUtil.so ──────────────────────────────────────────────────────
make_stub libSceAppInstUtil libSceAppInstUtil.so \
    sceAppInstUtilInitialize \
    sceAppInstUtilAppInstallPkg \
    sceAppInstUtilAppRecover \
    sceAppInstUtilTerminate

touch built
echo ""
echo "All stubs built in payload/stubs/. Run 'make' in payload/ to build the ELF."
