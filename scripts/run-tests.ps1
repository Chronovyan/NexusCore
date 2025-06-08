<#
.SYNOPSIS
    Runs the test suite for the project.
.DESCRIPTION
    This script builds and runs the test suite using CTest. It supports
    running specific tests, filtering tests, and generating various reports.
.PARAMETER BuildDir
    The build directory to use (default: 'build-debug').
.PARAMETER Config
    The build configuration to test (default: 'Debug').
.PARAMETER TestName
    Run only tests matching this name (supports regular expressions).
.PARAMETER Exclude
    Exclude tests matching this pattern (supports regular expressions).
.PARAMETER OutputOnFailure
    Print output from failed tests immediately.
.PARAMETER Parallel
    Run tests in parallel using the specified number of jobs.
.PARAMETER Repeat
    Run tests multiple times (useful for flaky tests).
.PARAMETER Timeout
    Set a timeout for each test in seconds.
.PARAMETER NoRebuild
    Don't rebuild the tests before running.
.PARAMETER Coverage
    Generate code coverage report (requires gcov/lcov or OpenCppCoverage).
.PARAMETER XmlOutput
    Generate test results in JUnit XML format.
.PARAMETER ListTests
    List available tests without running them.
.EXAMPLE
    .\scripts\run-tests.ps1
    Runs all tests in debug mode.
.EXAMPLE
    .\scripts\run-tests.ps1 -TestName "Editor" -OutputOnFailure -Parallel 4
    Runs tests containing "Editor" in their name with parallel execution.
#>

[CmdletBinding()]
param(
    [string]$BuildDir = "build-debug",
    [string]$Config = "Debug",
    [string]$TestName,
    [string]$Exclude,
    [switch]$OutputOnFailure,
    [int]$Parallel = 0,
    [int]$Repeat = 1,
    [int]$Timeout = 0,
    [switch]$NoRebuild,
    [switch]$Coverage,
    [string]$XmlOutput,
    [switch]$ListTests
)

# Error handling
$ErrorActionPreference = "Stop"

# Colors for output
$ErrorColor = "Red"
$WarningColor = "Yellow"
$SuccessColor = "Green"
$InfoColor = "Cyan"

# Get the project root directory
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $projectRoot $BuildDir

# Check if build directory exists
if (-not (Test-Path $buildPath)) {
    Write-Host "Build directory not found: $buildPath" -ForegroundColor $ErrorColor
    Write-Host "Please build the project first using .\scripts\build-debug.ps1 or .\scripts\build-release.ps1" -ForegroundColor $InfoColor
    exit 1
}

# Change to build directory
Push-Location $buildPath

try {
    # Build the tests if needed
    if (-not $NoRebuild) {
        Write-Host "Building tests..." -ForegroundColor $InfoColor
        & cmake --build . --config $Config --target editor_tests
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor $ErrorColor
            exit $LASTEXITCODE
        }
    }

    # Prepare CTest arguments
    $ctestArgs = @("--output-on-failure", "--build-config", $Config)
    
    # Add test filter if specified
    if (-not [string]::IsNullOrEmpty($TestName)) {
        $ctestArgs += "--tests-regex", $TestName
    }
    
    # Add exclude filter if specified
    if (-not [string]::IsNullOrEmpty($Exclude)) {
        $ctestArgs += "--exclude-regex", $Exclude
    }
    
    # Add parallel execution if specified
    if ($Parallel -gt 0) {
        $ctestArgs += "--parallel", $Parallel.ToString()
    }
    
    # Add repeat if specified
    if ($Repeat -gt 1) {
        $ctestArgs += "--repeat", "until-fail:$Repeat"
    }
    
    # Add timeout if specified
    if ($Timeout -gt 0) {
        $ctestArgs += "--timeout", "$Timeout"
    }
    
    # Add XML output if specified
    if (-not [string]::IsNullOrEmpty($XmlOutput)) {
        $ctestArgs += "--output-junit", $XmlOutput
    }
    
    # Just list tests if requested
    if ($ListTests) {
        $ctestArgs += "--show-only"
    }
    
    # Run CTest
    Write-Host "Running tests..." -ForegroundColor $InfoColor
    & ctest @ctestArgs
    $testExitCode = $LASTEXITCODE
    
    # Generate coverage report if requested
    if ($Coverage -and $testExitCode -eq 0) {
        Write-Host "Generating coverage report..." -ForegroundColor $InfoColor
        
        # Check for coverage tools
        if (Test-CommandExists gcovr) {
            # Using gcovr for coverage
            $coverageDir = Join-Path $projectRoot "coverage"
            New-Item -ItemType Directory -Force -Path $coverageDir | Out-Null
            $coverageReport = Join-Path $coverageDir "coverage.xml"
            
            & gcovr -r $projectRoot --xml-pretty --exclude "${buildPath}/.*" --exclude "${projectRoot}/tests/.*" --output $coverageReport
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "Coverage report generated: $coverageReport" -ForegroundColor $SuccessColor
            } else {
                Write-Host "Failed to generate coverage report with gcovr" -ForegroundColor $WarningColor
            }
        } elseif (Test-CommandExists OpenCppCoverage) {
            # Using OpenCppCoverage on Windows
            $coverageDir = Join-Path $projectPath "coverage"
            New-Item -ItemType Directory -Force -Path $coverageDir | Out-Null
            
            $testExecutable = Join-Path $buildPath $Config "editor_tests.exe"
            if (Test-Path $testExecutable) {
                & OpenCppCoverage --sources "$projectRoot/src" -- "$testExecutable" --output-dir "$coverageDir"
                
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "Coverage report generated in: $coverageDir" -ForegroundColor $SuccessColor
                } else {
                    Write-Host "Failed to generate coverage report with OpenCppCoverage" -ForegroundColor $WarningColor
                }
            } else {
                Write-Host "Test executable not found: $testExecutable" -ForegroundColor $WarningColor
            }
        } else {
            Write-Host "No coverage tool found. Install gcovr or OpenCppCoverage to generate coverage reports." -ForegroundColor $WarningColor
        }
    }
    
    exit $testExitCode
} finally {
    # Restore the original directory
    Pop-Location
}
