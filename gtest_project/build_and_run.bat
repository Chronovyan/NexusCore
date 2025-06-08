@echo off
setlocal

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

:: Build the test
echo Building test...
cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
)

:: Run the test
echo Running test...
.\Debug\MinimalTest.exe
if %ERRORLEVEL% neq 0 (
    echo Test failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo Test completed successfully.
endlocal
