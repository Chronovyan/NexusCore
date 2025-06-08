@echo off
echo Running Dependency Injection Framework Tests...

cd %~dp0\..
if not exist "build" mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 
cmake --build . --config Debug --target di_tests

if exist "tests\Debug\di_tests.exe" (
    echo.
    echo ===== DI Tests Results =====
    tests\Debug\di_tests.exe
) else (
    echo.
    echo Build failed or tests executable not found.
    echo Please check the build log for errors.
)

echo.
echo Done. 