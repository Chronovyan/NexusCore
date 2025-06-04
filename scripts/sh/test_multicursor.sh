#!/bin/bash
# MultiCursor Feature Test Script
# This script builds and runs the multiple cursor tests
#
# NOTE: On Unix-like systems (Linux/macOS), make this script executable with:
# chmod +x scripts/test_multicursor.sh

# Parse command line arguments
SKIP_BUILD=0
ONLY_BUILD_TESTS=0

while [[ $# -gt 0 ]]; do
  case $1 in
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --only-build-tests)
      ONLY_BUILD_TESTS=1
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--skip-build] [--only-build-tests]"
      exit 1
      ;;
  esac
done

# Set error handling
set -e

# Get script and root directories
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"

echo -e "\033[36m=== MultiCursor Feature Test Script ===\033[0m"
echo -e "\033[90mRoot directory: $ROOT_DIR\033[0m"

# Build the tests if not skipping
if [ $SKIP_BUILD -eq 0 ]; then
    echo -e "\n\033[33mStep 1: Building tests...\033[0m"
    
    # Create build directory if it doesn't exist
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "\033[90mCreating build directory...\033[0m"
        mkdir -p "$BUILD_DIR"
    fi
    
    # Navigate to build directory
    pushd "$BUILD_DIR" > /dev/null
    
    # Configure with CMake
    echo -e "\033[90mConfiguring with CMake...\033[0m"
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
    
    # Build tests
    echo -e "\033[90mBuilding tests...\033[0m"
    cmake --build . --config Debug --target MultiCursorTest EditorMultiCursorTest
    
    # Build the main application too if we're not only building tests
    if [ $ONLY_BUILD_TESTS -eq 0 ]; then
        echo -e "\033[90mBuilding main application...\033[0m"
        cmake --build . --config Debug --target AITextEditor
    fi
    
    # Return to original directory
    popd > /dev/null
fi

# Run the tests if we're not just building
if [ $ONLY_BUILD_TESTS -eq 0 ]; then
    echo -e "\n\033[33mStep 2: Running MultiCursor tests...\033[0m"
    
    MULTICURSOR_TEST_PATH="$BUILD_DIR/tests/MultiCursorTest"
    EDITOR_MULTICURSOR_TEST_PATH="$BUILD_DIR/tests/EditorMultiCursorTest"
    
    # Run MultiCursorTest
    if [ -f "$MULTICURSOR_TEST_PATH" ]; then
        echo -e "\n\033[35mRunning MultiCursorTest...\033[0m"
        "$MULTICURSOR_TEST_PATH"
        if [ $? -eq 0 ]; then
            echo -e "\033[32mMultiCursorTest passed successfully!\033[0m"
        else
            echo -e "\033[31mMultiCursorTest failed with exit code $?\033[0m"
        fi
    else
        echo -e "\033[31mMultiCursorTest executable not found at $MULTICURSOR_TEST_PATH\033[0m"
    fi
    
    # Run EditorMultiCursorTest
    if [ -f "$EDITOR_MULTICURSOR_TEST_PATH" ]; then
        echo -e "\n\033[35mRunning EditorMultiCursorTest...\033[0m"
        "$EDITOR_MULTICURSOR_TEST_PATH"
        if [ $? -eq 0 ]; then
            echo -e "\033[32mEditorMultiCursorTest passed successfully!\033[0m"
        else
            echo -e "\033[31mEditorMultiCursorTest failed with exit code $?\033[0m"
        fi
    else
        echo -e "\033[31mEditorMultiCursorTest executable not found at $EDITOR_MULTICURSOR_TEST_PATH\033[0m"
    fi
    
    # Offer to run the main application for manual testing
    APP_PATH="$BUILD_DIR/AITextEditor"
    if [ -f "$APP_PATH" ]; then
        echo -e "\nWould you like to run the application for manual testing? (y/n)"
        read -r RUN_APP
        if [[ "$RUN_APP" =~ ^[Yy]$ ]]; then
            echo -e "\n\033[33mLaunching AITextEditor for manual testing...\033[0m"
            echo -e "\033[36mTry the following multiple cursor operations:\033[0m"
            echo -e "\033[36m- Press Ctrl+Alt+Up/Down to add cursors above/below\033[0m"
            echo -e "\033[36m- Press Alt+Click to add cursors at specific positions\033[0m"
            echo -e "\033[36m- Select a word and press Ctrl+Alt+D to add cursors at all occurrences\033[0m"
            echo -e "\033[36m- Press Escape to return to single cursor mode\033[0m"
            echo -e "\n\033[90mPress Ctrl+C to exit the application when done testing.\033[0m\n"
            
            "$APP_PATH"
        fi
    else
        echo -e "\n\033[31mMain application executable not found at $APP_PATH\033[0m"
    fi
fi

echo -e "\n\033[36m=== Test script completed ===\033[0m" 