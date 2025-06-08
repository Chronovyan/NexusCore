#!/bin/bash

# Run the editor with various configurations
# Usage: ./scripts/run-editor.sh [debug|release|profile|valgrind|gdb|...] [args...]

set -e

# Default values
BUILD_TYPE="debug"
EXECUTABLE="editor"
BUILD_DIR="build-${BUILD_TYPE}"
ARGS=()

# Parse command line arguments
case "$1" in
    debug|release|profile|asan|tsan|msan|ubsan)
        BUILD_TYPE="$1"
        shift
        ;;
    --help|-h)
        echo "Usage: $0 [debug|release|profile|asan|tsan|msan|ubsan|gdb|valgrind|rr] [args...]"
        exit 0
        ;;
esac

# Set build directory and executable path
case "$BUILD_TYPE" in
    debug)
        BUILD_DIR="build-debug"
        ;;
    release)
        BUILD_DIR="build-release"
        ;;
    *)
        BUILD_DIR="build-${BUILD_TYPE}"
        ;;
esac

EXECUTABLE_PATH="${BUILD_DIR}/bin/${EXECUTABLE}"

# Check if the executable exists
if [ ! -f "$EXECUTABLE_PATH" ]; then
    echo "Error: Executable not found at $EXECUTABLE_PATH"
    echo "Please build the project first using:"
    echo "  ./scripts/build-${BUILD_TYPE}.sh"
    exit 1
fi

# Run with appropriate settings based on build type
case "$BUILD_TYPE" in
    debug)
        # Run with debug settings
        echo "Running in debug mode..."
        "$EXECUTABLE_PATH" "$@"
        ;;
        
    release)
        # Run with release settings
        echo "Running in release mode..."
        "$EXECUTABLE_PATH" "$@"
        ;;
        
    profile)
        # Run with profiling
        echo "Running with profiling..."
        if command -v perf >/dev/null 2>&1; then
            perf record -g -- "$EXECUTABLE_PATH" "$@"
            echo "Profiling data saved to perf.data"
            echo "To view the profile, run: perf report"
        else
            echo "Warning: 'perf' not found. Running without profiling."
            "$EXECUTABLE_PATH" "$@"
        fi
        ;;
        
    asan)
        # Run with AddressSanitizer
        echo "Running with AddressSanitizer..."
        ASAN_OPTIONS=detect_leaks=1 "$EXECUTABLE_PATH" "$@"
        ;;
        
    tsan)
        # Run with ThreadSanitizer
        echo "Running with ThreadSanitizer..."
        TSAN_OPTIONS=second_deadlock_stack=1 "$EXECUTABLE_PATH" "$@"
        ;;
        
    msan)
        # Run with MemorySanitizer
        echo "Running with MemorySanitizer..."
        MSAN_OPTIONS=poison_in_dtor=1 "$EXECUTABLE_PATH" "$@"
        ;;
        
    ubsan)
        # Run with UndefinedBehaviorSanitizer
        echo "Running with UndefinedBehaviorSanitizer..."
        UBSAN_OPTIONS=print_stacktrace=1 "$EXECUTABLE_PATH" "$@"
        ;;
        
    gdb)
        # Run with GDB
        echo "Running with GDB..."
        gdb --args "$EXECUTABLE_PATH" "$@"
        ;;
        
    valgrind)
        # Run with Valgrind
        echo "Running with Valgrind..."
        if command -v valgrind >/dev/null 2>&1; then
            valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes "$EXECUTABLE_PATH" "$@"
        else
            echo "Error: 'valgrind' not found. Please install it first."
            exit 1
        fi
        ;;
        
    rr)
        # Run with Mozilla rr
        echo "Running with Mozilla rr..."
        if command -v rr >/dev/null 2>&1; then
            rr record "$EXECUTABLE_PATH" "$@"
            echo "Recording complete. To replay, run: rr replay"
        else
            echo "Error: 'rr' not found. Please install it first."
            exit 1
        fi
        ;;
        
    *)
        echo "Error: Unknown run type: $BUILD_TYPE"
        echo "Usage: $0 [debug|release|profile|asan|tsan|msan|ubsan|gdb|valgrind|rr] [args...]"
        exit 1
        ;;
esac
