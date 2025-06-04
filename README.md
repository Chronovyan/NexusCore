# AI-First TextEditor

[![C++ Smoke Test](https://github.com/yourusername/AI-First-TextEditor/actions/workflows/smoke-test.yml/badge.svg)](https://github.com/yourusername/AI-First-TextEditor/actions/workflows/smoke-test.yml)

A modern code editor with deep AI integration as a core principle.

## Current Status

We're currently in Phase 3 (Architecture Refinement) of our [development roadmap](ROADMAP.md). Recent improvements include:

- ✅ Completed interface-based architecture implementation
- ✅ Enhanced dependency injection framework with scoped services
- ✅ Implemented plugin architecture and extension points
- ✅ Implemented multiple cursor and selection support
- ✅ Fixed all interface conversion issues in test framework
- ✅ Added shared test utilities for improved maintainability

See our [CHRONOLOG.md](CHRONOLOG.md) for detailed development history and issue resolutions.

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

### Running Extreme Large File Tests

Extreme large file tests are specialized tests designed to evaluate the editor's performance with exceptionally large files. These tests are disabled by default to prevent automated test runs from hanging.

```powershell
# Run extreme large file tests with PowerShell script (Windows)
.\scripts\run_extreme_tests.ps1

# Run with specific options
.\scripts\run_extreme_tests.ps1 -skipProblematicTests -timeoutSeconds 300

# Run only the most reliable subset of extreme tests
.\scripts\run_reliable_extreme_tests.ps1
```

For detailed information about these tests, see the [Extreme Large File Tests Documentation](docs/extreme_large_file_tests.md).

## Features

- **Advanced Text Editing**: Comprehensive text editing capabilities with robust undo/redo
- **Syntax Highlighting**: Built-in syntax highlighting for multiple languages
- **AI Integration**: Deeply integrated AI assistance for coding and text editing
- **Plugin System**: Extensible architecture allowing for plugins and extensions
- **Performance Optimized**: Handles large files efficiently with virtualized buffers
- **Multiple Cursors**: Edit text at multiple locations simultaneously
  - Add cursors at specific positions
  - Add cursors at all occurrences of selected text
  - Add cursors at specific column positions across multiple lines
  - Perform simultaneous edits at all cursor positions
  - Make selections with all active cursors
  - Test with provided scripts (test_multicursor.ps1 for Windows, test_multicursor.sh for Unix-like systems)
- **Diff and Merge**: Compare and merge different versions of text files
  - Visual diff highlighting between current document and files
  - Three-way merge capabilities for resolving conflicts
  - Interactive conflict resolution
  - Integration with the editor's command system

For detailed information about the diff and merge capabilities, see the [Diff and Merge Documentation](docs/diff_merge_features.md).

# Chronovyan Resource Optimization

## Centralized Configuration System

This project implements a centralized configuration system for the Chronovyan resource optimizer, addressing the dissonance identified in task CD-2023-08-002 regarding magic numbers and hardcoded values.

### Key Components

1. **ResourceConfig Class**
   - Singleton pattern implementation for global access to configuration
   - Stores and provides access to all configuration parameters
   - Default values loaded automatically
   - Support for getting/setting integer and double parameters

2. **ResourceConfigLoader**
   - Utility for loading configuration from files
   - Supports comments and blank lines
   - Handles errors gracefully
   - Validates parameter values

3. **Configuration File Format**
   - Simple key-value pair format
   - Comments start with '#'
   - Organized in logical sections
   - Sample provided in `config/resource_optimization.conf`

### Benefits of the New System

- **Centralization**: All parameters are managed in one place
- **Flexibility**: Easy to adjust values without code changes
- **Maintainability**: No more scattered magic numbers
- **Configurability**: Parameters can be changed at runtime
- **Clarity**: Named parameters instead of mysterious numbers

### Usage Example

```cpp
// Get the resource configuration singleton
ResourceConfig& config = ResourceConfig::getInstance();

// Load configuration from a file
if (!ResourceConfigLoader::loadFromFile("config/resource_optimization.conf", config)) {
    // Handle error - default values will remain
}

// Use configuration values in code
double threshold = config.getDoubleParam("chronons_threshold");
int threadCount = config.getIntParam("default_thread_count");
```

### Demo Program

A demo program is provided in `examples/resource_config_demo.cpp` that showcases:

- Loading configuration from a file
- Using the configuration with the ResourceOptimizer
- Demonstrating various optimization methods

### Building and Running

```bash
# Build the demo
make

# Run with default configuration
./build/resource_config_demo

# Run with custom configuration file
./build/resource_config_demo config/resource_optimization.conf
```

## Development

### Adding New Parameters

1. Add the parameter with a default value in `ResourceConfig::loadDefaults()`
2. Add the parameter to the sample configuration file
3. Use the parameter in code via `getIntParam()` or `getDoubleParam()`

### Future Enhancements

- Support for additional parameter types (string, boolean)
- Hierarchical configuration structure
- Configuration validation rules
- Runtime reconfiguration API