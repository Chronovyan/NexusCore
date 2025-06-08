@echo off
setlocal

:: Clean up
if exist build_minimal rmdir /s /q build_minimal

:: Create build directory
mkdir build_minimal
cd build_minimal

:: Remove any existing CMake cache
if exist CMakeCache.txt del /q CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

:: Configure with CMake using Ninja
echo Configuring CMake with Ninja...
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug ..\tests
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

:: Build the test
echo Building test...
cmake --build . --config Debug --target MinimalTest
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
