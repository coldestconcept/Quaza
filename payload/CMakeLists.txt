cmake_minimum_required(VERSION 3.15)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# These flags are vital to prevent the "crt1.o" leak
add_compile_options(-ffreestanding -fno-builtin -nostdlib -fPIC)

# The SDK libraries and headers
include_directories($ENV{PS5_PAYLOAD_SDK}/include)
include_directories($ENV{PS5_PAYLOAD_SDK}/common)
link_directories($ENV{PS5_PAYLOAD_SDK}/lib)

add_executable(quaza_payload.elf main.c pkg_server.c param_sfo.c browser_launch.c jailbreak_detect.c self_install.c)

# Link against the kernel stub and specify the linker script
target_link_options(quaza_payload.elf PRIVATE -T ${PS5_PAYLOAD_SDK}/linker.x -fuse-ld=lld -nostdlib -static)
target_link_libraries(quaza_payload.elf kernel)
