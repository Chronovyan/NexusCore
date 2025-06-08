@echo off
echo Building and running TextBuffer tests...

:: Change to build directory
cd build

:: Configure with CMake
cmake -G "MinGW Makefiles" .. -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed
    cd ..
    exit /b 1
)

:: Build the tests
cmake --build .
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    cd ..
    exit /b 1
)

:: Run the tests
echo.
echo =======================================
echo Running TextBuffer tests...
echo =======================================
.\TextBufferTest.exe

cd ..
