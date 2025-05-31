#!/bin/bash
# Shell script to run all automated tests

echo "===================================================="
echo "Automated Test Runner for AI-First TextEditor"
echo "===================================================="

# Determine script directory and set test directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
TESTS_DIR="${SCRIPT_DIR}/../build/tests"

if [ ! -d "$TESTS_DIR" ]; then
    echo "Error: Build directory not found at $TESTS_DIR"
    echo "Please build the project before running tests"
    exit 1
fi

echo
echo "Running Automated Tests:"
echo "===================================================="

# Track overall success
ALL_PASSED=true

# Run AutomatedUndoRedoTest
echo "Running AutomatedUndoRedoTest..."
"${TESTS_DIR}/AutomatedUndoRedoTest"
if [ $? -ne 0 ]; then
    echo "AutomatedUndoRedoTest FAILED with code $?"
    ALL_PASSED=false
else
    echo "AutomatedUndoRedoTest PASSED"
fi
echo

# Run AutomatedSearchTest
echo "Running AutomatedSearchTest..."
"${TESTS_DIR}/AutomatedSearchTest"
if [ $? -ne 0 ]; then
    echo "AutomatedSearchTest FAILED with code $?"
    ALL_PASSED=false
else
    echo "AutomatedSearchTest PASSED"
fi
echo

# Run AutomatedSyntaxHighlightingTest
echo "Running AutomatedSyntaxHighlightingTest..."
"${TESTS_DIR}/AutomatedSyntaxHighlightingTest"
if [ $? -ne 0 ]; then
    echo "AutomatedSyntaxHighlightingTest FAILED with code $?"
    ALL_PASSED=false
else
    echo "AutomatedSyntaxHighlightingTest PASSED"
fi
echo

# Run AutomatedConcurrencyTest
echo "Running AutomatedConcurrencyTest..."
"${TESTS_DIR}/AutomatedConcurrencyTest"
if [ $? -ne 0 ]; then
    echo "AutomatedConcurrencyTest FAILED with code $?"
    ALL_PASSED=false
else
    echo "AutomatedConcurrencyTest PASSED"
fi
echo

echo "===================================================="
echo "Automated Test Run Complete"
echo "===================================================="

# Return success only if all tests passed
if [ "$ALL_PASSED" = false ]; then
    echo "Some tests failed. Please check the output above."
    exit 1
else
    echo "All automated tests passed!"
    exit 0
fi 