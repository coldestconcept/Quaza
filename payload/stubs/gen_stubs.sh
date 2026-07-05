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

    local asm_file="${name}.s"
    {
        printf '.text\n'
        printf '.macro stub n\n'
        printf '.global \\n\n'
        printf '.type   \\n, @function\n'
        printf '\\n: xor %%eax,%%eax; ret\n'
        printf '.endm\n\n'
        for sym in "${syms[@]}"; do
            printf 'stub %s\n' "$sym"
        done
        # errno is a global int variable, not a function
        if [[ " ${syms[*]} " == *" errno "* ]]; then
            : # handled below
        fi
    } > "$asm_file"

    # errno needs to be a .data symbol, not a function stub
    # (replace the function stub line with a proper .data entry)
    if [[ " ${syms[*]} " == *" errno "* ]]; then
        sed -i 's/^stub errno$//' "$asm_file"
        printf '\n.data\n.global errno\nerrno: .long 0\n' >> "$asm_file"
    fi

    $STUB_CC -Wl,-soname,"$soname" -o "${name}.so" "$asm_file"
    rm -f "$asm_file"
    echo "  ✓  ${name}.so  (SONAME=${soname})"
}

echo "Building PS5 stub shared libs..."

# ── libSceLibcInternal.sprx — standard C library ──────────────────────────────
make_stub libSceLibcInternal libSceLibcInternal.sprx \
    printf fprintf sprintf snprintf vprintf vfprintf vsnprintf \
    fflush perror puts fopen fclose fread fwrite ftell fseek rewind \
    fgets fputs fputc fgetc feof ferror clearerr sscanf \
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
    errno

# ── libSceNet.sprx — networking ───────────────────────────────────────────────
make_stub libSceNet libSceNet.sprx \
    socket bind listen accept connect \
    send recv sendto recvfrom \
    setsockopt getsockopt shutdown select poll \
    getifaddrs freeifaddrs \
    inet_ntop inet_pton inet_addr \
    htons htonl ntohs ntohl \
    getaddrinfo freeaddrinfo gethostbyname

# ── libkernel.sprx — PS5 kernel (only symbols not handled by the CRT) ─────────
make_stub libkernel libkernel.sprx \
    sceKernelSendNotificationRequest \
    sceKernelDlsym \
    sceKernelLoadStartModule \
    sceKernelStopUnloadModule \
    getpid

# ── libSceUserService.sprx ────────────────────────────────────────────────────
make_stub libSceUserService libSceUserService.sprx \
    sceUserServiceInitialize \
    sceUserServiceTerminate

# ── libSceSystemService.sprx ──────────────────────────────────────────────────
make_stub libSceSystemService libSceSystemService.sprx \
    sceSystemServiceLaunchApp \
    sceSystemServiceGetStatus

# ── libSceAppInstUtil.sprx ────────────────────────────────────────────────────
make_stub libSceAppInstUtil libSceAppInstUtil.sprx \
    sceAppInstUtilInitialize \
    sceAppInstUtilAppInstallPkg \
    sceAppInstUtilAppRecover \
    sceAppInstUtilTerminate

touch built
echo ""
echo "All stubs built in payload/stubs/. Run 'make' in payload/ to build the ELF."
