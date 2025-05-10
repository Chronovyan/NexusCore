@echo off
setlocal

echo Building and Running Unit Tests

REM Setup Visual Studio environment if needed
if not defined VSCMD_VER (
    echo Setting up Visual Studio environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if errorlevel 1 (
        echo Failed to setup Visual Studio environment.
        exit /b 1
    )
)

REM Create output directory if it doesn't exist
if not exist bin mkdir bin

REM Check for enabling sanitizers
set ENABLE_TSAN=0
set ENABLE_ASAN=0
set ENABLE_PARALLEL=1

REM Parse command line arguments
:parse_args
if "%1"=="" goto end_parse_args
if /I "%1"=="--tsan" (
    set ENABLE_TSAN=1
    echo Enabling Thread Sanitizer
    shift
    goto parse_args
)
if /I "%1"=="--asan" (
    set ENABLE_ASAN=1
    echo Enabling Address Sanitizer
    shift
    goto parse_args
)
if /I "%1"=="--no-parallel" (
    set ENABLE_PARALLEL=0
    echo Disabling parallel build
    shift
    goto parse_args
)
shift
goto parse_args
:end_parse_args

REM Set up compiler flags
set COMMON_FLAGS=/std:c++17 /EHsc /W4 /Zi /I. /Isrc /Itests

REM Add sanitizer options
if %ENABLE_TSAN%==1 (
    set COMMON_FLAGS=%COMMON_FLAGS% /fsanitize=thread
)
if %ENABLE_ASAN%==1 (
    set COMMON_FLAGS=%COMMON_FLAGS% /fsanitize=address
)

REM Set optimization level and parallel build flags
set OPT_FLAGS=/O2
if %ENABLE_PARALLEL%==1 (
    set OPT_FLAGS=%OPT_FLAGS% /MP
)

REM Define common source files
set SRC_FILES=src\Editor.cpp src\TextBuffer.cpp src\SyntaxHighlighter.cpp src\SyntaxHighlightingManager.cpp src\EditorCommands.cpp

REM Compile CommandLogicTests.cpp
echo Compiling tests/CommandLogicTests.cpp with flags: %COMMON_FLAGS% %OPT_FLAGS%
cl.exe %COMMON_FLAGS% %OPT_FLAGS% ^
    /Febin\CommandLogicTests.exe tests\CommandLogicTests.cpp %SRC_FILES%
if errorlevel 1 (
    echo Compilation of CommandLogicTests failed.
    exit /b 1
)
echo CommandLogicTests.cpp compiled successfully.
echo.

REM Compile CommandManagerTests.cpp
echo Compiling tests/CommandManagerTests.cpp with flags: %COMMON_FLAGS% %OPT_FLAGS%
cl.exe %COMMON_FLAGS% %OPT_FLAGS% ^
    /Febin\CommandManagerTests.exe tests\CommandManagerTests.cpp %SRC_FILES%
if errorlevel 1 (
    echo Compilation of CommandManagerTests failed.
    exit /b 1
)
echo CommandManagerTests.cpp compiled successfully.
echo.

REM Clean up temporary files
if exist *.obj del *.obj
if exist bin\*.obj del bin\*.obj

echo.
echo All unit test compilations successful.
echo Running unit tests...
echo.

REM Set environment variables for sanitizers
if %ENABLE_TSAN%==1 (
    set TSAN_OPTIONS=second_deadlock_stack=1
)
if %ENABLE_ASAN%==1 (
    set ASAN_OPTIONS=detect_leaks=1:symbolize=1
)

REM Run CommandLogicTests
echo --- Running CommandLogicTests ---
bin\CommandLogicTests.exe
if errorlevel 1 (
    echo.
    echo CommandLogicTests FAILED.
) else (
    echo.
    echo CommandLogicTests PASSED.
)
echo.

REM Run CommandManagerTests
echo --- Running CommandManagerTests ---
bin\CommandManagerTests.exe
if errorlevel 1 (
    echo.
    echo CommandManagerTests FAILED.
) else (
    echo.
    echo CommandManagerTests PASSED.
)
echo.

endlocal 