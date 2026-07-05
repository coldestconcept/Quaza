cmake_minimum_required(VERSION 3.2)
project(quaza LANGUAGES C)

# ── Source files ────────────────────────────────────────────────────────────
# CRT must come first so _start is laid out before everything else.
set(CRT_SOURCES
    crt/crt.c
    crt/klog.c
    crt/kernel.c
    crt/mdbg.c
    crt/patch.c
    crt/rtld.c
    crt/env.c
)

set(LIBPKG_SOURCES
    libprosperopkg/src/npdrm.c
    libprosperopkg/src/pfs_builder.c
    libprosperopkg/src/pfs_directory.c
    libprosperopkg/src/pfs_inode.c
    libprosperopkg/src/pkg_builder.c
    libprosperopkg/src/utils.c
)

set(PAYLOAD_SOURCES
    main.c
    pkg_server.c
    browser_launch.c
    self_install.c
    param_sfo.c
    jailbreak_detect.c
)

add_executable(quaza_payload.elf
    ${CRT_SOURCES}
    ${LIBPKG_SOURCES}
    ${PAYLOAD_SOURCES}
)

# ── Include paths ────────────────────────────────────────────────────────────
target_include_directories(quaza_payload.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}            # www_embed.h, tile_pkg_embed.h, sfo_embed.h
    ${CMAKE_CURRENT_SOURCE_DIR}/crt        # payload.h, klog.h, kernel.h, etc.
    ${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include
)

# ── Compile flags ────────────────────────────────────────────────────────────
target_compile_options(quaza_payload.elf PRIVATE
    -Wall
    -Wno-unused-parameter
    -Wno-unused-function
    # Disable the PS5 .text syscall restriction: all syscalls go through the
    # ptr_syscall trampoline in crt.c, so we don't need an executable stack.
    -fno-stack-protector
)

# ── Linker flags ─────────────────────────────────────────────────────────────
# -T: override the SDK default linker script with our own so we control segment
#     layout and make sure .got/.got.plt are in the writable ph_data segment.
# -nostartfiles: we provide our own _start in crt/crt.c; don't link the SDK's
#     crt1.o or we'll get duplicate-symbol errors.
# --export-dynamic: keep all symbols in the ELF so rtld_load() can match them.
target_link_options(quaza_payload.elf PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/payload.ld
    -nostartfiles
    --export-dynamic
)

# ── Libraries (create DT_NEEDED entries) ─────────────────────────────────────
#
# These are the .sprx libraries whose symbols we use via extern declarations.
# The ps5-payload-sdk provides stub .a archives for each one under
# ${PS5_PAYLOAD_SDK}/target/lib/.  The linker reads them, creates DT_NEEDED
# entries in the ELF's .dynamic section, and rtld.c loads the real .sprx at
# runtime via sceKernelLoadStartModule.
#
#   libkernel          → sceKernelDlsym, sceKernelSendNotificationRequest, etc.
#   SceLibcInternal    → printf, malloc, free, pthread_*, etc.
#   SceNet             → socket, bind, listen, accept, getifaddrs, inet_ntop, etc.
#   SceUserService     → sceUserServiceInitialize
#   SceSystemService   → sceSystemServiceLaunchApp
#   SceAppInstUtil     → sceAppInstUtilInitialize, sceAppInstUtilAppInstallPkg,
#                        sceAppInstUtilAppRecover
#
target_link_libraries(quaza_payload.elf PRIVATE
    kernel
    SceLibcInternal
    SceNet
    SceUserService
    SceSystemService
    SceAppInstUtil
)
