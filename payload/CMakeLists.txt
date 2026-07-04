cmake_minimum_required(VERSION 3.15)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# Mandatory PS5 freestanding flags
add_compile_options(-ffreestanding -fno-builtin -nostdlib -fPIC -Wall)

# Header Search Paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/include/freebsd)

# Source Files
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

# Linker setup: Use SDK linker script and libraries
# Removed -fuse-ld=lld as it is rejected by the x86_64-sie-ps5 compiler target
target_link_options(quaza_payload.elf PRIVATE 
    -T $ENV{PS5_PAYLOAD_SDK}/linker.x 
    -nostdlib 
    -static
)

target_link_libraries(quaza_payload.elf kernel libc)
