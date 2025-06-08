#!/bin/bash

set -e

echo "Setting up development environment for AI-First Text Editor..."

# Function to detect OS
get_os() {
    case "$(uname -s)" in
        Linux*)     echo "Linux" ;;
        Darwin*)    echo "macOS" ;;
        CYGWIN*|MINGW*|MSYS*) echo "Windows" ;;
        *)          echo "Unknown" ;;
    esac
}

OS=$(get_os)

# Install dependencies based on OS
echo "Detected OS: $OS"

case "$OS" in
    "Linux")
        echo "Installing dependencies for Linux..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            git \
            clang-format \
            clang-tidy \
            lcov \
            python3-pip
        ;;
        
    "macOS")
        echo "Installing dependencies for macOS..."
        # Check if Homebrew is installed
        if ! command -v brew &> /dev/null; then
            echo "Installing Homebrew..."
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        fi
        
        # Install dependencies
        brew update
        brew install \
            cmake \
            git \
            llvm \
            lcov \
            python@3
            
        # Add llvm to PATH
        echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.zshrc
        export PATH="/usr/local/opt/llvm/bin:$PATH"
        ;;
        
    "Windows")
        echo "Installing dependencies for Windows..."
        # Check if Chocolatey is installed
        if ! command -v choco &> /dev/null; then
            echo "Installing Chocolatey..."
            powershell -NoProfile -ExecutionPolicy Bypass -Command \
                "Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
        fi
        
        # Install dependencies
        choco install -y \
            cmake \
            git \
            llvm \
            python \
            ninja \
            visualstudio2022buildtools \
            visualstudio2022-workload-vctools
        ;;
        
    *)
        echo "Unsupported operating system. Please install the following manually:"
        echo "- CMake"
        echo "- Git"
        echo "- Clang/LLVM (for clang-format and clang-tidy)"
        echo "- Python 3"
        echo "- Build tools for your platform"
        exit 1
        ;;
esac

# Install Python packages
echo "Installing Python packages..."
pip3 install --upgrade pip
pip3 install cmake

# Install pre-commit hook
echo "Setting up git hooks..."
chmod +x scripts/setup-git-hooks.sh
./scripts/setup-git-hooks.sh

echo ""
echo "Development environment setup complete!"
echo "Next steps:"
echo "1. Create a build directory: mkdir -p build && cd build"
echo "2. Configure the project: cmake .."
echo "3. Build the project: cmake --build ."
echo "4. Run tests: ctest --output-on-failure"
echo ""
echo "Happy coding!"
