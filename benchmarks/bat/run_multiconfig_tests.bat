@echo off
setlocal enabledelayedexpansion

echo Running performance tests in both Debug and Release modes...

REM Run Debug tests
echo.
echo ===== Running Debug Tests =====
call run_performance_tests.bat Debug
if errorlevel 1 (
    echo Debug tests failed
    exit /b 1
)

REM Run Release tests
echo.
echo ===== Running Release Tests =====
call run_performance_tests.bat Release
if errorlevel 1 (
    echo Release tests failed
    exit /b 1
)

echo.
echo All performance tests completed successfully
echo Results are available in:
echo - benchmarks/large_file_baselines.csv
echo - docs/performance_comparisons.md
echo - benchmarks/performance_trends_*.png 