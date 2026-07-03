cmake_minimum_required(VERSION 3.20)
project(quaza_payload C)

set(CMAKE_C_STANDARD 11)

# PS5 SDK paths
if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
    message(FATAL_ERROR "PS5_PAYLOAD_SDK environment variable not set")
endif()

set(PS5_SDK $ENV{PS5_PAYLOAD_SDK})

# Compilers
set(CMAKE_C_COMPILER ${PS5_SDK}/bin/prospero-clang)
set(CMAKE_CXX_COMPILER ${PS5_SDK}/bin/prospero-clang++)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/libprosperopkg/include
    ${PS5_SDK}/include
)

# Source files
set(SOURCES
    main.c
    pkg_server.c
    libprosperopkg/src/pkg_builder.c
    libprosperopkg/src/pfs_builder.c
    libprosperopkg/src/pfs_inode.c
    libprosperopkg/src/pfs_directory.c
    libprosperopkg/src/npdrm.c
    libprosperopkg/src/utils.c
)

# Create executable
add_executable(quaza_payload.elf ${SOURCES})

# Link libraries
target_link_libraries(quaza_payload.elf
    ${PS5_SDK}/lib/libSceLibcInternal.a
    pthread
)

# Compiler flags
target_compile_options(quaza_payload.elf PRIVATE
    -O2
    -Wall
    -D__PS5__
    -D_POSIX_THREADS
)

# Linker flags
target_link_options(quaza_payload.elf PRIVATE
    -T${PS5_SDK}/elf_x86_64.x
    -nostdlib
    -dynamic-linker
)

# Install target
install(TARGETS quaza_payload.elf
    DESTINATION ${CMAKE_SOURCE_DIR}/../www/
)
