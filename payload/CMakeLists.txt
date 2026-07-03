cmake_minimum_required(VERSION 3.10)
project(QuazaPayload C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
    message(FATAL_ERROR "Environmental baseline payload SDK missing. Please execute: export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk")
endif()

set(PS5_SDK $ENV{PS5_PAYLOAD_SDK})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libprosperopkg/include
    ${PS5_SDK}/include
    ${PS5_SDK}/common
)

link_directories(
    ${PS5_SDK}/lib
)

set(SOURCE_FILES
    main.c
    pkg_server.c
   
