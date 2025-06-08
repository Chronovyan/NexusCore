#!/bin/bash

# Generate code coverage report
# Usage: ./scripts/run-coverage.sh [build-dir] [output-dir]

set -e

# Default values
BUILD_DIR=${1:-build}
OUTPUT_DIR=${2:-coverage}

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo "Error: lcov is not installed."
    echo "Please install lcov to generate coverage reports."
    exit 1
fi

# Check if genhtml is installed
if ! command -v genhtml &> /dev/null; then
    echo "Error: genhtml is not installed."
    echo "Please install lcov to generate HTML coverage reports."
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Configure with coverage support
echo "Configuring build with coverage support..."
cd "${BUILD_DIR}" && cmake -DENABLE_COVERAGE=ON ..

# Build the project
echo "Building with coverage instrumentation..."
cmake --build . --clean-first

# Run tests
echo "Running tests with coverage..."
ctest --output-on-failure

# Create output directory
mkdir -p "../${OUTPUT_DIR}"

# Generate coverage data
echo "Generating coverage report..."
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '*/tests/*' '*/third_party/*' '*/external/*' '*/moc_*.cpp' '*/qrc_*.cpp' '*/ui_*.h' '*/moc_*.h' '*/qrc_*.h' '*/qrc_*.cpp' '*/moc_*.cpp' '*/ui_*.h' '*/moc_*.h' '*/qrc_*.h' -o coverage_filtered.info

# Generate HTML report
genhtml coverage_filtered.info --output-directory "../${OUTPUT_DIR}" --demangle-cpp --legend --title "AI-First Text Editor Coverage"

# Return to the original directory
cd -

echo "Coverage report generated successfully!"
echo "Open ${OUTPUT_DIR}/index.html in your browser to view the coverage report."
