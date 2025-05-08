@echo off
setlocal

echo Building Performance Benchmark...

REM --- Visual Studio detection similar to build.bat ---
set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
set "VCVARS_SCRIPT=%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"

set COMPILER_FOUND=0

if "%1"=="gcc" (
    goto use_gcc
)

echo Checking for Visual Studio...

if EXIST "%VCVARS_SCRIPT%" (
    echo Found Visual Studio at %VS_ROOT%
    call "%VCVARS_SCRIPT%"
    set COMPILER_FOUND=1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2019 Community
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    set COMPILER_FOUND=1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2019 Enterprise
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
    set COMPILER_FOUND=1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2019 Professional
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"
    set COMPILER_FOUND=1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2017 Community
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
    set COMPILER_FOUND=1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio 2022 Community
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    set COMPILER_FOUND=1
)

if "%COMPILER_FOUND%"=="1" (
    echo Compiling with MSVC...
    cl.exe /EHsc /std:c++17 /O2 /Fe:perf_benchmark.exe tests/PerformanceBenchmark.cpp src/Editor.cpp src/TextBuffer.cpp src/SyntaxHighlighter.cpp /I.
    
    if %ERRORLEVEL% NEQ 0 (
        echo MSVC compilation failed, trying g++ instead...
        goto use_gcc
    )
) else (
    echo Visual Studio not found, trying g++ instead...
    goto use_gcc
)

goto run_benchmark

:use_gcc
echo Trying to compile with g++...
where g++ > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo g++ not found either. Cannot build benchmark.
    exit /b 1
)

g++ -std=c++17 -Wall -Wextra -O2 -o perf_benchmark.exe tests/PerformanceBenchmark.cpp src/Editor.cpp src/TextBuffer.cpp src/SyntaxHighlighter.cpp

if %ERRORLEVEL% NEQ 0 (
    echo Build failed with g++ as well.
    exit /b 1
)

:run_benchmark
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
if /i "%1"=="gcc" goto shift_args
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
echo Benchmark Configuration:
echo   Lines: %LINES%
echo   Characters per line: %CHARS%
echo   Iterations: %ITERATIONS%
echo   Run search benchmark: %RUN_SEARCH%
echo   Run syntax benchmark: %RUN_SYNTAX%
echo   Custom tests: %CUSTOM_TESTS%

rem Run the benchmark with the parsed parameters
perf_benchmark.exe %LINES% %CHARS% %ITERATIONS% %RUN_SEARCH% %RUN_SYNTAX% %CUSTOM_TESTS%

echo Done.
endlocal 