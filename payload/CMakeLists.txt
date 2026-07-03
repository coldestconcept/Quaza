cmake_minimum_required(VERSION 3.10)
project(QuazaPayload C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── PS5 Payload SDK ──────────────────────────────────────────────────────
if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
    message(FATAL_ERROR
        "PS5_PAYLOAD_SDK not set.\n"
        "Run: export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk\n"
        "See docs/BUILD.md for full instructions.")
endif()
set(PS5_SDK $ENV{PS5_PAYLOAD_SDK})

# Embed www/ assets into the ELF before compiling (offline mode)
# Run manually if www/ changes: python3 payload/tools/gen_embed.py
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/www_embed.h")
    message(STATUS "Generating www_embed.h from www/ assets...")
    execute_process(
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_embed.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE embed_rc)
    if(NOT embed_rc EQUAL 0)
        message(FATAL_ERROR "gen_embed.py failed (exit ${embed_rc})")
    endif()
endif()

# ── Include paths ────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include
    ${PS5_SDK}/include
    ${PS5_SDK}/common
)

link_directories(${PS5_SDK}/lib)

# ── Source files ─────────────────────────────────────────────────────────
set(SOURCE_FILES
    main.c
    pkg_server.c
    param_sfo.c
    browser_launch.c
    libprosperopkg/src/pfs_builder.c
    libprosperopkg/src/pfs_directory.c
    libprosperopkg/src/pfs_inode.c
    libprosperopkg/src/pkg_builder.c
    libprosperopkg/src/npdrm.c
    libprosperopkg/src/utils.c
)

# ── Build as PS5 ELF payload ─────────────────────────────────────────────
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
    SceLncUtil           # for sceLncUtilLaunchWebBrowser2
    pthread
)

# ── Post-build: package as fPKG ──────────────────────────────────────────
add_custom_command(TARGET quaza_payload.elf POST_BUILD
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_param_sfo.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/param_sfo_config.json"
            "${CMAKE_BINARY_DIR}/pkg_staging/sce_sys/param.sfo"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/pkg_staging/sce_sys"
    COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:quaza_payload.elf>"
            "${CMAKE_BINARY_DIR}/pkg_staging/eboot.bin"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/icon0.png"
            "${CMAKE_BINARY_DIR}/pkg_staging/sce_sys/icon0.png"
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_pkg.sh"
            "${CMAKE_BINARY_DIR}/pkg_staging"
            "${CMAKE_BINARY_DIR}/Quaza.pkg"
    COMMENT "Packaging Quaza.pkg..."
)
