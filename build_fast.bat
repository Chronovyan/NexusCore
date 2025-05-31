@echo off
REM Wrapper script for fast_build.ps1 PowerShell script
REM This allows users to double-click to run or use from command prompt without PowerShell syntax

echo Running optimized build script...

REM Parse command line arguments
set ARGS=
set WITH_TESTS=
set CLEAN=
set RELEASE=
set JOBS=

:parse_args
if "%~1"=="" goto end_parse_args
if /i "%~1"=="-withTests" set WITH_TESTS=-withTests
if /i "%~1"=="-clean" set CLEAN=-clean
if /i "%~1"=="-release" set RELEASE=-release
if /i "%~1"=="-jobs" set JOBS=-jobs %~2 & shift
shift
goto parse_args

:end_parse_args
set ARGS=%WITH_TESTS% %CLEAN% %RELEASE% %JOBS%

REM Execute the PowerShell script with the parsed arguments
powershell.exe -ExecutionPolicy Bypass -File scripts\fast_build.ps1 %ARGS%

if %ERRORLEVEL% NEQ 0 (
    echo Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo Build completed successfully.
echo.
echo For more build options, run:
echo   build_fast.bat -withTests    (to build with tests)
echo   build_fast.bat -clean        (to clean before building)
echo   build_fast.bat -release      (to build in release mode)
echo   build_fast.bat -jobs N       (to specify number of jobs)
echo.
echo See docs\BUILD_OPTIMIZATION.md for more information.

REM Uncomment to pause at the end (useful when double-clicking the batch file)
REM pause 