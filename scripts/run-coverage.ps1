<#
.SYNOPSIS
    Generates code coverage reports for the project.
.DESCRIPTION
    This script builds the project with coverage instrumentation, runs the tests,
    and generates code coverage reports in various formats.
.PARAMETER BuildDir
    The build directory to use (default: 'build-coverage').
.PARAMETER Config
    The build configuration to use (default: 'Debug').
.PARAMETER OutputDir
    The output directory for coverage reports (default: 'coverage').
.PARAMETER Format
    The output format(s) for the coverage report (html, xml, cobertura, lcov, text).
.PARAMETER Open
    Open the HTML report after generation.
.PARAMETER Clean
    Clean the build directory before building.
.PARAMETER NoBuild
    Skip the build step (assumes the project is already built with coverage).
.PARAMETER NoTests
    Skip running tests (only generate reports from existing data).
.PARAMETER Threshold
    Set the coverage threshold (0-100). Fails if coverage is below this value.
.PARAMETER Exclude
    Exclude files/directories matching the given glob patterns.
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\run-coverage.ps1
    Generates an HTML coverage report in the default location.
.EXAMPLE
    .\scripts\run-coverage.ps1 -Format html,xml -Threshold 80
    Generates HTML and XML reports and fails if coverage is below 80%.
#>

