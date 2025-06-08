#!/bin/bash
echo "Running Dependency Injection Framework Tests..."

# Get the directory where this script is located
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR/.."

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Build the tests
cmake ..
make di_tests

# Run the tests
if [ -f "tests/di_tests" ]; then
    echo ""
    echo "===== DI Tests Results ====="
    ./tests/di_tests
else
    echo ""
    echo "Build failed or tests executable not found."
    echo "Please check the build log for errors."
fi

echo ""
echo "Done." 