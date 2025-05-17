@echo off
setlocal

echo Testing libcurl installation...

REM Create the curl_test_cmake directory if it doesn't exist
if not exist curl_test_cmake mkdir curl_test_cmake
cd curl_test_cmake

echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 .

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    cd ..
    exit /b %ERRORLEVEL%
)

echo Building test executable...
cmake --build . --config Debug

if %ERRORLEVEL% neq 0 (
    echo Build failed.
    cd ..
    exit /b %ERRORLEVEL%
)

echo Running curl test...
Debug\curl_test.exe

cd ..

echo Done. 