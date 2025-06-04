param (
    [Parameter(Mandatory=$false)]
    [string]$testFilter = "ExtremeLargeFileTest.*",
    
    [Parameter(Mandatory=$false)]
    [int]$timeoutSeconds = 180,
    
    [Parameter(Mandatory=$false)]
    [switch]$skipProblematicTests = $false,
    
    [Parameter(Mandatory=$false)]
    [int]$jobs = 8
)

# Ensure the build directory exists
$buildDir = "build_focused"
if (!(Test-Path $buildDir)) {
    Write-Host "Building project first..." -ForegroundColor Yellow
    .\scripts\focused_debug.ps1 -jobs $jobs
}

# Set up the test executable path
$testExecutable = ".\$buildDir\tests\Debug\ExtremeLargeFileTest.exe"

# Check if the test executable exists
if (!(Test-Path $testExecutable)) {
    Write-Host "Test executable not found. Building ExtremeLargeFileTest..." -ForegroundColor Yellow
    .\scripts\focused_debug.ps1 -jobs $jobs -target ExtremeLargeFileTest
}

# Build the command line arguments
$commandArgs = "--gtest_filter=$testFilter"

# Add flag to skip problematic tests if requested
if ($skipProblematicTests) {
    $env:SKIP_PROBLEMATIC_TESTS = "1"
    Write-Host "Problematic tests will be skipped" -ForegroundColor Yellow
} else {
    $env:SKIP_PROBLEMATIC_TESTS = "0"
}

# Enable extreme tests
$env:ENABLE_EXTREME_TESTS = "1"
Write-Host "Running extreme large file tests with timeout of $timeoutSeconds seconds" -ForegroundColor Cyan
Write-Host "Test filter: $testFilter" -ForegroundColor Cyan

# Create a job to run the test with a timeout
$job = Start-Job -ScriptBlock {
    param($executable, $args)
    # Set the environment variables in the job context
    $env:ENABLE_EXTREME_TESTS = "1"
    $env:SKIP_PROBLEMATIC_TESTS = $using:env:SKIP_PROBLEMATIC_TESTS
    
    # Run the test and capture output
    & $executable $args
    
    # Return the exit code
    return $LASTEXITCODE
} -ArgumentList $testExecutable, $commandArgs

# Wait for the job to complete or timeout
$completed = $job | Wait-Job -Timeout $timeoutSeconds

# Check if the job completed or timed out
if ($null -eq $completed) {
    Write-Host "Test execution timed out after $timeoutSeconds seconds!" -ForegroundColor Red
    Write-Host "Forcibly stopping the test process..." -ForegroundColor Red
    
    # Find and kill the test process
    $processName = (Get-Item $testExecutable).BaseName
    $processes = Get-Process | Where-Object { $_.Name -eq $processName }
    
    if ($processes.Count -gt 0) {
        $processes | ForEach-Object { 
            Write-Host "Killing process: $($_.Id)" -ForegroundColor Red
            $_ | Stop-Process -Force 
        }
    }
    
    # Clean up the job
    $job | Remove-Job -Force
    
    exit 1
} else {
    # Get the job output
    $output = $job | Receive-Job
    Write-Host $output
    
    # Get the exit code
    $exitCode = $job.ChildJobs[0].Output[-1]
    $job | Remove-Job
    
    if ($exitCode -eq 0) {
        Write-Host "Tests completed successfully!" -ForegroundColor Green
    } else {
        Write-Host "Tests failed with exit code $exitCode" -ForegroundColor Red
    }
    
    exit $exitCode
} 