[CmdletBinding()]
param(
    [string]$BuildDir = "build-coverage",
    [string]$Config = "Debug",
    [string]$OutputDir = "coverage",
    [string[]]$Format = @("html"),
    [switch]$Open,
    [switch]$Clean,
    [switch]$NoBuild,
    [switch]$NoTests,
    [int]$Threshold = 0,
    [string[]]$Exclude = @(),
    [switch]$Verbose
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
$outputPath = Join-Path $projectRoot $OutputDir

# Check for required tools
$missingTools = @()

# Check for gcovr (cross-platform coverage tool)
if (-not (Get-Command "gcovr" -ErrorAction SilentlyContinue)) {
    $missingTools += "gcovr (install with: pip install gcovr)"
}

# Check for OpenCppCoverage on Windows
if ($IsWindows -and -not (Get-Command "OpenCppCoverage" -ErrorAction SilentlyContinue)) {
    $missingTools += "OpenCppCoverage (install with: choco install opencppcoverage)"
}

if ($missingTools.Count -gt 0) {
    Write-Host "The following tools are required but not found:" -ForegroundColor $ErrorColor
    $missingTools | ForEach-Object { Write-Host "  - $_" -ForegroundColor $ErrorColor }
    Write-Host "`nPlease install the missing tools and try again." -ForegroundColor $WarningColor
    exit 1
}

# Function to run a command and check its exit code
function Invoke-CommandChecked {
    param(
        [string]$Command,
        [string[]]$Arguments,
        [string]$WorkingDirectory = $PWD.Path,
        [switch]$Fatal
    )
    
    Write-Host "$Command $($Arguments -join ' ')" -ForegroundColor $InfoColor
    
    $process = Start-Process -FilePath $Command -ArgumentList $Arguments -NoNewWindow -PassThru -WorkingDirectory $WorkingDirectory
    $process.WaitForExit()
    
    if ($process.ExitCode -ne 0 -and $Fatal) {
        Write-Host "Command failed with exit code $($process.ExitCode): $Command $($Arguments -join ' ')" -ForegroundColor $ErrorColor
        exit $process.ExitCode
    }
    
    return $process.ExitCode
}

# Clean build directory if requested
if ($Clean -and (Test-Path $buildPath)) {
    Write-Host "Cleaning build directory: $buildPath" -ForegroundColor $InfoColor
    Remove-Item -Path $buildPath -Recurse -Force
}

# Create build directory if it doesn't exist
if (-not (Test-Path $buildPath)) {
    New-Item -ItemType Directory -Path $buildPath | Out-Null
}

# Create output directory if it doesn't exist
if (-not (Test-Path $outputPath)) {
    New-Item -ItemType Directory -Path $outputPath | Out-Null
}

# Build the project with coverage if not skipped
if (-not $NoBuild) {
    Write-Host "Configuring project with coverage..." -ForegroundColor $InfoColor
    
    # Configure with coverage flags
    $cmakeArgs = @(
        "-B", "`"$buildPath`"",
        "-S", "`"$projectRoot`"",
        "-DCMAKE_BUILD_TYPE=$Config",
        "-DCMAKE_CXX_FLAGS=--coverage",
        "-DCMAKE_C_FLAGS=--coverage",
        "-DCMAKE_EXE_LINKER_FLAGS=--coverage"
    )
    
    $exitCode = Invoke-CommandChecked -Command "cmake" -Arguments $cmakeArgs -WorkingDirectory $projectRoot -Fatal
    
    if ($exitCode -ne 0) {
        exit $exitCode
    }
    
    # Build the project
    Write-Host "Building project with coverage..." -ForegroundColor $InfoColor
    $exitCode = Invoke-CommandChecked -Command "cmake" -Arguments @("--build", "`"$buildPath`"", "--config", $Config) -WorkingDirectory $buildPath -Fatal
    
    if ($exitCode -ne 0) {
        exit $exitCode
    }
}

# Run tests if not skipped
if (-not $NoTests) {
    Write-Host "Running tests with coverage..." -ForegroundColor $InfoColor
    
    # Change to build directory for test execution
    Push-Location $buildPath
    
    try {
        # Run tests with coverage
        if ($IsWindows) {
            # On Windows, use OpenCppCoverage
            $testExecutable = Join-Path $buildPath $Config "editor_tests.exe"
            
            if (-not (Test-Path $testExecutable)) {
                $testExecutable = Join-Path $buildPath "editor_tests.exe"
            }
            
            if (Test-Path $testExecutable) {
                $coverageDir = Join-Path $outputPath "opencppcoverage"
                New-Item -ItemType Directory -Path $coverageDir -Force | Out-Null
                
                $coverageArgs = @(
                    "--sources", "$projectRoot\\src",
                    "--export_type=cobertura:$coverageDir\\coverage.xml",
                    "--export_type=binary:$coverageDir\\coverage.coverage",
                    "--", "$testExecutable"
                )
                
                $exitCode = Invoke-CommandChecked -Command "OpenCppCoverage" -Arguments $coverageArgs -WorkingDirectory $buildPath -Fatal
                
                if ($exitCode -ne 0) {
                    exit $exitCode
                }
                
                # Convert to other formats if needed
                if ($Format -contains "html") {
                    $htmlDir = Join-Path $outputPath "html"
                    New-Item -ItemType Directory -Path $htmlDir -Force | Out-Null
                    
                    $convertArgs = @(
                        "--input_coverage=$coverageDir\\coverage.coverage",
                        "--export_type=html:$htmlDir"
                    )
                    
                    $exitCode = Invoke-CommandChecked -Command "OpenCppCoverage" -Arguments $convertArgs -WorkingDirectory $buildPath -Fatal
                    
                    if ($exitCode -eq 0 -and $Open) {
                        Start-Process (Join-Path $htmlDir "index.html")
                    }
                }
            } else {
                Write-Host "Test executable not found: $testExecutable" -ForegroundColor $ErrorColor
                exit 1
            }
        } else {
            # On Unix-like systems, use gcovr
            $testExecutable = Join-Path $buildPath "editor_tests"
            
            if (Test-Path $testExecutable) {
                # Run the tests to generate coverage data
                & $testExecutable
                
                # Generate reports
                $gcovrArgs = @(
                    "--root", "$projectRoot",
                    "--filter", "$projectRoot/src",
                    "--exclude-unreachable-branches",
                    "--exclude-throw-branches"
                )
                
                # Add exclude patterns
                foreach ($pattern in $Exclude) {
                    $gcovrArgs += "--exclude"
                    $gcovrArgs += $pattern
                }
                
                # Generate requested report formats
                foreach ($fmt in $Format) {
                    $outputFile = ""
                    
                    switch ($fmt.ToLower()) {
                        "html" {
                            $outputFile = Join-Path $outputPath "coverage.html"
                            $gcovrArgs += "--html"
                            $gcovrArgs += "--html-details"
                            $gcovrArgs += "-o"
                            $gcovrArgs += $outputFile
                        }
                        "xml" {
                            $outputFile = Join-Path $outputPath "coverage.xml"
                            $gcovrArgs += "--xml"
                            $gcovrArgs += "-o"
                            $gcovrArgs += $outputFile
                        }
                        "cobertura" {
                            $outputFile = Join-Path $outputPath "cobertura.xml"
                            $gcovrArgs += "--cobertura"
                            $gcovrArgs += $outputFile
                        }
                        "lcov" {
                            $outputFile = Join-Path $outputPath "lcov.info"
                            $gcovrArgs += "--lcov"
                            $gcovrArgs += $outputFile
                        }
                        "text" {
                            $outputFile = Join-Path $outputPath "coverage.txt"
                            $gcovrArgs += "--output"
                            $gcovrArgs += $outputFile
                        }
                        default {
                            Write-Host "Unknown format: $fmt. Skipping..." -ForegroundColor $WarningColor
                            continue
                        }
                    }
                    
                    Write-Host "Generating $fmt report: $outputFile" -ForegroundColor $InfoColor
                    $exitCode = Invoke-CommandChecked -Command "gcovr" -Arguments $gcovrArgs -WorkingDirectory $buildPath
                    
                    if ($exitCode -ne 0) {
                        Write-Host "Failed to generate $fmt report" -ForegroundColor $ErrorColor
                    } elseif ($fmt -eq "html" -and $Open) {
                        Start-Process $outputFile
                    }
                    
                    # Remove format-specific args for next iteration
                    $gcovrArgs = $gcovrArgs[0..($gcovrArgs.Count-3)]
                }
                
                # Check coverage threshold if specified
                if ($Threshold -gt 0) {
                    $gcovrThresholdArgs = $gcovrArgs + @("--fail-under-line=$Threshold")
                    $exitCode = Invoke-CommandChecked -Command "gcovr" -Arguments $gcovrThresholdArgs -WorkingDirectory $buildPath
                    
                    if ($exitCode -ne 0) {
                        Write-Host "Coverage is below threshold of ${Threshold}%" -ForegroundColor $ErrorColor
                        exit 1
                    }
                }
            } else {
                Write-Host "Test executable not found: $testExecutable" -ForegroundColor $ErrorColor
                exit 1
            }
        }
    } finally {
        Pop-Location
    }
}

Write-Host "`nCoverage reports generated in: $outputPath" -ForegroundColor $SuccessColor
