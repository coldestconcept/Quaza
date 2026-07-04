cmake_minimum_required(VERSION 3.15)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# Mandatory flags for PS5 freestanding payloads
add_compile_options(-ffreestanding -fno-builtin -nostdlib -fPIC)

# Ensure headers are found
include_directories($ENV{PS5_PAYLOAD_SDK}/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/common)

# List your source files
add_executable(quaza_payload.elf 
    main.c 
    pkg_server.c 
    param_sfo.c 
    browser_launch.c 
    jailbreak_detect.c 
    self_install.c
)

# Link settings: Use the SDK's linker script and kernel library
target_link_options(quaza_payload.elf PRIVATE 
    -T $ENV{PS5_PAYLOAD_SDK}/linker.x 
    -fuse-ld=lld 
    -nostdlib 
    -static
)

target_link_libraries(quaza_payload.elf kernel)
