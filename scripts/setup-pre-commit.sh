#!/bin/bash

# Set up Git pre-commit hooks for the project
# This script installs a pre-commit hook that runs:
# 1. Code formatting with clang-format
# 2. Static analysis with clang-tidy
# 3. Running tests

set -e

# Check if we're in a Git repository
if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    echo "Error: Not a Git repository"
    exit 1
fi

# Create the hooks directory if it doesn't exist
GIT_HOOKS_DIR=".git/hooks"
if [ ! -d "$GIT_HOOKS_DIR" ]; then
    mkdir -p "$GIT_HOOKS_DIR"
fi

# Create the pre-commit hook
cat > "$GIT_HOOKS_DIR/pre-commit" << 'EOL'
#!/bin/bash

# Run code formatting
if ! ./scripts/check-format.sh; then
    echo "❌ Code formatting check failed. Please run './scripts/format-code.sh' and try again."
    exit 1
fi

# Run static analysis
if ! ./scripts/analyze-code.sh; then
    echo "❌ Static analysis found issues. Please fix them and try again."
    exit 1
fi

# Run tests
if ! ./scripts/run-tests.sh; then
    echo "❌ Tests failed. Please fix the failing tests and try again."
    exit 1
fi

echo "✅ All checks passed!"
exit 0
EOL

# Make the pre-commit hook executable
chmod +x "$GIT_HOOKS_DIR/pre-commit"

echo "✅ Pre-commit hook installed successfully!"
echo "The following checks will run before each commit:"
echo "1. Code formatting with clang-format"
echo "2. Static analysis with clang-tidy"
echo "3. Running tests"
