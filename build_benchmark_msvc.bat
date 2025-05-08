@echo off
setlocal

if /i "%1"=="help" goto :show_help

echo Building Performance Benchmark with MSVC only...

REM --- Visual Studio detection similar to build.bat ---
set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
set "VCVARS_SCRIPT=%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"

echo Checking for Visual Studio...

if EXIST "%VCVARS_SCRIPT%" (
    echo Found Visual Studio at %VS_ROOT%
    call "%VCVARS_SCRIPT%" > nul 2>&1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2019 Community
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" > nul 2>&1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2019 Enterprise
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat" > nul 2>&1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2019 Professional
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" > nul 2>&1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2017 Community
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" > nul 2>&1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2022 Community
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" > nul 2>&1
) else (
    echo Error: Could not find Visual Studio installation.
    echo Please install Visual Studio or check the paths in the script.
    exit /b 1
)

echo Visual Studio environment initialized.
echo.
echo Compiling with MSVC...
cl.exe /EHsc /std:c++17 /O2 /Fe:perf_benchmark.exe tests/PerformanceBenchmark.cpp src/Editor.cpp src/TextBuffer.cpp src/SyntaxHighlighter.cpp /I.

if %ERRORLEVEL% NEQ 0 (
    echo MSVC compilation failed with error code %ERRORLEVEL%
    exit /b 1
)

echo.
echo Compilation successful!
echo.
echo Running Performance Benchmark...

rem Default test configuration
set LINES=1000
set CHARS=80
set ITERATIONS=100
set RUN_SEARCH=
set RUN_SYNTAX=1
set CUSTOM_TESTS=

rem Parse command line arguments
:parse_args
if "%1"=="" goto end_parse_args
if /i "%1"=="search" (
    set RUN_SEARCH=search
    goto shift_args
)
if /i "%1"=="nosyntax" (
    set RUN_SYNTAX=0
    goto shift_args
)
if /i "%1"=="lines" (
    set LINES=%2
    shift
    goto shift_args
)
if /i "%1"=="chars" (
    set CHARS=%2
    shift
    goto shift_args
)
if /i "%1"=="iterations" (
    set ITERATIONS=%2
    shift
    goto shift_args
)
if /i "%1"=="cursor" (
    set CUSTOM_TESTS=%CUSTOM_TESTS% cursor
    goto shift_args
)
if /i "%1"=="edit" (
    set CUSTOM_TESTS=%CUSTOM_TESTS% edit
    goto shift_args
)
if /i "%1"=="undoredo" (
    set CUSTOM_TESTS=%CUSTOM_TESTS% undoredo
    goto shift_args
)
if /i "%1"=="all" (
    set CUSTOM_TESTS=all
    goto shift_args
)

:shift_args
shift
goto parse_args

:end_parse_args

rem Display configuration
echo.
echo Benchmark Configuration:
echo   Lines: %LINES%
echo   Characters per line: %CHARS%
echo   Iterations: %ITERATIONS%
echo   Run search benchmark: %RUN_SEARCH%
echo   Run syntax benchmark: %RUN_SYNTAX%
echo   Custom tests: %CUSTOM_TESTS%
echo.

rem Run the benchmark with the parsed parameters
perf_benchmark.exe %LINES% %CHARS% %ITERATIONS% %RUN_SEARCH% %RUN_SYNTAX% %CUSTOM_TESTS%

echo.
echo Benchmark completed.
echo Done.
goto :eof

:show_help
echo.
echo Text Editor Performance Benchmark Helper
echo =======================================
echo.
echo Usage: build_benchmark_msvc.bat [options]
echo.
echo Options:
echo   help        - Display this help message
echo   search      - Enable search benchmark (disabled by default)
echo   nosyntax    - Disable syntax highlighting benchmark
echo   lines N     - Set number of lines in test file (default: 1000)
echo   chars N     - Set avg characters per line (default: 80)
echo   iterations N - Set iterations for benchmarks (default: 100)
echo.
echo Test Selection:
echo   cursor      - Run only cursor operations benchmark
echo   edit        - Run only editing operations benchmark
echo   undoredo    - Run only undo/redo operations benchmark
echo   all         - Run all benchmarks (default)
echo.
echo Examples:
echo   build_benchmark_msvc.bat
echo   build_benchmark_msvc.bat search lines 500
echo   build_benchmark_msvc.bat cursor edit lines 2000 iterations 50
echo.
exit /b 0
endlocal 