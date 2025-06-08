@echo off
setlocal enabledelayedexpansion

:: Set up build directory
if not exist "build" mkdir build
cd build

:: Configure with CMake
echo Configuring build with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    exit /b %ERRORLEVEL%
)

:: Build the project
echo Building project...
cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

:: Run tests
echo Running tests...
cd tests/Debug
ctest --output-on-failure
if %ERRORLEVEL% neq 0 (
    echo Some tests failed
    exit /b %ERRORLEVEL%
)

echo All tests passed!

:: Run the demo
echo.
echo =========================================
echo To run the demo, execute:
echo   cd ..\..\bin\Debug

echo    ai_text_rpg.exe
echo =========================================

cd ..\..
