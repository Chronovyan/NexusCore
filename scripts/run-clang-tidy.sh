#!/bin/bash

# Run clang-tidy on the codebase
# Usage: ./scripts/run-clang-tidy.sh [build-dir] [source-dir] [extra-args...]

set -e

# Default values
BUILD_DIR=${1:-build}
SOURCE_DIR=${2:-.}
shift $(($# > 2 ? 2 : $#))

# Check if clang-tidy is installed
if ! command -v clang-tidy &> /dev/null; then
    echo "Error: clang-tidy is not installed."
    exit 1
fi

# Ensure build directory exists and has compile_commands.json
if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
    echo "Generating compile_commands.json..."
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}" && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && cd ..
    
    if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
        echo "Error: Failed to generate compile_commands.json"
        exit 1
    fi
fi

# Run clang-tidy
RUN_TIDY="clang-tidy -p ${BUILD_DIR} $@"

# Find all C++ files in the source directory
FILES=$(find "${SOURCE_DIR}" -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) \
    -not -path "*/build/*" \
    -not -path "*/third_party/*" \
    -not -path "*/external/*" \
    -not -path "*/.git/*" \
    -not -path "*/.vscode/*" \
    -not -path "*/.idea/*" \
    -not -path "*/cmake-*")

# Run clang-tidy on all files
echo "Running clang-tidy on ${#FILES[@]} files..."
for file in ${FILES}; do
    echo "Checking ${file}"
    ${RUN_TIDY} "${file}" -- \
        -I"${SOURCE_DIR}/include" \
        -I"${BUILD_DIR}/_deps" \
        -std=c++20 \
        -Wall -Wextra -Werror \
        -Wno-unused-parameter \
        -Wno-unused-variable \
        -Wno-unused-function
    
    if [ $? -ne 0 ]; then
        echo "Error: clang-tidy found issues in ${file}"
        exit 1
    fi
done

echo "clang-tidy check completed successfully!"
