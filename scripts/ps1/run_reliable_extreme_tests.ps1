# PowerShell script to run only the most reliable extreme tests
# This script provides a convenient way to run a curated subset of the extreme tests
# that are less likely to hang or cause issues.

param (
    [int]$timeoutSeconds = 120,
    [int]$jobs = 8
)

Write-Host "Running reliable subset of extreme large file tests..." -ForegroundColor Cyan

# Define the tests we consider reliable
$reliableTests = @(
    "ExtremeLargeFileTest.MediumFilePerformance"  # Medium file tests are more reliable
)

# Join the reliable tests with colons for gtest
$filterString = $reliableTests -join ":"

# Always enable the skip problematic tests flag for the reliable script
$skipFlag = $true

# Call the main extreme tests script with our parameters
$scriptPath = Join-Path $PSScriptRoot "run_extreme_tests.ps1"
& $scriptPath -testFilter $filterString -timeoutSeconds $timeoutSeconds -skipProblematicTests:$skipFlag -jobs $jobs

if ($LASTEXITCODE -eq 0) {
    Write-Host "All reliable extreme tests completed successfully!" -ForegroundColor Green
} else {
    Write-Host "Some reliable extreme tests failed with exit code: $LASTEXITCODE" -ForegroundColor Red
}

exit $LASTEXITCODE 