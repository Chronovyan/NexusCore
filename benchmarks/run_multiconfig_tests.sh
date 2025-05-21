#!/bin/bash

echo "Running performance tests in both Debug and Release modes..."

# Run Debug tests
echo
echo "===== Running Debug Tests ====="
./run_performance_tests.sh Debug
if [ $? -ne 0 ]; then
    echo "Debug tests failed"
    exit 1
fi

# Run Release tests
echo
echo "===== Running Release Tests ====="
./run_performance_tests.sh Release
if [ $? -ne 0 ]; then
    echo "Release tests failed"
    exit 1
fi

echo
echo "All performance tests completed successfully"
echo "Results are available in:"
echo "- benchmarks/large_file_baselines.csv"
echo "- docs/performance_comparisons.md"
echo "- benchmarks/performance_trends_*.png" 