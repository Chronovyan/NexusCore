# Rapid Debugging Workflow

This guide outlines the optimized debugging workflow for the AI-First TextEditor project, leveraging our build system optimizations for lightning-fast debug cycles.

## Debug-Optimized Build Scripts

We've created specialized scripts that configure the build system for optimal debugging:

- Debug symbols are enabled for better debugging information
- Tests are automatically included in the build
- Build performance optimizations (ccache, PCH, unity builds) are maintained
- Targeted test execution is supported

## Quick Start

### Windows

```batch
# Run debug build with all tests
debug_build.bat

# Run debug build and execute specific tests
debug_build.bat -testFilter CommandTest

# Clean and rebuild
debug_build.bat -clean
```

### Linux/macOS

```bash
# Make the script executable (first time only)
chmod +x scripts/debug_build.sh

# Run debug build with all tests
./scripts/debug_build.sh

# Run debug build and execute specific tests
./scripts/debug_build.sh -testFilter=CommandTest

# Clean and rebuild
./scripts/debug_build.sh -clean
```

## Debug Cycle Workflow

The optimized debug cycle follows these steps:

1. **Initial Build**: Run the debug build script to create an optimized debug build
2. **Identify Issue**: Use the test output or application behavior to identify issues
3. **Fix Implementation**: Make changes to the source code
4. **Verify Fix**: Run the debug build with targeted tests to verify the fix
5. **Repeat**: Continue the cycle until all issues are resolved

### Example Debug Session

```bash
# Initial build with all tests
debug_build.bat

# After identifying an issue in EditorCommands, update the code and run targeted tests
debug_build.bat -testFilter EditorCommandTest

# Once the fix is confirmed, rebuild with all tests to ensure no regressions
debug_build.bat
```

## Advanced Debug Techniques

### 1. Targeted Test Filtering

You can use partial test names with wildcards to run groups of related tests:

```bash
# Run all command-related tests
debug_build.bat -testFilter Command*

# Run all tests related to deletion
debug_build.bat -testFilter "*Delete*"
```

### 2. Debugging Test Failures

When tests fail, the debug build scripts automatically provide detailed output:

1. The exact test that failed is highlighted
2. Expected vs. actual values are shown where applicable
3. Stack traces are provided for crashes

### 3. Parallel Debugging Workflow

For complex fixes that touch multiple components:

1. Open multiple terminals
2. Run targeted tests for different components in separate terminals
3. Use the `-jobs` parameter to control resource usage:
   ```bash
   # Terminal 1: Run editor command tests with 4 jobs
   debug_build.bat -testFilter CommandTest -jobs 4
   
   # Terminal 2: Run text buffer tests with 4 jobs
   debug_build.bat -testFilter BufferTest -jobs 4
   ```

### 4. Integrating with Visual Studio

If using Visual Studio for debugging:

1. Run `debug_build.bat` to generate the Visual Studio solution
2. Open `build_debug/TextEditor.sln` in Visual Studio
3. Set breakpoints in the source code
4. Run specific tests from Visual Studio's Test Explorer

### 5. Incremental Debugging

For minor changes, you can skip the CMake configuration step:

```bash
# Just build and run tests without reconfiguring
cd build_debug
cmake --build . --config Debug
ctest -C Debug -R "CommandTest" --output-on-failure
```

## Debugging Common Issues

### 1. Command Behavior Issues

When debugging command execution and undo/redo behavior:

```bash
debug_build.bat -testFilter "CommandTest"
```

Look for:
- Proper state restoration in undo operations
- Correct handling of edge cases
- Memory management issues

### 2. Text Buffer Operations

When debugging text buffer operations:

```bash
debug_build.bat -testFilter "BufferTest"
```

Look for:
- Bounds checking issues
- Line ending handling
- Cursor position calculations

### 3. Memory and Resource Issues

For memory leaks and resource management:

```bash
debug_build.bat -testFilter "MemoryTest"
```

### 4. Interface Implementation Issues

For interface implementation bugs:

```bash
debug_build.bat -testFilter "Interface"
```

## Performance Tips

1. **Use ccache**: Ensure ccache is installed for maximum incremental build speed
2. **Targeted Tests**: Always use the most specific test filter possible
3. **Build Directory Reuse**: The debug build scripts reuse the same build directory by default
4. **Concurrent Editing**: Edit files while builds are running to maximize productivity

## Troubleshooting

### Build Failures

1. **Check error messages**: The build scripts provide detailed error information
2. **Verify include paths**: Many build errors stem from incorrect include paths
3. **Check for missing files**: Ensure all referenced files exist

### Test Failures

1. **Examine test output**: Look for specific assertions that failed
2. **Debug iteratively**: Fix one test at a time
3. **Check edge cases**: Many failures happen with empty inputs, large inputs, or special characters 