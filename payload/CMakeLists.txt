cmake_minimum_required(VERSION 3.15)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# Essential flags for a PS5 standalone binary
add_compile_options(-ffreestanding -fno-builtin -nostdlib -fPIC -Wall)

# Include the payload's local headers and the SDK headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/include)

# Define the source files
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

# Linker settings: Use the SDK's linker script and libraries
target_link_options(quaza_payload.elf PRIVATE 
    -T $ENV{PS5_PAYLOAD_SDK}/linker.x 
    -fuse-ld=lld 
    -nostdlib 
    -static
)

# Link against the consolidated SDK libraries
target_link_libraries(quaza_payload.elf kernel libc)
