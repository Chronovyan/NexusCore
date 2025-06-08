#!/bin/bash

# Build the project in release mode
# Usage: ./scripts/build-release.sh [build-dir] [extra-cmake-args...]

set -e

# Default values
BUILD_DIR=${1:-build-release}
shift

# Create build directory
mkdir -p "${BUILD_DIR}"

# Configure the project in release mode
echo "Configuring release build in ${BUILD_DIR}..."
cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DCMAKE_CXX_FLAGS="-march=native -O3 -DNDEBUG" \
    "$@" \
    .

# Build the project
echo "Building the project in release mode..."
cmake --build "${BUILD_DIR}" --config Release -- -j$(nproc)

echo "\n✅ Release build completed successfully!"
echo "The executable is available at: ${BUILD_DIR}/bin/editor"
