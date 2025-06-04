#!/bin/bash

# Check if build config is provided
if [ -z "$1" ]; then
    echo "Usage: ./run_performance_tests.sh [Debug|Release]"
    exit 1
fi

BUILD_CONFIG="$1"
if [ "$BUILD_CONFIG" != "Debug" ] && [ "$BUILD_CONFIG" != "Release" ]; then
    echo "Invalid build config. Use Debug or Release."
    exit 1
fi

# Set paths
TEST_EXE="../build/$BUILD_CONFIG/LargeFilePerformanceTest"
RAW_OUTPUT="performance_baseline_raw.txt"
CSV_OUTPUT="large_file_baselines.csv"

# Check if test executable exists
if [ ! -f "$TEST_EXE" ]; then
    echo "Error: $TEST_EXE not found"
    echo "Please build the project first"
    exit 1
fi

# Make test executable
chmod +x "$TEST_EXE"

# Run the performance test
echo "Running performance tests in $BUILD_CONFIG mode..."
"$TEST_EXE" > "$RAW_OUTPUT"

# Update system details in CSV
echo "Updating system details in CSV..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    sed -i '' "s_TBD,TBD,TBD_${COMMIT_HASH},Intel Core i7-3630QM @ 2.40GHz,16GB_g" "${CSV_OUTPUT}"
else
    # Linux
    sed -i "s_TBD,TBD,TBD_${COMMIT_HASH},Intel Core i7-3630QM @ 2.40GHz,16GB_g" "${CSV_OUTPUT}"
fi

# Extract metrics
echo "Extracting performance metrics..."
python3 extract_performance_metrics.py "$RAW_OUTPUT" "$BUILD_CONFIG"

# Compare with previous baseline
echo "Comparing with previous baseline..."
python3 compare_baselines.py "$CSV_OUTPUT"

# Generate trend visualizations
echo "Generating trend visualizations..."
python3 visualize_trends.py "$CSV_OUTPUT"

echo "Performance testing completed successfully"
echo "Results written to:"
echo "- $RAW_OUTPUT"
echo "- $CSV_OUTPUT"
echo "- docs/performance_comparisons.md"
echo "- benchmarks/performance_trends_*.png" 