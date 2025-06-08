#!/bin/bash

# Build the project in debug mode with sanitizers
# Usage: ./scripts/build-debug.sh [build-dir] [extra-cmake-args...]

set -e

# Default values
BUILD_DIR=${1:-build-debug}
shift

# Create build directory
mkdir -p "${BUILD_DIR}"

# Configure the project in debug mode with sanitizers
echo "Configuring debug build in ${BUILD_DIR}..."
cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON \
    -DENABLE_DEBUG_SYMBOLS=ON \
    "$@" \
    .

# Build the project
echo "Building the project in debug mode..."
cmake --build "${BUILD_DIR}" --config Debug -- -j$(nproc)

echo "\n✅ Debug build completed successfully!"
echo "The debug executable is available at: ${BUILD_DIR}/bin/editor"
