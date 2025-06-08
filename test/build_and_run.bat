@echo off
echo Compiling minimal text buffer test...

g++ -std=c++17 -o minimal_test.exe test_minimal_textbuffer.cpp
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    exit /b 1
)

echo.
echo =======================================
echo Running minimal text buffer tests...
echo =======================================
.\minimal_test.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo All tests passed successfully!
    echo You can find the test executable at: %CD%\minimal_test.exe
) else (
    echo.
    echo Some tests failed with error code %ERRORLEVEL%
)

pause
