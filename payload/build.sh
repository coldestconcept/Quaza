#!/bin/bash

# 1. Clear old build configurations
rm -rf build && mkdir build && cd build

# 2. Hardcode the absolute paths to map directly to the toolchain script parameters
export PS5_PAYLOAD_SDK="$HOME/target-sdk/ps5-payload-sdk"
REAL_TOOLCHAIN="${PS5_PAYLOAD_SDK}/toolchain/prospero.cmake"
SDK_TARGET_DIR="${PS5_PAYLOAD_SDK}/target"
STDIO_DIR="${PS5_PAYLOAD_SDK}/target/include_common"

# 3. Configure the pure cross-compilation pipeline utilizing the modern SDK system variables
cmake -DCMAKE_TOOLCHAIN_FILE="$REAL_TOOLCHAIN" \
      -DPS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK" \
      -DCMAKE_C_COMPILER_WORKS=1 \
      -DCMAKE_C_FLAGS="-I${SDK_TARGET_DIR}/include -I$STDIO_DIR" ..

# 4. Fire up the compiler engine execution
cmake --build .
