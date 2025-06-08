#!/bin/bash

# Run the test suite for the project
# Usage: ./scripts/run-tests.sh [build-dir] [ctest-args...]

set -e

# Default values
BUILD_DIR=${1:-build}
shift

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo "Error: Build directory '${BUILD_DIR}' does not exist."
    echo "Please build the project first or specify the build directory."
    exit 1
fi

# Run tests with ctest
echo "Running tests in ${BUILD_DIR}..."
cd "${BUILD_DIR}" && ctest --output-on-failure "$@"

# Capture the exit code
EXIT_CODE=$?

# Return to the original directory
cd -

# Exit with the test result code
exit ${EXIT_CODE}
