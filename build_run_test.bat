@echo off
setlocal

if "%1"=="" (
    echo ERROR: No test name provided.
    echo Usage: %~n0 TestName [--tsan] [--asan] [--no-parallel]
    goto :eof
)
set "TEST_NAME=%1"
shift

echo Building and Running Standalone Test: %TEST_NAME%

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

REM Parse command line arguments (after TEST_NAME is shifted)
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
echo Unrecognized argument: %1
shift
goto parse_args
:end_parse_args

REM Set up compiler flags - /I. for src headers, /Itests for test utility headers
set COMMON_FLAGS=/std:c++17 /EHsc /W4 /Zi /I. /Itests

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

REM Define common source files (adjust if some tests need a different set)
set COMMON_SRC_FILES=src\Editor.cpp src\TextBuffer.cpp src\SyntaxHighlighter.cpp src\SyntaxHighlightingManager.cpp
set TEST_SRC_FILE=tests\standalone\%TEST_NAME%.cpp
set OUTPUT_EXE_FILE=bin\%TEST_NAME%.exe

REM Check if test source file exists
if not exist "%TEST_SRC_FILE%" (
    echo ERROR: Test source file not found: %TEST_SRC_FILE%
    goto :eof
)

REM Compile the test
echo Compiling %TEST_SRC_FILE% with flags: %COMMON_FLAGS% %OPT_FLAGS%
cl.exe %COMMON_FLAGS% %OPT_FLAGS% ^
    /Fe%OUTPUT_EXE_FILE% %TEST_SRC_FILE% ^
    %COMMON_SRC_FILES%

if errorlevel 1 (
    echo Compilation of %TEST_NAME% failed.
    exit /b 1
)

REM Clean up temporary object files from the root (if any)
if exist %TEST_NAME%.obj del %TEST_NAME%.obj 
REM Object files are directed to bin\ with /Fo"bin\" for the main test file, 
REM but common_src_files might create .obj in current dir if not careful.
REM The /Fo"bin\" applies to the output name from /Fe, not necessarily all intermediate .obj files.
REM A cleaner way is to specify /Fo for each src file or use a make system.
REM For now, let's also clean obj files from where common sources are.
if exist src\%TEST_NAME%.obj del src\%TEST_NAME%.obj 2>nul
if exist *.obj del *.obj 2>nul


echo.
echo Compilation of %TEST_NAME% successful.
echo Running %TEST_NAME%...
echo.

REM Set environment variables for sanitizers
if %ENABLE_TSAN%==1 (
    set TSAN_OPTIONS=second_deadlock_stack=1
)
if %ENABLE_ASAN%==1 (
    set ASAN_OPTIONS=detect_leaks=1:symbolize=1
)

REM Run the test
%OUTPUT_EXE_FILE% > test_output.txt 2>&1
set TEST_EXIT_CODE=%ERRORLEVEL%

if %TEST_EXIT_CODE% equ 0 (
    echo.
    echo Test %TEST_NAME% PASSED.
) else (
    echo.
    echo Test %TEST_NAME% FAILED with exit code %TEST_EXIT_CODE%.
)

endlocal
exit /b %TEST_EXIT_CODE%

:eof
endlocal 