@echo off
setlocal enabledelayedexpansion

echo TextEditor Coverage Analysis
echo ===========================

:: Create build directory for coverage if it doesn't exist
if not exist coverage mkdir coverage
cd coverage

:: Check for compiler selection
where cl.exe >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using MSVC compiler with OpenCppCoverage...
    
    :: Check if OpenCppCoverage is installed
    where OpenCppCoverage.exe >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo Error: OpenCppCoverage not found. Please install it from:
        echo https://github.com/OpenCppCoverage/OpenCppCoverage/releases
        exit /B 1
    )
    
    :: Build tests for coverage using MSVC
    echo Building tests for coverage analysis...
    cl.exe /std:c++17 /EHsc /Zi /Od /MDd /Fe:CombinedTestRunner.exe ..\CombinedTestRunner.cpp ..\BasicEditorTests.cpp ..\AdvancedEditorTests.cpp ..\FutureFeatureTests.cpp ..\UndoRedoTest.cpp ..\..\src\Editor.cpp ..\..\src\TextBuffer.cpp /I..\..\src /I.. /D_DEBUG
    
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to build tests for coverage analysis.
        exit /B 1
    )
    
    :: Run the tests with coverage
    echo Running tests with coverage...
    OpenCppCoverage.exe --sources=..\..\src --export_type=html:report --cover_children CombinedTestRunner.exe
) else (
    echo Using GCC compiler with gcov/lcov...
    
    :: Check if gcov and lcov are installed
    where gcov.exe >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo Error: gcov not found. Please install it as part of the MinGW/GCC toolchain.
        exit /B 1
    )
    
    where lcov.exe >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo Error: lcov not found. Please install it from http://gnuwin32.sourceforge.net/packages/lcov.htm
        exit /B 1
    )
    
    :: Build tests for coverage using GCC
    echo Building tests for coverage analysis...
    g++ -std=c++17 -g -O0 -fprofile-arcs -ftest-coverage -o CombinedTestRunner.exe ..\CombinedTestRunner.cpp ..\BasicEditorTests.cpp ..\AdvancedEditorTests.cpp ..\FutureFeatureTests.cpp ..\UndoRedoTest.cpp ..\..\src\Editor.cpp ..\..\src\TextBuffer.cpp -I..\..\src -I..
    
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to build tests for coverage analysis.
        exit /B 1
    )
    
    :: Run the tests with coverage
    echo Running tests with coverage...
    .\CombinedTestRunner.exe
    
    :: Generate coverage report
    echo Generating coverage report...
    lcov --capture --directory . --output-file coverage.info
    genhtml coverage.info --output-directory report
)

cd ..
echo Coverage report generated in coverage\report directory.
echo Open coverage\report\index.html to view the report.

endlocal 