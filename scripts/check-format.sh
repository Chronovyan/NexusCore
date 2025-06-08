#!/bin/bash

# Check code formatting using clang-format
# Usage: ./scripts/check-format.sh [directory]

set -e

# Default to current directory if no directory is specified
DIR=${1:-.}

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    echo "Please install clang-format to check code formatting."
    exit 1
fi

# Find all C++ files and check their formatting
FILES=$(find "$DIR" -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \) \
    -not -path "*/build/*" \
    -not -path "*/third_party/*" \
    -not -path "*/external/*" \
    -not -path "*/.git/*" \
    -not -path "*/.vscode/*" \
    -not -path "*/.idea/*" \
    -not -path "*/cmake-*")

# Check formatting for each file
ERRORS=0
for file in $FILES; do
    echo "Checking formatting for $file"
    if ! clang-format --dry-run --Werror "$file" &> /dev/null; then
        echo "  ❌ Formatting issues found in $file"
        ERRORS=$((ERRORS + 1))
    fi
done

# Report results
if [ $ERRORS -gt 0 ]; then
    echo "\nFound $ERRORS files with formatting issues."
    echo "Please run './scripts/format-code.sh' to fix the formatting issues."
    exit 1
else
    echo "\n✅ All files are properly formatted!"
    exit 0
fi
