@echo off
setlocal enabledelayedexpansion

REM Check if build config is provided
if "%~1"=="" (
    echo Usage: run_performance_tests.bat [Debug^|Release]
    exit /b 1
)

set BUILD_CONFIG=%~1
if not "%BUILD_CONFIG%"=="Debug" if not "%BUILD_CONFIG%"=="Release" (
    echo Invalid build config. Use Debug or Release.
    exit /b 1
)

REM Set paths
set TEST_EXE=..\build\%BUILD_CONFIG%\LargeFilePerformanceTest.exe
set RAW_OUTPUT=performance_baseline_raw.txt
set CSV_OUTPUT=large_file_baselines.csv

REM Check if test executable exists
if not exist "%TEST_EXE%" (
    echo Error: %TEST_EXE% not found
    echo Please build the project first
    exit /b 1
)

REM Run the performance test
echo Running performance tests in %BUILD_CONFIG% mode...
"%TEST_EXE%" > "%RAW_OUTPUT%"

REM Update system details in CSV
echo Updating system details in CSV...
powershell -Command "(Get-Content '%CSV_OUTPUT%') -replace 'TBD,TBD,TBD', ('%COMMIT_HASH%,' + 'Intel Core i7-3630QM @ 2.40GHz' + ',' + '16GB') | Set-Content '%CSV_OUTPUT%'"

REM Extract metrics
echo Extracting performance metrics...
python extract_performance_metrics.py "%RAW_OUTPUT%" "%BUILD_CONFIG%"

REM Compare with previous baseline
echo Comparing with previous baseline...
python compare_baselines.py "%CSV_OUTPUT%"

REM Generate trend visualizations
echo Generating trend visualizations...
python visualize_trends.py "%CSV_OUTPUT%"

echo Performance testing completed successfully
echo Results written to:
echo - %RAW_OUTPUT%
echo - %CSV_OUTPUT%
echo - docs/performance_comparisons.md
echo - benchmarks/performance_trends_*.png 