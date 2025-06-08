#!/bin/bash

# Run static code analysis using clang-tidy
# Usage: ./scripts/analyze-code.sh [build-dir] [source-dir]

set -e

# Default values
BUILD_DIR=${1:-build}
SOURCE_DIR=${2:-.}

# Check if clang-tidy is installed
if ! command -v clang-tidy &> /dev/null; then
    echo "Error: clang-tidy is not installed."
    echo "Please install clang-tidy to run static analysis."
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Generate compilation database if it doesn't exist
if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
    echo "Generating compilation database..."
    cd "${BUILD_DIR}" && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
    cd ..
fi

# Run clang-tidy
echo "Running clang-tidy on source files..."
find "${SOURCE_DIR}/src" "${SOURCE_DIR}/include" -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) \
    -not -path "*/build/*" \
    -not -path "*/third_party/*" \
    -not -path "*/external/*" \
    -not -path "*/.git/*" \
    -not -path "*/.vscode/*" \
    -not -path "*/.idea/*" \
    -not -path "*/cmake-*" | xargs -I{} clang-tidy -p "${BUILD_DIR}" {}

echo "Static analysis completed successfully!"
