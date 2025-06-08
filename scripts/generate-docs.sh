#!/bin/bash

# Generate documentation for the project
# Usage: ./scripts/generate-docs.sh [build-dir] [output-dir]

set -e

# Default values
BUILD_DIR=${1:-build}
OUTPUT_DIR=${2:-docs}

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed."
    echo "Please install Doxygen to generate documentation."
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Configure with Doxygen support
echo "Configuring build with Doxygen support..."
cd "${BUILD_DIR}" && cmake -DBUILD_DOCS=ON ..

# Build the docs target
echo "Generating documentation..."
cmake --build . --target docs

# Create output directory
mkdir -p "../${OUTPUT_DIR}"

# Copy documentation to output directory
echo "Copying documentation to ${OUTPUT_DIR}..."
cp -r "${BUILD_DIR}/docs/html/"* "../${OUTPUT_DIR}/"

# Return to the original directory
cd -

echo "Documentation generated successfully!"
echo "Open ${OUTPUT_DIR}/index.html in your browser to view the documentation."
