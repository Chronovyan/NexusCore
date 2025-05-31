#!/bin/bash
# Fast build script for the AI-First TextEditor
# This script provides optimized build settings for Linux/macOS

# Default values
WITH_TESTS=OFF
CLEAN=false
RELEASE=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_DIR="build_fast"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -withTests|--with-tests)
      WITH_TESTS=ON
      shift
      ;;
    -clean|--clean)
      CLEAN=true
      shift
      ;;
    -release|--release)
      RELEASE=true
      shift
      ;;
    -jobs|--jobs)
      JOBS="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [-withTests] [-clean] [-release] [-jobs N]"
      exit 1
      ;;
  esac
done

# Determine build type
BUILD_TYPE=$([ "$RELEASE" = true ] && echo "Release" || echo "Debug")

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Clean if requested
if [ "$CLEAN" = true ]; then
  echo "Cleaning build directory..."
  rm -rf "$BUILD_DIR"/*
  mkdir -p "$BUILD_DIR"
fi

# Change to the build directory
cd "$BUILD_DIR" || { echo "Failed to change to build directory"; exit 1; }

# Configure with optimized settings
echo "Configuring with BUILD_TESTS=$WITH_TESTS, CMAKE_BUILD_TYPE=$BUILD_TYPE"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTS="$WITH_TESTS"

# Build with specified number of jobs
echo "Building with $JOBS parallel jobs..."
cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS"

BUILD_RESULT=$?

# Report results
if [ $BUILD_RESULT -eq 0 ]; then
  echo "Build completed successfully!"
  
  if [ "$WITH_TESTS" = "ON" ]; then
    echo "To run tests: cd $BUILD_DIR && ctest -C $BUILD_TYPE"
  else
    echo "Tests were not built. Use -withTests to include them."
  fi
else
  echo "Build failed with error code $BUILD_RESULT"
  exit $BUILD_RESULT
fi 