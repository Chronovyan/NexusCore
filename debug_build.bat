@echo off
REM Wrapper script for debug_build.ps1 PowerShell script
REM This allows users to double-click to run or use from command prompt

echo Running optimized debug build script...

REM Parse command line arguments
set ARGS=
set CLEAN=
set TEST_FILTER=
set JOBS=

:parse_args
if "%~1"=="" goto end_parse_args
if /i "%~1"=="-clean" set CLEAN=-clean
if /i "%~1"=="-testFilter" set TEST_FILTER=-testFilter %~2 & shift
if /i "%~1"=="-jobs" set JOBS=-jobs %~2 & shift
shift
goto parse_args

:end_parse_args
set ARGS=%CLEAN% %TEST_FILTER% %JOBS%

REM Execute the PowerShell script with the parsed arguments
powershell.exe -ExecutionPolicy Bypass -File scripts\debug_build.ps1 %ARGS%

if %ERRORLEVEL% NEQ 0 (
    echo Build or tests failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo For debugging options, run:
echo   debug_build.bat -clean           (to clean before building)
echo   debug_build.bat -testFilter NAME (to run specific tests)
echo   debug_build.bat -jobs N          (to specify number of jobs)
echo.

REM Uncomment to pause at the end (useful when double-clicking the batch file)
REM pause 