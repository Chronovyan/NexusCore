@echo off
echo Compiling final simplified TextBuffer test...

g++ -std=c++17 -o simple_textbuffer_test.exe simple_textbuffer_test_final.cpp

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    exit /b 1
)

echo.
echo =======================================
echo Running simplified TextBuffer tests...
echo =======================================
.\simple_textbuffer_test.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo All tests passed successfully!
) else (
    echo.
    echo Some tests failed with error code %ERRORLEVEL%
)

pause
