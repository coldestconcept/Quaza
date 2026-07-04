cmake_minimum_required(VERSION 3.15)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# Ensure CMake compiles test targets as static libraries to bypass host checks
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Enforce a freestanding, position-independent environment
add_compile_options("-ffreestanding" "-fno-builtin" "-nostdlib" "-fPIC" "-Wall")

# Read target environment variable
set(PS5_SDK "$ENV{PS5_PAYLOAD_SDK}")
if(NOT PS5_SDK)
    message(FATAL_ERROR "PS5_PAYLOAD_SDK environment variable is not set!")
endif()

# Header Include Mapping
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include")
include_directories("${PS5_SDK}/include")
include_directories("${PS5_SDK}/include/freebsd")

# Explicit Source Definitions
set(SOURCES
    "main.c"
    "pkg_server.c"
    "param_sfo.c"
    "browser_launch.c"
    "jailbreak_detect.c"
    "self_install.c"
    "libprosperopkg/src/pfs_builder.c"
    "libprosperopkg/src/pfs_directory.c"
    "libprosperopkg/src/pfs_inode.c"
    "libprosperopkg/src/pkg_builder.c"
    "libprosperopkg/src/npdrm.c"
    "libprosperopkg/src/utils.c"
)

add_executable(quaza_payload.elf ${SOURCES})

# Library Search Directories
link_directories("${PS5_SDK}/lib")

# Link options to handle PS5 layout constraints
target_link_options(quaza_payload.elf PRIVATE 
    "-T" "${PS5_SDK}/linker.x"
    "-fuse-ld=lld"
    "-nostdlib"
    "-pie"
)

# Explicitly link crt1.o object first followed by static/stub libraries
target_link_libraries(quaza_payload.elf PRIVATE
    "${PS5_SDK}/lib/crt1.o"
    "kernel"
    "libc"
)
