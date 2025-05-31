@echo off
REM Wrapper script for focused_debug.ps1 PowerShell script
REM This allows users to double-click to run or use from command prompt

echo Running focused debug build script...

REM Parse command line arguments
set ARGS=
set CLEAN=
set TEST_FILTER=
set TARGET=
set JOBS=

:parse_args
if "%~1"=="" goto end_parse_args
if /i "%~1"=="-clean" set CLEAN=-clean
if /i "%~1"=="-testFilter" set TEST_FILTER=-testFilter %~2 & shift
if /i "%~1"=="-target" set TARGET=-target %~2 & shift
if /i "%~1"=="-jobs" set JOBS=-jobs %~2 & shift
shift
goto parse_args

:end_parse_args
set ARGS=%CLEAN% %TEST_FILTER% %TARGET% %JOBS%

REM Execute the PowerShell script with the parsed arguments
powershell.exe -ExecutionPolicy Bypass -File scripts\focused_debug.ps1 %ARGS%

if %ERRORLEVEL% NEQ 0 (
    echo Build or tests failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo For focused debugging options, run:
echo   focused_debug.bat -clean                (to clean before building)
echo   focused_debug.bat -target TARGET_NAME   (to build specific target)
echo   focused_debug.bat -testFilter TEST_NAME (to run specific tests)
echo   focused_debug.bat -jobs N               (to specify number of jobs)
echo.

REM Uncomment to pause at the end (useful when double-clicking the batch file)
REM pause 