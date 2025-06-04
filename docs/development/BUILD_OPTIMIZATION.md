# Build Optimization Guide

This document outlines the build optimization strategies implemented in the AI-First TextEditor project to improve build times and developer productivity.

## Implemented Optimizations

### 1. Compiler Caching with ccache

The build system now automatically detects and uses ccache if available, which caches compilation results to speed up rebuilds.

```cmake
# In CMakeLists.txt
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
```

### 2. Precompiled Headers

A comprehensive precompiled header (`TextEditorPCH.h`) has been created that includes frequently used standard library and project headers, significantly reducing compilation time.

```cmake
# In CMakeLists.txt
target_precompile_headers(EditorLib PRIVATE
  "src/TextEditorPCH.h"
)
```

### 3. Unity Builds

Unity builds (also known as jumbo builds) combine multiple source files into a single compilation unit, reducing the overhead of compilation and linking.

```cmake
# In CMakeLists.txt
set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 10) # Adjust based on your project size
```

### 4. Optional Test Builds

Tests are now built only when explicitly requested, reducing build times during regular development.

```cmake
# In CMakeLists.txt
option(BUILD_TESTS "Build the tests" OFF)
if(BUILD_TESTS)
  # Test configuration here...
endif()
```

### 5. Optimized Build Scripts

#### Windows PowerShell Script

A PowerShell script (`scripts/fast_build.ps1`) has been created to streamline the build process with optimized settings. For convenience, a batch file wrapper (`build_fast.bat`) is also provided in the root directory.

- Automatic parallelization based on CPU cores
- Separate build directory for optimized builds
- Option to include/exclude tests
- Support for clean builds and release mode

#### Linux/macOS Shell Script

A Bash script (`scripts/fast_build.sh`) provides similar functionality for Linux and macOS users:

- Automatic detection of available CPU cores
- Same options as the PowerShell script
- Proper error handling and reporting

**Note for Linux/macOS users:** You may need to make the script executable after cloning the repository:
```bash
chmod +x scripts/fast_build.sh
```

## Using the Optimized Build System

### Windows

#### Using the Batch File (Simplest)

```cmd
# Basic build (excludes tests)
build_fast.bat

# Build with tests
build_fast.bat -withTests

# Other options
build_fast.bat -clean -release -jobs 8
```

#### Using the PowerShell Script

```powershell
# Basic build (excludes tests)
.\scripts\fast_build.ps1

# Build with specific number of parallel jobs
.\scripts\fast_build.ps1 -jobs 8

# Build including tests
.\scripts\fast_build.ps1 -withTests

# Clean build
.\scripts\fast_build.ps1 -clean

# Release build
.\scripts\fast_build.ps1 -release

# Combining options
.\scripts\fast_build.ps1 -release -jobs 12 -clean
```

### Linux/macOS

```bash
# Make script executable (first time only)
chmod +x scripts/fast_build.sh

# Basic build (excludes tests)
./scripts/fast_build.sh

# Build with tests
./scripts/fast_build.sh -withTests

# Other options
./scripts/fast_build.sh -clean -release -jobs 8
```

### Manual CMake Configuration

If you prefer to use CMake directly:

```sh
# Configure without tests (faster)
cmake -B build_dir -DBUILD_TESTS=OFF

# Configure with tests
cmake -B build_dir -DBUILD_TESTS=ON

# Build with parallel jobs
cmake --build build_dir --parallel 8
```

## Additional Tips for Faster Builds

1. **Minimize Header Dependencies**
   - Use forward declarations where possible
   - Consider the PIMPL (Pointer to Implementation) pattern for key classes

2. **Install ccache**
   - Install ccache to enable compilation caching: https://ccache.dev/

3. **Avoid Unnecessary Rebuilds**
   - Be careful when modifying frequently included headers
   - Consider using include guards and pragma once consistently

4. **Optimize Include Hierarchy**
   - Audit includes to eliminate unnecessary ones
   - Consider using tools like include-what-you-use

5. **Separate Debug and Release Builds**
   - Maintain separate build directories for debug and release builds

## Future Optimization Opportunities

1. **Use Pre-built Dependencies**
   - Consider using pre-built binaries for large third-party dependencies instead of building them from source

2. **Module System**
   - Prepare for C++20 modules by restructuring code to minimize header dependencies

3. **Distributed Builds**
   - For large teams, consider implementing distributed build systems like IncrediBuild or distcc

4. **Optimize Test Compilation**
   - Fix the interface issues in test code to prevent failed compilations
   - Consider splitting tests into smaller, more focused test suites

5. **Response Files**
   - For MSVC builds with many source files, use response files to avoid command-line length limitations

## Measuring Build Performance

To track build performance improvements:

```powershell
# Measure build time in PowerShell
Measure-Command { .\scripts\fast_build.ps1 }
```

```bash
# Measure build time in Bash
time ./scripts/fast_build.sh
```

## Troubleshooting

### Common Issues

1. **ccache Not Found**
   - Install ccache or disable the ccache optimization in CMakeLists.txt

2. **Unity Build Errors**
   - If unity builds cause compilation errors, adjust the batch size or disable unity builds temporarily:
   ```cmake
   set(CMAKE_UNITY_BUILD OFF)
   ```

3. **Precompiled Header Errors**
   - If precompiled headers cause issues, check for circular dependencies or incompatible headers

### Getting Help

For additional help with build optimizations, consult:
- The CMake documentation: https://cmake.org/documentation/
- The ccache documentation: https://ccache.dev/manual/latest.html 