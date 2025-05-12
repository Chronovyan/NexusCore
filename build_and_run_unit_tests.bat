@echo off
REM Build and run tests for the TextEditor project
REM This script builds all test files and runs them individually or collectively

echo === Building Text Editor Tests ===

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring build with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Debug

REM Build tests
echo Building tests...
cmake --build . --config Debug

echo.
echo === Running Individual Tests ===

REM Run individual tests
echo.
echo === Running Simple Command Test ===
.\tests\SimpleCommandTest.exe
if %ERRORLEVEL% neq 0 (
    echo Simple Command Test FAILED with error code %ERRORLEVEL%
) else (
    echo Simple Command Test PASSED
)

echo.
echo === Running Command Manager Test ===
.\tests\CommandManagerTests.exe
if %ERRORLEVEL% neq 0 (
    echo Command Manager Test FAILED with error code %ERRORLEVEL%
) else (
    echo Command Manager Test PASSED
)

echo.
echo === Running Comprehensive Undo/Redo Test ===
.\tests\ComprehensiveUndoRedoTest.exe
if %ERRORLEVEL% neq 0 (
    echo Comprehensive Undo/Redo Test FAILED with error code %ERRORLEVEL%
) else (
    echo Comprehensive Undo/Redo Test PASSED
)

echo.
echo === Running Selection Clipboard Test ===
.\tests\SelectionClipboardTest.exe
if %ERRORLEVEL% neq 0 (
    echo Selection Clipboard Test FAILED with error code %ERRORLEVEL%
) else (
    echo Selection Clipboard Test PASSED
)

echo.
echo === Running Exit Test ===
.\tests\ExitTest.exe
if %ERRORLEVEL% neq 0 (
    echo Exit Test FAILED with error code %ERRORLEVEL%
) else (
    echo Exit Test PASSED
)

echo.
echo === Running All Tests Together ===
.\tests\RunAllTests.exe
if %ERRORLEVEL% neq 0 (
    echo All Tests FAILED with error code %ERRORLEVEL%
) else (
    echo All Tests PASSED
)

REM Return to original directory
cd ..

echo.
echo === Testing Complete === 