# Extreme Large File Test Documentation

## Overview

The `ExtremeLargeFileTest` suite is designed to evaluate the performance and reliability of the text editor when handling exceptionally large files. These tests help ensure that the editor maintains acceptable performance characteristics and memory usage patterns even with large files.

## Test Categories

The test suite includes the following tests:

1. **MediumFilePerformance**: Tests performance with 2MB files (reduced from 10MB in the original design).
2. **LargeFilePerformance**: Tests performance with 10MB files (reduced from 50MB in the original design).
3. **MemoryRecoveryAfterLargeFile**: Tests memory cleanup after working with large files.

## Key Operations Tested

Each performance test evaluates:

- File opening performance
- File saving performance
- Scrolling performance
- Text searching performance
- Text insertion performance

## Running the Tests

### Basic Usage

By default, the extreme large file tests are **disabled** to prevent automated test runs from hanging. To run these tests, you must explicitly enable them using the provided script:

```powershell
.\scripts\run_extreme_tests.ps1
```

### Command Line Options

The script supports several options:

- `-testFilter`: Specify which tests to run (default: "ExtremeLargeFileTest.*")
- `-timeoutSeconds`: Maximum allowed runtime before force-termination (default: 180 seconds)
- `-skipProblematicTests`: Skip tests known to cause hangs (use this flag to avoid problematic tests)
- `-jobs`: Number of parallel jobs for building (default: 8)

Examples:

```powershell
# Run only the medium file tests
.\scripts\run_extreme_tests.ps1 -testFilter "ExtremeLargeFileTest.MediumFilePerformance"

# Run all tests with a 5-minute timeout
.\scripts\run_extreme_tests.ps1 -timeoutSeconds 300

# Run tests but skip any known problematic operations
.\scripts\run_extreme_tests.ps1 -skipProblematicTests
```

### Running a Reliable Subset

If you want to run only the most reliable tests that are less likely to hang, use the dedicated script:

```powershell
.\scripts\run_reliable_extreme_tests.ps1
```

This script:
- Runs only the medium file performance tests
- Automatically enables the `-skipProblematicTests` flag
- Uses a shorter default timeout (120 seconds)

It's ideal for:
- Quick verification of basic performance
- CI/CD pipelines where stability is crucial
- Initial testing before running the full suite

## Handling Test Hangs

These tests can occasionally hang due to the nature of processing very large files. The test suite implements several strategies to mitigate this:

1. **Operation Timeouts**: Individual operations have a 15-second timeout
2. **Process-Level Timeout**: The entire test process is terminated after the specified timeout (default: 3 minutes)
3. **Problematic Test Skipping**: Known problematic tests can be skipped using the `-skipProblematicTests` flag

If a test does hang despite these precautions, you may need to manually terminate the process:

1. Press Ctrl+C in the terminal running the tests
2. If that doesn't work, use Task Manager to end the "ExtremeLargeFileTest.exe" process

## Modifying the Tests

### Adjusting File Sizes

If the tests are consistently hanging with the current file sizes, you can reduce them further by modifying the constants in the test file:

```cpp
// File size constants - reduced for stability
static constexpr size_t MEDIUM_SIZE = 2 * 1024 * 1024;  // 2 MB
static constexpr size_t LARGE_SIZE = 10 * 1024 * 1024;  // 10 MB
```

### Marking Problematic Tests

If specific operations consistently cause problems, mark them as problematic in the `ProblematicTests` struct:

```cpp
struct ProblematicTests {
    static constexpr bool LARGE_FILE_OPEN = true;   // Mark as problematic
    static constexpr bool LARGE_FILE_SEARCH = true; // Mark as problematic
    static constexpr bool MEDIUM_FILE_SAVE = false; // Not problematic
};
```

## Performance Thresholds

The tests compare operation times against predefined thresholds:

```cpp
// Initialize thresholds for different file sizes
thresholds[MEDIUM_SIZE] = {1000, 1000, 300, 500, 200}; // openMs, saveMs, scrollMs, searchMs, insertMs
thresholds[LARGE_SIZE] = {3000, 3000, 600, 1500, 500}; // openMs, saveMs, scrollMs, searchMs, insertMs
```

Adjust these thresholds if needed based on your hardware and performance expectations.

## Test Implementation Details

### Environment Variables

- `ENABLE_EXTREME_TESTS`: Set to "1" to enable the tests (otherwise they're skipped)
- `SKIP_PROBLEMATIC_TESTS`: Set to "1" to skip known problematic test operations

### Timeouts

- Individual operations: 15 seconds (`OPERATION_TIMEOUT_MS`)
- Overall test suite: 60 seconds internal timeout (`TEST_TIMEOUT_MS`), plus the process-level timeout

### Error Handling

The test implementation cannot forcibly terminate hung threads (a limitation of C++), but it:

1. Detects timeouts and marks tests as failed
2. Sets cancellation flags that well-behaved operations can check
3. Provides warnings about potentially hung background threads
4. Uses process-level timeouts via PowerShell to terminate the entire process if needed

## Maintenance

When maintaining these tests:

1. If a test consistently hangs, mark it as problematic or adjust its parameters
2. Consider reducing file sizes if necessary for stability
3. Keep timeouts reasonable but sufficient for the operations being tested
4. Remember that the primary goal is to detect regressions in performance, not to test with unrealistically large files 