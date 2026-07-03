cmake_minimum_required(VERSION 3.10)
project(QuazaPayload C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
    message(FATAL_ERROR "Environmental baseline payload SDK missing. Please execute: export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk")
endif()

set(PS5_SDK $ENV{PS5_PAYLOAD_SDK})

# Include directories - ADDED libprosperopkg/include
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include
    ${PS5_SDK}/include
    ${PS5_SDK}/common
)

link_directories(
    ${PS5_SDK}/lib
)

# FIXED: Include all libprosperopkg source files, removed server.c and router.c
set(SOURCE_FILES
    main.c
    pkg_server.c
    libprosperopkg/src/pkg_builder.c
    libprosperopkg/src/pfs_builder.c
    libprosperopkg/src/pfs_inode.c
    libprosperopkg/src/pfs_directory.c
    libprosperopkg/src/npdrm.c
    libprosperopkg/src/utils.c
)

set(OUTPUT_ELF_NAME "quaza_payload.elf")

add_executable(${OUTPUT_ELF_NAME} ${SOURCE_FILES})

target_compile_options(${OUTPUT_ELF_NAME} PRIVATE
    -O2
    -fno-builtin
    -fPIC
    -Wall
    -D__PS5__
)

target_link_options(${OUTPUT_ELF_NAME} PRIVATE
    -static
    -pie
    -nostdlib
)

# FIXED: Added pthread
target_link_libraries(${OUTPUT_ELF
