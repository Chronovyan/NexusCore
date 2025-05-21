# Performance Testing Framework

This document describes the performance testing framework for AI-First TextEditor, which allows for systematic measurement and tracking of editor performance with large files.

## Overview

The framework consists of:

1. A test suite that measures performance metrics for various operations
2. Scripts to run tests and process results
3. Tools to store and analyze performance data over time

## Test Suite

The large file performance tests (`tests/large_file_performance_test.cpp`) measure:

- **File Open Performance**: Time and memory usage when opening files of different sizes
- **Memory During Editing**: Memory usage while editing large files
- **File Save Performance**: Time required to save files of different sizes
- **Scrolling Performance**: Navigation speed in large documents
- **Content Integrity**: Verification that file content is preserved
- **Search & Replace Performance**: Time for search/replace operations

## Running the Tests

### Prerequisites

- CMake build completed (Debug or Release mode)
- Python 3.x installed for results processing

### Windows
```batch
cd benchmarks
run_performance_tests.bat [Debug|Release]
```

### Linux/macOS
```bash
cd benchmarks
./run_performance_tests.sh [Debug|Release]
```

## Results Analysis

The test results are processed into:

1. A raw text file with all test output (`benchmarks/performance_baseline_raw.txt`)
2. A CSV file with structured metrics (`benchmarks/large_file_baselines.csv`)
3. A Markdown report with tables and analysis (`docs/performance_baselines.md`)

## Performance Metrics

The following metrics are captured:

### File Operations
- **Open Time**: Time in milliseconds to open files of different sizes
- **Save Time**: Time in milliseconds to save files of different sizes
- **Memory Usage**: Memory consumption in MB during file operations

### Editing Operations  
- **Text Insertion**: Time to insert text blocks into large files
- **Navigation**: Time to move through documents after edits

### User Interface
- **Scrolling**: Time to scroll up/down by specified number of lines
- **Jump Navigation**: Time to move to beginning/end of file

### Text Processing
- **Search & Replace**: Time to perform pattern replacements

## Tracking Performance Over Time

The CSV format allows for easy tracking of performance changes over time. Each run includes:

- **Date**: When the tests were run
- **Commit Hash**: Git commit identifier
- **System Info**: CPU, RAM, and OS details
- **Build Config**: Debug or Release build

## Adding New Performance Tests

To add a new performance test:

1. Add a new test case to `tests/large_file_performance_test.cpp`
2. Update the `extract_performance_metrics.py` script to extract new metrics
3. Run the tests to verify the new metrics are captured correctly

## Using Results for Optimization

Performance test results can be used to:

1. Establish baseline performance for the editor
2. Identify operations that need optimization
3. Verify performance improvements from code changes
4. Detect performance regressions

## Integration with CI/CD

Future work includes integrating performance testing with the CI/CD pipeline to:

1. Run tests automatically on new commits
2. Compare results against historical baselines
3. Flag significant performance changes
4. Generate trend reports over time 