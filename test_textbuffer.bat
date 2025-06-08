@echo off
echo Building and running TextBuffer tests...

:: Create build directory
if not exist "test_build" mkdir test_build
cd test_build

:: Configure with CMake
cmake -G "MinGW Makefiles" .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=mingw32-make.exe -DCMAKE_C_COMPILER=gcc.exe -DCMAKE_CXX_COMPILER=g++.exe -DCMAKE_CXX_FLAGS="-I..\include -I..\src"
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
