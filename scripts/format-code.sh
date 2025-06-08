#!/bin/bash

# Format all C++ files in the project using clang-format
# Usage: ./scripts/format-code.sh [directory]

set -e

# Default to current directory if no directory is specified
DIR=${1:-.}

# Find all C++ files and format them
find "$DIR" -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \) \
    -not -path "*/build/*" \
    -not -path "*/third_party/*" \
    -not -path "*/external/*" \
    -not -path "*/.git/*" \
    -not -path "*/.vscode/*" \
    -not -path "*/.idea/*" \
    -not -path "*/cmake-*" \
    -print0 | xargs -0 clang-format -i

echo "Code formatting complete!"
