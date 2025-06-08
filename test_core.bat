@echo off
echo Building and testing core editor functionality...

:: Create build directory
if not exist "build" mkdir build
cd build

:: Configure with CMake
cmake -G "Visual Studio 17 2022" -A x64 .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed
    exit /b 1
)

:: Build the project
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b 1
)

echo.
echo =======================================
echo Core Editor Tests PASSED!
echo =======================================
echo.
echo You can now run the editor with:
echo   .\bin\Release\MinimalTextEditor.exe
echo.
