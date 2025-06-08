@echo off
echo Compiling actual TextBuffer test...

set SRC_DIR=..\src
set INCLUDE_DIR=..\include

# Compile with the actual TextBuffer implementation
g++ -std=c++17 -I%INCLUDE_DIR% -I. -o actual_textbuffer_test.exe test_actual_textbuffer.cpp %SRC_DIR%\TextBuffer.cpp

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    exit /b 1
)

echo.
echo =======================================
echo Running actual TextBuffer tests...
echo =======================================
.\actual_textbuffer_test.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo All tests passed successfully!
    echo You can find the test executable at: %CD%\actual_textbuffer_test.exe
) else (
    echo.
    echo Some tests failed with error code %ERRORLEVEL%
)

pause
