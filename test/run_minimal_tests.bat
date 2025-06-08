@echo off
echo Building and running MinimalTextBuffer tests...

:: Create build directory
if not exist "minimal_build" mkdir minimal_build
cd minimal_build

:: Configure with CMake
cmake -G "MinGW Makefiles" .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=mingw32-make.exe -DCMAKE_C_COMPILER=gcc.exe -DCMAKE_CXX_COMPILER=g++.exe
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
echo Running MinimalTextBuffer tests...
echo =======================================
.\MinimalTextBufferTest.exe

cd ..
