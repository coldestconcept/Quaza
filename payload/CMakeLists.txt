cmake_minimum_required(VERSION 3.10)
project(QuazaPayload C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── PS5 Payload SDK ──────────────────────────────────────────────────────
if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
    message(FATAL_ERROR
        "PS5_PAYLOAD_SDK not set.\n"
        "Run: export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk\n"
        "See docs/BUILD.md for full setup instructions.")
endif()
set(PS5_SDK $ENV{PS5_PAYLOAD_SDK})

# ── Embed www/ assets into ELF (offline mode) ────────────────────────────
# Regenerate whenever www/ files change:  python3 payload/tools/gen_embed.py
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/www_embed.h")
    message(STATUS "Generating www_embed.h...")
    execute_process(
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_embed.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE embed_rc)
    if(NOT embed_rc EQUAL 0)
        message(FATAL_ERROR "gen_embed.py failed (exit ${embed_rc})")
    endif()
endif()

# ── Embed param.sfo binary ───────────────────────────────────────────────
# Regenerate when param_sfo_config.json changes:
#   python3 payload/tools/gen_param_sfo.py \
#       payload/sce_sys/param_sfo_config.json /tmp/param.sfo
#   python3 - (embed script embedded in CMakeLists below)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sfo_embed.h")
    message(STATUS "Generating sfo_embed.h...")
    execute_process(
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_param_sfo.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/param_sfo_config.json"
                "${CMAKE_CURRENT_BINARY_DIR}/param.sfo"
        RESULT_VARIABLE sfo_rc)
    if(NOT sfo_rc EQUAL 0)
        message(FATAL_ERROR "gen_param_sfo.py failed")
    endif()
    execute_process(
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_sfo_embed.py"
                "${CMAKE_CURRENT_BINARY_DIR}/param.sfo"
                "${CMAKE_CURRENT_SOURCE_DIR}/sfo_embed.h"
        RESULT_VARIABLE embed_sfo_rc)
    if(NOT embed_sfo_rc EQUAL 0)
        message(FATAL_ERROR "gen_sfo_embed.py failed")
    endif()
endif()

# ── Include paths ────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include
    ${PS5_SDK}/include
    ${PS5_SDK}/common
    # kstuff-lite headers (from the PS5 Payload SDK — NOT bundled with Quaza;
    # kstuff must already be running in the system before the ELF is injected)
    ${PS5_SDK}/include/kstuff
)

    ${PS5_SDK}/lib/ps5-payload-sdk
    ${PS5_SDK}/ps5-payload-sdk/lib
    ${PS5_SDK}/lib/ps5-payload-sdk
    ${PS5_SDK}/ps5-payload-sdk/lib
    ${PS5_SDK}/lib/ps5-payload-sdk
    ${PS5_SDK}/ps5-payload-sdk/lib
link_directories(
    ${PS5_SDK}/lib
    # No ${PS5_SDK}/lib/kstuff — we do NOT link kstuff statically.
    # At runtime we call through the already-running kstuff bridge socket.
    # The kstuff_lite.h declarations are resolved via the SDK import library
    # (libkstuff_stub.a), which connects to /dev/kstuff or /tmp/kstuff.sock.
    ${PS5_SDK}/lib/kstuff_stub
)

# ── Source files ─────────────────────────────────────────────────────────
set(SOURCE_FILES
    main.c
    pkg_server.c
    param_sfo.c
    browser_launch.c
    jailbreak_detect.c
    self_install.c
    libprosperopkg/src/pfs_builder.c
    libprosperopkg/src/pfs_directory.c
    libprosperopkg/src/pfs_inode.c
    libprosperopkg/src/pkg_builder.c
    libprosperopkg/src/npdrm.c
    libprosperopkg/src/utils.c
)

# ── Build ELF ────────────────────────────────────────────────────────────
add_executable(quaza_payload.elf ${SOURCE_FILES})

target_compile_options(quaza_payload.elf PRIVATE
    -target x86_64-pc-freebsd12
    -fPIC
    -Wall
    -Wextra
    -O2
)

target_link_options(quaza_payload.elf PRIVATE
    -target x86_64-pc-freebsd12
)

target_link_libraries(quaza_payload.elf
    kernel
    SceLibcInternal
    SceLncUtil           # sceLncUtilLaunchWebBrowser2
    SceAppInstUtil       # sceAppInstUtil* — app installation APIs
    SceUserService       # sceUserServiceInitialize
    kstuff_stub          # import stub — resolves at runtime against the
                         # kstuff bridge already running in the system.
                         # Quaza does NOT ship or install kstuff-lite;
                         # it requires the user to have it active first
                         # (standard on FW 12.70 via ps5-payload-elfldr).
    pthread
)

# ── Post-build: copy ELF and package as fPKG ─────────────────────────────
add_custom_command(TARGET quaza_payload.elf POST_BUILD
    # Staging directory
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/pkg_staging/sce_sys"
    # eboot.bin
    COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:quaza_payload.elf>"
            "${CMAKE_BINARY_DIR}/pkg_staging/eboot.bin"
    # param.sfo (freshly generated)
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_param_sfo.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/param_sfo_config.json"
            "${CMAKE_BINARY_DIR}/pkg_staging/sce_sys/param.sfo"
    # icon0.png (user-provided — see payload/sce_sys/README.md)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/icon0.png"
            "${CMAKE_BINARY_DIR}/pkg_staging/sce_sys/icon0.png"
    # Pack into Quaza.pkg
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_pkg.sh"
            "${CMAKE_BINARY_DIR}/pkg_staging"
            "${CMAKE_BINARY_DIR}/Quaza.pkg"
    COMMENT "Packaging Quaza.pkg..."
    VERBATIM
)
