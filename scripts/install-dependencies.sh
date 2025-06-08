#!/bin/bash

# Install project dependencies using vcpkg
# Usage: ./scripts/install-dependencies.sh [vcpkg-root] [triplet]

set -e

# Default values
VCPKG_ROOT="${1:-${VCPKG_ROOT:-$HOME/vcpkg}}"
TRIPLET="${2:-x64-linux}"

# Check if vcpkg is installed
if [ ! -d "$VCPKG_ROOT" ]; then
    echo "vcpkg not found at $VCPKG_ROOT"
    read -p "Would you like to install vcpkg? [Y/n] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]] || [ -z "$REPLY" ]; then
        echo "Cloning vcpkg repository..."
        git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_ROOT"
        cd "$VCPKG_ROOT"
        ./bootstrap-vcpkg.sh
        cd -
    else
        echo "Please install vcpkg manually and try again."
        exit 1
    fi
fi

# Add vcpkg to PATH if not already there
if [[ ":$PATH:" != *":$VCPKG_ROOT:"* ]]; then
    export PATH="$VCPKG_ROOT:$PATH"
    echo "Added vcpkg to PATH"
fi

# Install dependencies
echo "Installing dependencies using vcpkg..."
"$VCPKG_ROOT/vcpkg" install \
    --triplet "$TRIPLET" \
    imgui[glfw-binding,opengl3-binding] \
    glfw3 \
    stb \
    glm \
    fmt \
    spdlog \
    nlohmann-json \
    cpr \
    doctest \
    catch2

# Integrate with CMake
echo "Setting up vcpkg CMake toolchain..."
"$VCPKG_ROOT/vcpkg" integrate install

echo "Dependencies installed successfully!"
echo "You can now configure the project with CMake using:"
echo "  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
