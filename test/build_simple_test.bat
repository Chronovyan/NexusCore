@echo off
echo Compiling simple TextBuffer test...

g++ -std=c++17 -o simple_textbuffer_test.exe simple_textbuffer_test.cpp

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    exit /b 1
)

echo.
echo =======================================
echo Running simple TextBuffer tests...
echo =======================================
.\simple_textbuffer_test.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo All tests passed successfully!
    echo You can find the test executable at: %CD%\simple_textbuffer_test.exe
) else (
    echo.
    echo Some tests failed with error code %ERRORLEVEL%
)

pause
