cmake_minimum_required(VERSION 3.15)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# Enforce freestanding compiler options
add_compile_options(-ffreestanding -fno-builtin -nostdlib -fPIC -Wall)

# Header Search Paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/include/freebsd)

# Explicitly list source files
# This ensures that all custom helper modules are compiled into the payload
set(SOURCES
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

add_executable(quaza_payload.elf ${SOURCES})

# Linker setup: Exclude standard host libraries
target_link_options(quaza_payload.elf PRIVATE 
    "-nostdlib"
)

# Link against the target stubs compiled in the SDK
# We link the specific library modules to resolve the undefined symbols:
# - SceUserService: sceUserServiceInitialize
# - SceAppInstUtil: sceAppInstUtilInitialize, sceAppInstUtilAppPrepareInstall, sceAppInstUtilAppInstall, sceAppInstUtilAppRecover
# - SceLncUtil: sceLncUtilLaunchWebBrowser2
target_link_libraries(quaza_payload.elf PRIVATE
    "$ENV{PS5_PAYLOAD_SDK}/lib/crt1.o"
    "$ENV{PS5_PAYLOAD_SDK}/lib/libc.a"
    kernel
    SceLibcInternal
    SceLncUtil
    SceUserService
    SceAppInstUtil
)
