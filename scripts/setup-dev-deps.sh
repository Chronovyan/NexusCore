#!/bin/bash

# Install development dependencies for the project
# This script installs the necessary tools and libraries for development

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install packages
install_packages() {
    local packages=("$@")
    
    if command_exists apt-get; then
        # Debian/Ubuntu
        echo -e "${YELLOW}Updating package list...${NC}"
        sudo apt-get update
        echo -e "${YELLOW}Installing packages: ${packages[*]}${NC}"
        sudo apt-get install -y "${packings[@]}" || {
            echo -e "${RED}Failed to install packages. Trying with --fix-missing...${NC}"
            sudo apt-get install -y --fix-missing "${packages[@]}" || {
                echo -e "${RED}Failed to install packages. Please install them manually.${NC}"
                return 1
            }
        }
    elif command_exists dnf; then
        # Fedora
        echo -e "${YELLOW}Installing packages: ${packages[*]}${NC}"
        sudo dnf install -y "${packages[@]}"
    elif command_exists yum; then
        # CentOS/RHEL
        echo -e "${YELLOW}Installing packages: ${packages[*]}${NC}"
        sudo yum install -y "${packages[@]}"
    elif command_exists pacman; then
        # Arch Linux
        echo -e "${YELLOW}Installing packages: ${packages[*]}${NC}"
        sudo pacman -S --noconfirm --needed "${packages[@]}"
    elif command_exists brew; then
        # macOS with Homebrew
        echo -e "${YELLOW}Installing packages: ${packages[*]}${NC}"
        brew install "${packages[@]}"
    else
        echo -e "${RED}Package manager not supported. Please install the following packages manually:${NC}"
        printf "%s\n" "${packages[@]}"
        return 1
    fi
}

# Main function
main() {
    echo -e "${GREEN}Setting up development environment...${NC}"
    
    # Install basic build tools
    local build_tools=(
        git cmake ninja-build
        build-essential autoconf automake libtool pkg-config
    )
    
    # Install compiler and debugger
    local compiler_tools=(
        gcc g++ clang clang-tidy clang-format lldb
    )
    
    # Install development libraries
    local dev_libs=(
        libgl1-mesa-dev libglu1-mesa-dev
        libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
        libasound2-dev libpulse-dev
    )
    
    # Install Python and tools
    local python_tools=(
        python3 python3-pip python3-venv
    )
    
    # Install documentation tools
    local doc_tools=(
        doxygen graphviz
    )
    
    # Install all packages
    install_packages "${build_tools[@]}" "${compiler_tools[@]}" "${dev_libs[@]}" "${python_tools[@]}" "${doc_tools[@]}"
    
    # Install Python packages
    echo -e "${YELLOW}Installing Python packages...${NC}"
    pip3 install --upgrade pip
    pip3 install conan cmake-format cmake-format pre-commit
    
    # Setup pre-commit hooks
    echo -e "${YELLOW}Setting up pre-commit hooks...${NC}"
    if command_exists pre-commit; then
        pre-commit install
    else
        echo -e "${YELLOW}pre-commit not found. Skipping pre-commit setup.${NC}"
    fi
    
    echo -e "${GREEN}Development environment setup complete!${NC}"
    echo -e "You can now build the project with:\n"
    echo -e "  mkdir -p build && cd build"
    echo -e "  cmake .."
    echo -e "  cmake --build ."
}

# Run the main function
main "$@"
