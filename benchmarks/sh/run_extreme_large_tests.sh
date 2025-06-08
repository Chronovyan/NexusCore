#!/bin/bash

# Check if build config is provided
if [ $# -lt 1 ]; then
    echo "Usage: ./run_extreme_large_tests.sh [Debug|Release] [ultra]"
    echo
    echo "Parameters:"
    echo "  Debug|Release  - Build configuration to use"
    echo "  ultra          - Optional parameter to enable ultra large tests (500MB)"
    exit 1
fi

BUILD_CONFIG=$1
if [ "$BUILD_CONFIG" != "Debug" ] && [ "$BUILD_CONFIG" != "Release" ]; then
    echo "Invalid build config. Use Debug or Release."
    exit 1
fi

# Check for ultra large test flag
ULTRA_LARGE_TESTS=""
if [ "$2" = "ultra" ]; then
    ULTRA_LARGE_TESTS=1
    echo "Ultra large tests (500MB) enabled"
fi

# Set paths
TEST_EXE="../build/$BUILD_CONFIG/ExtremeLargeFileTest"
RESULTS_DIR="test_results"
LOG_FILE="$RESULTS_DIR/extreme_large_tests_$BUILD_CONFIG.log"
CSV_FILE="$RESULTS_DIR/extreme_large_performance.csv"

# Create results directory if it doesn't exist
mkdir -p "$RESULTS_DIR"

# Check if test executable exists
if [ ! -f "$TEST_EXE" ]; then
    echo "Error: $TEST_EXE not found"
    echo "Please build the project with:"
    echo "  cmake --build build --config $BUILD_CONFIG --target ExtremeLargeFileTest"
    exit 1
fi

# Run the tests
echo "Running extreme large file tests in $BUILD_CONFIG mode..."
echo "Test results will be written to: $LOG_FILE"

# Export environment variable if ultra large tests enabled
if [ -n "$ULTRA_LARGE_TESTS" ]; then
    echo "Running with ULTRA_LARGE_TESTS=1"
    export ULTRA_LARGE_TESTS=1
fi

# Run the test and capture output
"$TEST_EXE" > "$LOG_FILE" 2>&1
TEST_RESULT=$?

# Check test result
if [ $TEST_RESULT -ne 0 ]; then
    echo "Tests failed with error code: $TEST_RESULT"
    echo "See $LOG_FILE for details"
    exit $TEST_RESULT
fi

echo
echo "Tests completed successfully"
echo "Results saved to: $LOG_FILE"

# Extract key performance metrics to CSV
echo "Extracting performance metrics to CSV..."
echo "Timestamp,FileSize,OpenTime,SaveTime,SearchTime,ScrollTime,InsertTime,MemoryRatio" > "$CSV_FILE"

# Parse log file and extract metrics
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

# Function to extract value from log using regex
extract_value() {
    local pattern="$1"
    grep -o "$pattern" "$LOG_FILE" | sed -E "s/$pattern/\\1/"
}

# Process each file size
for SIZE in "MediumLarge" "VeryLarge" "ExtremeLarge" "UltraLarge"; do
    # Extract metrics using regex
    OPEN_TIME=$(extract_value "\\[$SIZE\\] File open time: ([0-9.]+) ms")
    SAVE_TIME=$(extract_value "\\[$SIZE\\] File save time: ([0-9.]+) ms")
    SEARCH_TIME=$(extract_value "\\[$SIZE\\] Average search time: ([0-9.]+) ms")
    SCROLL_TIME=$(extract_value "\\[$SIZE\\] Average page down time: ([0-9.]+) ms")
    INSERT_TIME=$(extract_value "\\[$SIZE\\] Text insertion time: ([0-9.]+) ms")
    MEMORY_RATIO=$(extract_value "\\[$SIZE\\] Memory/File ratio: ([0-9.]+)")
    
    # Only process if we have data for this size
    if [ -n "$OPEN_TIME" ]; then
        # Determine file size in MB
        case "$SIZE" in
            "MediumLarge") FILE_SIZE="12" ;;
            "VeryLarge") FILE_SIZE="50" ;;
            "ExtremeLarge") FILE_SIZE="150" ;;
            "UltraLarge") FILE_SIZE="500" ;;
            *) FILE_SIZE="0" ;;
        esac
        
        # Add to CSV
        echo "$TIMESTAMP,$FILE_SIZE,$OPEN_TIME,$SAVE_TIME,$SEARCH_TIME,$SCROLL_TIME,$INSERT_TIME,$MEMORY_RATIO" >> "$CSV_FILE"
    fi
done

echo "Performance metrics saved to: $CSV_FILE"
echo
echo "To run tests with Ultra Large files (500MB), use:"
echo "  ./run_extreme_large_tests.sh $BUILD_CONFIG ultra"
echo

# Make the script executable
chmod +x "run_extreme_large_tests.sh" 