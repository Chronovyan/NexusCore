# AI-First TextEditor

[![C++ Smoke Test](https://github.com/yourusername/AI-First-TextEditor/actions/workflows/smoke-test.yml/badge.svg)](https://github.com/yourusername/AI-First-TextEditor/actions/workflows/smoke-test.yml)

A modern code editor with deep AI integration as a core principle.

## Documentation

For comprehensive documentation, please refer to:

- [Project Overview](docs/project_overview.md) - Complete introduction to the project
- [Build Optimization Guide](docs/BUILD_OPTIMIZATION.md) - Guide to faster build times
- [Rapid Debugging Workflow](docs/RAPID_DEBUGGING.md) - Lightning-fast debugging process
- [Documentation Index](docs/index.md) - All documentation resources

## Quick Start

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake 3.15+
- OpenAI API key (for AI features)
- [ccache](https://ccache.dev/) (optional, for faster builds)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/AI-First-TextEditor.git
cd AI-First-TextEditor

# Initialize submodules
git submodule update --init --recursive

# Option 1: Fast build with PowerShell script (Windows)
.\scripts\fast_build.ps1

# Option 2: Fast build with Batch file (Windows, easier)
build_fast.bat

# Option 3: Fast build with Shell script (Linux/macOS)
chmod +x scripts/fast_build.sh  # Make executable (first time only)
./scripts/fast_build.sh

# Option 4: Manual CMake build
mkdir build && cd build
cmake ..
cmake --build . --parallel
```

For detailed build options and optimizations, see the [Build Optimization Guide](docs/BUILD_OPTIMIZATION.md).

### Running Tests

```bash
# Option 1: Build with tests using PowerShell script (Windows)
.\scripts\fast_build.ps1 -withTests

# Option 2: Build with tests using Batch file (Windows)
build_fast.bat -withTests

# Option 3: Build with tests using Shell script (Linux/macOS)
./scripts/fast_build.sh -withTests

# Option 4: Manual CMake configuration with tests enabled
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
cmake --build .

# Run all tests
ctest -C Debug

# Run only critical tests (faster)
./tests/Debug/RunAllTests --gtest_filter=*_Critical*
```