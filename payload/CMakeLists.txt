cmake_minimum_required(VERSION 3.10)
project(QuazaPayload C)

# Ensure C compiler standards match target operating system libraries
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Verify environmental variables exist for the PS5 Payload SDK location
if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
    message(FATAL_ERROR "Environmental baseline payload SDK missing. Please execute: export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk")
endif()

set(PS5_SDK $ENV{PS5_PAYLOAD_SDK})

# Define targeted search include headers matching hardware architectures
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PS5_SDK}/include
    ${PS5_SDK}/common
)

# Identify build output library binary pathways 
link_directories(
    ${PS5_SDK}/lib
)

# Aggregate modular background core source files
set(SOURCE_FILES
    main.c
    server.c
    router.c
)

# Configure output naming rules matching build instruction layouts
set(OUTPUT_ELF_NAME "quaza_payload.elf")

add_executable(${OUTPUT_ELF_NAME} ${SOURCE_FILES})

# Target toolchain configuration variables 
# Appends platform flags needed for custom homebrew system injection runtimes
target_compile_options(${OUTPUT_ELF_NAME} PRIVATE
    -O2 
    -fno-builtin 
    -fPIC 
    -Wall
)

# Apply link-time system arguments ensuring compatibility with loader formats
target_link_options(${OUTPUT_ELF_NAME} PRIVATE
    -static 
    -pie 
    -nostdlib
)

# Bind system networking capabilities and target kernel access libraries
target_link_libraries(${OUTPUT_ELF_NAME} PRIVATE
    prospero_kernel
    prospero_net
    prospero_user
)

# -------------------------------------------------------------------
# Deployment Target Configurations
# -------------------------------------------------------------------
if(DEFINED ENV{PS5_HOST})
    set(PS5_IP $ENV{PS5_HOST})
else()
    set(PS5_IP "127.0.0.1") # Fallback loopback address
endif()

if(DEFINED ENV{PS5_PORT})
    set(PS5_NET_PORT $ENV{PS5_PORT})
else()
    set(PS5_NET_PORT "9021") # Fallback deployment execution port configuration
endif()

# Build automation rule allowing direct injection with 'make deploy'
add_custom_target(deploy
    COMMAND echo "Injecting compiled software package binary into host location ${PS5_IP}:${PS5_NET_PORT}..."
    COMMAND nc -w 3 ${PS5_IP} ${PS5_NET_PORT} < ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_ELF_NAME}
    DEPENDS ${OUTPUT_ELF_NAME}
    COMMENT "Deploying Quaza executable packet to physical target environment."
)
