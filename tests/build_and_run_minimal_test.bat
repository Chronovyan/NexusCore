@echo off
setlocal

:: Set paths
set BUILD_DIR=build_minimal
set SOURCE_DIR=.

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Configure with CMake
echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -S "%SOURCE_DIR%" -B "%BUILD_DIR%"
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

:: Build the test
echo Building test...
cmake --build "%BUILD_DIR%" --config Debug --target MinimalTest
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
)

:: Run the test
echo Running test...
"%BUILD_DIR%\Debug\MinimalTest.exe"
if %ERRORLEVEL% neq 0 (
    echo Test failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo Test completed successfully.
endlocal
