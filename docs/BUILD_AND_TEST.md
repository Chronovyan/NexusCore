# Building and Testing the TextEditor Project

This document outlines the steps to build the TextEditor project and run its associated tests using CMake and CTest.

## Prerequisites

*   **CMake**: Version 3.14 or higher.
*   **C++ Compiler**: A C++17 compatible compiler (e.g., GCC, Clang, MSVC).
*   **Git**: Required for fetching GoogleTest if not already present.

## Project Structure Overview

*   The main project (`CMakeLists.txt`) defines:
    *   `EditorLib`: A static library built from sources in the `src/` directory.
    *   `TextEditor`: The main application executable.
*   **Testing**:
    *   GoogleTest is used as the testing framework and is fetched automatically by CMake.
    *   Test source files are located in the `tests/` directory.
    *   The `tests/CMakeLists.txt` file defines a test executable named `runTests`.
    *   CTest is configured to discover and run these tests.

## Build and Test Procedure

These commands should be executed from the **root directory** of the project (i.e., the directory containing the main `CMakeLists.txt` file).

### 1. Configure the Build (Generate Build Files)

It's standard practice to perform an "out-of-source" build. This keeps your source directory clean by placing all build artifacts in a separate directory (commonly named `build`).

```bash
# Create a build directory (if it doesn't exist)
mkdir build

# Navigate into the build directory
cd build

# Run CMake to configure the project and generate build files
# This will prepare the build system (e.g., Makefiles, Visual Studio solution)
cmake ..
```

**Notes on Configuration:**

*   If you want to use a specific CMake generator (e.g., for a particular IDE or build tool), you can specify it with the `-G` option. For example, for Visual Studio 2022:
    ```bash
    cmake -G "Visual Studio 17 2022" ..
    ```
*   On subsequent builds, if you haven't changed `CMakeLists.txt` files, you typically don't need to re-run the `cmake ..` command unless you want to change build configurations.

### 2. Build the Project (Compile Code)

Once CMake has successfully configured the project, compile all targets (libraries, executables, and tests).

Still inside the `build` directory:

```bash
# Compile the project
cmake --build .
```

**Alternative Build Methods:**

*   **Makefiles (Linux/macOS):** If Makefiles were generated, you could run `make` directly in the `build` directory.
*   **Visual Studio (Windows):** If a Visual Studio solution (`.sln`) was generated, you can open it and build from within the IDE.
*   The `cmake --build .` command is generally preferred for its portability across different build systems.

**Building Specific Targets:**

You can build a specific target (e.g., only the tests) if needed:

```bash
# Example: Build only the test executable
cmake --build . --target runTests
```

### 3. Run the Tests

After a successful build, you can execute the tests using CTest. CTest will automatically discover and run the tests defined in `runTests`.

Still inside the `build` directory:

```bash
# Run all discovered tests
ctest
```

**Test Output Options:**

*   For more detailed (verbose) output from CTest:
    ```bash
    ctest -V
    ```
*   To see output from disabled tests:
    ```bash
    ctest --output-on-failure
    ```

**Running Test Executable Directly (Advanced):**

You can also run the `runTests` executable directly. This is useful for debugging or passing specific GoogleTest command-line flags. The executable will be located within your `build` directory (e.g., `build/tests/runTests` on Linux/macOS, or `build/Debug/tests/runTests.exe` on Windows if using a Debug configuration with MSVC).

From the `build` directory:

```bash
# Example for Linux/macOS:
./tests/runTests

# Example for Windows (assuming Debug build):
tests\\Debug\\runTests.exe
```

To pass GoogleTest flags (e.g., filtering tests):

```bash
# Run only tests in MyTestSuite
./tests/runTests --gtest_filter=MyTestSuite.*
```

## Quick Summary of Commands (from project root)

```bash
# 1. Create build directory and configure (if first time or CMake files changed)
cmake -S . -B build
# Or, if already in 'build' directory created previously: cmake ..

# 2. Build the project (from the 'build' directory)
# cd build (if not already there)
cmake --build .
# cd .. (to return to project root if desired)

# 3. Run tests (from the 'build' directory)
# cd build (if not already there)
ctest
# cd .. (to return to project root if desired)
```

This document should provide a comprehensive guide to building and testing the TextEditor. 