@echo off
setlocal

:: Set paths
set BUILD_DIR=build_simple_test
set SOURCE_DIR=.

:: Clean up previous build
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)

:: Create build directory
mkdir "%BUILD_DIR%"

:: Configure with CMake
echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 -S "%SOURCE_DIR%" -B "%BUILD_DIR%"
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

:: Build the project
echo Building project...
cmake --build "%BUILD_DIR%" --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
)

:: Run the test
echo Running test...
"%BUILD_DIR%\Debug\SimpleEditorTest.exe"
if %ERRORLEVEL% neq 0 (
    echo Test failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo Test completed successfully.
endlocal
