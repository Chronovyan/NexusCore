#!/bin/bash
# Debug-optimized build script for the AI-First TextEditor
# This script provides a specialized debug build with optimizations

# Default values
CLEAN=false
TEST_FILTER=""
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_DIR="build_debug"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -clean|--clean)
      CLEAN=true
      shift
      ;;
    -testFilter=*|--test-filter=*)
      TEST_FILTER="${1#*=}"
      shift
      ;;
    -testFilter|--test-filter)
      TEST_FILTER="$2"
      shift 2
      ;;
    -jobs=*|--jobs=*)
      JOBS="${1#*=}"
      shift
      ;;
    -jobs|--jobs)
      JOBS="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [-clean] [-testFilter=PATTERN] [-jobs=N]"
      exit 1
      ;;
  esac
done

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Clean if requested
if [ "$CLEAN" = true ]; then
  echo "Cleaning debug build directory..."
  rm -rf "$BUILD_DIR"/*
  mkdir -p "$BUILD_DIR"
fi

# Change to the build directory
cd "$BUILD_DIR" || { echo "Failed to change to build directory"; exit 1; }

# Configure with debug optimizations
echo "Configuring optimized debug build..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build with specified number of jobs
echo "Building with $JOBS parallel jobs..."
cmake --build . --config Debug --parallel "$JOBS"

BUILD_RESULT=$?

if [ $BUILD_RESULT -ne 0 ]; then
  echo "Build failed with error code $BUILD_RESULT"
  exit $BUILD_RESULT
fi

# Run specific test if filter is provided
if [ -n "$TEST_FILTER" ]; then
  echo "Running tests matching filter: $TEST_FILTER"
  ctest -C Debug -R "$TEST_FILTER" --output-on-failure
  
  TEST_RESULT=$?
  if [ $TEST_RESULT -eq 0 ]; then
    echo "Tests passed successfully!"
  else
    echo "Tests failed with error code $TEST_RESULT"
    exit $TEST_RESULT
  fi
fi

echo "Debug build completed successfully!"
echo "You can run specific tests with: ./scripts/debug_build.sh -testFilter=CommandTest" 