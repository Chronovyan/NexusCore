#!/bin/bash

set -e

echo "Running advanced pre-commit checks..."

failures=""

# 1. Check for modifications to critical API implementation files
critical_files=("OpenAI_API_Client.cpp" "OpenAI_API_Client.h" "IOpenAI_API_Client.h")
staged_files=$(git diff --cached --name-only)

for file in "${critical_files[@]}"; do
    if echo "$staged_files" | grep -q "^$file$"; then
        echo "Critical file staged: $file"
        # 2. Warn if "not implemented yet" is still present
        if [ -f "$file" ]; then
            if grep -q "not implemented yet" "$file"; then
                echo "WARNING: 'not implemented yet' found in $file. Please complete implementation before committing."
                failures="$failures\n'not implemented yet' in $file"
            fi
        fi
    fi
done

# 3. Check interface compatibility (basic signature check)
if [ -f "IOpenAI_API_Client.h" ] && [ -f "OpenAI_API_Client.h" ]; then
    interface=$(cat IOpenAI_API_Client.h)
    impl=$(cat OpenAI_API_Client.h)
    if ! echo "$impl" | grep -q "listModels" || ! echo "$impl" | grep -q "retrieveModel" || ! echo "$impl" | grep -q "createEmbedding"; then
        echo "WARNING: OpenAI_API_Client.h may not fully implement all methods from IOpenAI_API_Client.h"
        failures="$failures\nAPI client missing interface methods"
    fi
fi

# 4. Warn if test files were modified but no critical tests added
for t in $(echo "$staged_files" | grep "^tests/"); do
    if [ -f "$t" ]; then
        if ! grep -q "CRITICAL_TEST\|Critical" "$t"; then
            echo "WARNING: Test file $t was modified, but no critical tests detected. Consider tagging important tests as CRITICAL."
        fi
    fi
done

# 5. If any static analysis failed, block the commit before building
if [ -n "$failures" ]; then
    echo -e "Enhanced pre-commit checks failed:$failures"
    exit 1
fi

# 6. Build and run "Critical" tests
echo "Static checks passed. Building project and running critical tests..."

cmake -S . -B build -DCMAKE_CXX_COMPILER=g++
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configure failed."
    exit 1
fi
cmake --build build
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed."
    exit 1
fi

cd build
ctest -R Critical --output-on-failure
if [ $? -ne 0 ]; then
    echo "ERROR: Critical tests failed."
    exit 1
fi
cd ..

echo "All advanced pre-commit checks and critical tests passed. Commit allowed."
exit 0 