@echo off
echo Compiling real TextBuffer test...

set SRC_DIR=..\src
set INCLUDE_DIR=..\include

# Compile with the real TextBuffer implementation
g++ -std=c++17 -I. ^
    -o real_textbuffer_test.exe ^
    test_real_textbuffer.cpp ^
    TextBuffer_test.cpp

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    exit /b 1
)

echo.
echo =======================================
echo Running real TextBuffer tests...
echo =======================================
.\real_textbuffer_test.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo All tests passed successfully!
    echo You can find the test executable at: %CD%\real_textbuffer_test.exe
) else (
    echo.
    echo Some tests failed with error code %ERRORLEVEL%
)

pause
