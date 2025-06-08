#!/bin/bash

# Create git hooks directory if it doesn't exist
mkdir -p .git/hooks

# Create pre-commit hook
cat > .git/hooks/pre-commit << 'EOL'
#!/bin/bash

# Check for unstaged changes
if ! git diff --exit-code --quiet; then
    echo "Error: You have unstaged changes. Please commit or stash them first."
    exit 1
fi

# Run clang-format on staged files
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACMR "*.h" "*.cpp")

if [ -n "$STAGED_FILES" ]; then
    echo "Formatting code..."
    echo "$STAGED_FILES" | xargs clang-format -i
    
    # Add the formatted files back to the staging area
    echo "$STAGED_FILES" | xargs git add
    
    # Check if there are any changes after formatting
    if ! git diff --exit-code --quiet; then
        echo "Code was reformatted and automatically staged."
        echo "Please review the changes and run 'git commit' again."
        exit 1
    fi
fi

# Run clang-tidy
if [ -f "CMakeLists.txt" ]; then
    echo "Running static analysis..."
    mkdir -p build
    cd build
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
    run-clang-tidy -p . -header-filter='(src|include)/.*\.(h|hpp|cpp)$'
    if [ $? -ne 0 ]; then
        echo "Static analysis found issues. Please fix them before committing."
        exit 1
    fi
    cd ..
fi

# Run tests if they exist
if [ -f "CTestTestfile.cmake" ]; then
    echo "Running tests..."
    cd build
    ctest --output-on-failure
    if [ $? -ne 0 ]; then
        echo "Tests failed. Please fix them before committing."
        exit 1
    fi
    cd ..
fi

exit 0
EOL

# Make the pre-commit hook executable
chmod +x .git/hooks/pre-commit

echo "Git pre-commit hook has been installed successfully."
echo "It will now automatically format your code and run checks before each commit."
