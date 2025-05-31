@echo off
setlocal enabledelayedexpansion

REM Check if build config is provided
if "%~1"=="" (
    echo Usage: run_extreme_large_tests.bat [Debug^|Release] [ultra]
    echo.
    echo Parameters:
    echo   Debug^|Release  - Build configuration to use
    echo   ultra          - Optional parameter to enable ultra large tests (500MB)
    exit /b 1
)

set BUILD_CONFIG=%~1
if not "%BUILD_CONFIG%"=="Debug" if not "%BUILD_CONFIG%"=="Release" (
    echo Invalid build config. Use Debug or Release.
    exit /b 1
)

REM Check for ultra large test flag
set ULTRA_LARGE_TESTS=
if /i "%~2"=="ultra" (
    set ULTRA_LARGE_TESTS=1
    echo Ultra large tests (500MB) enabled
)

REM Set paths
set TEST_EXE=..\build\%BUILD_CONFIG%\ExtremeLargeFileTest.exe
set RESULTS_DIR=test_results
set LOG_FILE=%RESULTS_DIR%\extreme_large_tests_%BUILD_CONFIG%.log
set CSV_FILE=%RESULTS_DIR%\extreme_large_performance.csv

REM Create results directory if it doesn't exist
if not exist "%RESULTS_DIR%" mkdir "%RESULTS_DIR%"

REM Check if test executable exists
if not exist "%TEST_EXE%" (
    echo Error: %TEST_EXE% not found
    echo Please build the project with:
    echo   cmake --build build --config %BUILD_CONFIG% --target ExtremeLargeFileTest
    exit /b 1
)

REM Run the tests
echo Running extreme large file tests in %BUILD_CONFIG% mode...
echo Test results will be written to: %LOG_FILE%

if defined ULTRA_LARGE_TESTS (
    echo Running with ULTRA_LARGE_TESTS=1
    set ULTRA_LARGE_TESTS=1
)

REM Run the test and capture output
"%TEST_EXE%" > "%LOG_FILE%" 2>&1

REM Check test result
if %ERRORLEVEL% neq 0 (
    echo Tests failed with error code: %ERRORLEVEL%
    echo See %LOG_FILE% for details
    exit /b %ERRORLEVEL%
)

echo.
echo Tests completed successfully
echo Results saved to: %LOG_FILE%

REM Extract key performance metrics to CSV
echo Extracting performance metrics to CSV...
echo Timestamp,FileSize,OpenTime,SaveTime,SearchTime,ScrollTime,InsertTime,MemoryRatio > "%CSV_FILE%"

REM Parse log file and extract metrics
powershell -Command "& {
    $log = Get-Content '%LOG_FILE%'
    $timestamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    
    $sizes = @('MediumLarge', 'VeryLarge', 'ExtremeLarge', 'UltraLarge')
    
    foreach ($size in $sizes) {
        $openTime = ($log | Select-String -Pattern '\[$size\] File open time: (\d+\.?\d*) ms' | ForEach-Object { $_.Matches.Groups[1].Value })
        $saveTime = ($log | Select-String -Pattern '\[$size\] File save time: (\d+\.?\d*) ms' | ForEach-Object { $_.Matches.Groups[1].Value })
        $searchTime = ($log | Select-String -Pattern '\[$size\] Average search time: (\d+\.?\d*) ms' | ForEach-Object { $_.Matches.Groups[1].Value })
        $scrollTime = ($log | Select-String -Pattern '\[$size\] Average page down time: (\d+\.?\d*) ms' | ForEach-Object { $_.Matches.Groups[1].Value })
        $insertTime = ($log | Select-String -Pattern '\[$size\] Text insertion time: (\d+\.?\d*) ms' | ForEach-Object { $_.Matches.Groups[1].Value })
        $memoryRatio = ($log | Select-String -Pattern '\[$size\] Memory/File ratio: (\d+\.?\d*)' | ForEach-Object { $_.Matches.Groups[1].Value })
        
        if ($openTime) {
            $fileSizeMB = switch ($size) {
                'MediumLarge' { '12' }
                'VeryLarge' { '50' }
                'ExtremeLarge' { '150' }
                'UltraLarge' { '500' }
                default { '0' }
            }
            
            Add-Content -Path '%CSV_FILE%' -Value `"$timestamp,$fileSizeMB,$openTime,$saveTime,$searchTime,$scrollTime,$insertTime,$memoryRatio`"
        }
    }
}"

echo Performance metrics saved to: %CSV_FILE%
echo.
echo To run tests with Ultra Large files (500MB), use:
echo   run_extreme_large_tests.bat %BUILD_CONFIG% ultra
echo.

endlocal 