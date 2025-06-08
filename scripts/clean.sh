#!/bin/bash

# Clean build directories and generated files
# Usage: ./scripts/clean.sh [build-dirs...]

set -e

# Default build directories to clean
DEFAULT_BUILD_DIRS=("build" "build-debug" "build-release" "coverage" "docs")

# Use provided directories or default ones
if [ $# -eq 0 ]; then
    BUILD_DIRS=("${DEFAULT_BUILD_DIRS[@]}")
else
    BUILD_DIRS=("$@")
fi

# Clean each build directory
for dir in "${BUILD_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "Removing $dir..."
        rm -rf "$dir"
    else
        echo "Skipping $dir (not found)"
    fi
done

# Clean any other generated files
echo "Cleaning generated files..."
find . -type d -name "CMakeFiles" -exec rm -rf {} + 2>/dev/null || true
find . -type f -name "CMakeCache.txt" -delete
find . -type f -name "cmake_install.cmake" -delete
find . -type f -name "Makefile" -delete
find . -type f -name "*.o" -delete
find . -type f -name "*.a" -delete
find . -type f -name "*.so" -delete
find . -type f -name "*.dll" -delete
find . -type f -name "*.exe" -delete
find . -type f -name "*.user" -delete
find . -type f -name "*.sln" -delete
find . -type f -name "*.vcxproj*" -delete
find . -type d -name ".vs" -exec rm -rf {} + 2>/dev/null || true
find . -type d -name "x64" -exec rm -rf {} + 2>/dev/null || true
find . -type d -name "Debug" -exec rm -rf {} + 2>/dev/null || true
find . -type d -name "Release" -exec rm -rf {} + 2>/dev/null || true

echo "\n✅ Clean completed successfully!"
