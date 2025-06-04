#!/bin/bash
# Fast build script for the AI-First TextEditor
# This script provides optimized build settings for Linux/macOS

# Default parameters
CLEAN=false
TARGET=""
CONFIG="Debug"
JOBS=0
UNITY=true
USE_NINJA=false

# Process command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -clean|--clean)
      CLEAN=true
      shift
      ;;
    -target|--target)
      TARGET="$2"
      shift 2
      ;;
    -config|--config)
      CONFIG="$2"
      shift 2
      ;;
    -jobs|--jobs)
      JOBS="$2"
      shift 2
      ;;
    -unity|--unity)
      if [[ "$2" == "false" || "$2" == "0" ]]; then
        UNITY=false
      fi
      shift 2
      ;;
    -ninja|--ninja)
      USE_NINJA=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Auto-detect optimal job count if not specified
if [ "$JOBS" -eq 0 ]; then
  # Detect CPU count based on platform
  case "$(uname -s)" in
    Linux*)
      CPU_COUNT=$(nproc)
      ;;
    Darwin*)  # macOS
      CPU_COUNT=$(sysctl -n hw.logicalcpu)
      ;;
    *)
      CPU_COUNT=4  # Default fallback
      ;;
  esac
  
  # Use CPU count + 2 for optimal performance with hyperthreading
  JOBS=$((CPU_COUNT + 2))
  
  # Limit to a reasonable number to prevent system from becoming unresponsive
  if [ "$JOBS" -gt 16 ]; then
    JOBS=16
  fi
fi

# Create build directory
BUILD_DIR="build_fast"
mkdir -p "$BUILD_DIR"
echo "Using build directory: $BUILD_DIR"

# Clean if requested
if [ "$CLEAN" = true ]; then
  echo -e "\033[33mCleaning build directory...\033[0m"
  rm -rf "${BUILD_DIR:?}"/*
  mkdir -p "$BUILD_DIR"
fi

# Change to build directory
pushd "$BUILD_DIR" > /dev/null

# Set generator and build tool options
GENERATOR="Unix Makefiles"

if [ "$USE_NINJA" = true ]; then
  # Check if ninja is available
  if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
    echo -e "\033[32mUsing Ninja build system for faster builds\033[0m"
  else
    echo -e "\033[33mNinja not found in PATH. Falling back to Unix Makefiles.\033[0m"
    echo -e "\033[33mConsider installing Ninja for faster builds\033[0m"
  fi
fi

# Set environment variable for parallel builds
export CMAKE_BUILD_PARALLEL_LEVEL=$JOBS

# Configure with optimizations
echo -e "\033[36mConfiguring optimized build with $JOBS parallel jobs...\033[0m"
CMAKE_ARGS="-G \"$GENERATOR\" -DCMAKE_BUILD_TYPE=$CONFIG -DENABLE_UNITY_BUILD=$UNITY -DCMAKE_BUILD_PARALLEL_LEVEL=$JOBS"

# Execute cmake configuration
eval cmake .. $CMAKE_ARGS

if [ $? -ne 0 ]; then
  echo -e "\033[31mConfiguration failed\033[0m"
  popd > /dev/null
  exit 1
fi

# Build target
BUILD_TARGET=""
if [ ! -z "$TARGET" ]; then
  BUILD_TARGET="--target $TARGET"
fi

echo -e "\033[36mBuilding with $JOBS parallel jobs...\033[0m"
eval cmake --build . --config "$CONFIG" $BUILD_TARGET --parallel $JOBS

if [ $? -ne 0 ]; then
  echo -e "\033[31mBuild failed\033[0m"
  popd > /dev/null
  exit 1
fi

# Return to original directory
popd > /dev/null

echo -e "\033[32mFast build completed successfully!\033[0m"
echo "Usage examples:"
echo -e "\033[36m  - Basic build:          ./scripts/fast_build.sh\033[0m"
echo -e "\033[36m  - Clean and rebuild:    ./scripts/fast_build.sh -clean\033[0m" 
echo -e "\033[36m  - Build specific target: ./scripts/fast_build.sh -target EditorLib\033[0m"
echo -e "\033[36m  - Release build:        ./scripts/fast_build.sh -config Release\033[0m"
echo -e "\033[36m  - Build with Ninja:     ./scripts/fast_build.sh -ninja\033[0m"
echo -e "\033[36m  - Disable unity builds: ./scripts/fast_build.sh -unity false\033[0m" 