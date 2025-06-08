<#
.SYNOPSIS
    Performs static code analysis on the project.
.DESCRIPTION
    This script runs various static code analysis tools on the project,
    including clang-tidy, cppcheck, and other linters.
.PARAMETER BuildDir
    The build directory to use (default: 'build-debug').
.PARAMETER Config
    The build configuration to analyze (default: 'Debug').
.PARAMETER Fix
    Automatically fix issues where possible.
.PARAMETER Checks
    Comma-separated list of checks to enable (clang-tidy format).
.PARAMETER Exclude
    Exclude files/directories matching the given glob patterns.
.PARAMETER Threads
    Number of threads to use for analysis.
.PARAMETER ExportFixes
    Export fixes to the specified file.
.PARAMETER Format
    Output format (text, json, yaml, etc.).
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\analyze-code.ps1
    Runs static analysis with default settings.
.EXAMPLE
    .\scripts\analyze-code.ps1 -Checks "*" -Fix
    Runs all available checks and applies automatic fixes.
#>

[CmdletBinding()]
param(
    [string]$BuildDir = "build-debug",
    [string]$Config = "Debug",
    [switch]$Fix,
    [string]$Checks = "*",
    [string[]]$Exclude = @(),
    [int]$Threads = 0,
    [string]$ExportFixes,
    [string]$Format = "text",
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

# Check if build directory exists
if (-not (Test-Path $buildPath)) {
    Write-Host "Build directory not found: $buildPath" -ForegroundColor $ErrorColor
    Write-Host "Please build the project first using .\scripts\build-debug.ps1" -ForegroundColor $InfoColor
    exit 1
}

# Check for required tools
$missingTools = @()

if (-not (Get-Command "clang-tidy" -ErrorAction SilentlyContinue)) {
    $missingTools += "clang-tidy (install with: choco install llvm)"
}

if (-not (Get-Command "cppcheck" -ErrorAction SilentlyContinue)) {
    $missingTools += "cppcheck (install with: choco install cppcheck)"
}

if ($missingTools.Count -gt 0) {
    Write-Host "The following tools are required but not found:" -ForegroundColor $ErrorColor
    $missingTools | ForEach-Object { Write-Host "  - $_" -ForegroundColor $ErrorColor }
    Write-Host "`nPlease install the missing tools and try again." -ForegroundColor $WarningColor
    exit 1
}

# Function to check if a file should be excluded
function Test-ExcludedFile {
    param([string]$filePath)
    
    $relativePath = $filePath.Substring($projectRoot.Length).TrimStart('\', '/')
    
    foreach ($pattern in $Exclude) {
        if ([System.IO.Path]::GetFullPath($filePath) -like "$projectRoot\$pattern" -or 
            [System.IO.Path]::GetFullPath($filePath) -like "$projectRoot\$pattern".Replace('/', '\')) {
            return $true
        }
    }
    
    return $false
}

# Find all source files
$sourceFiles = Get-ChildItem -Path $projectRoot -Recurse -Include *.h,*.hpp,*.cpp,*.c,*.cc,*.cxx | 
    Where-Object { -not (Test-ExcludedFile $_.FullName) }

if ($sourceFiles.Count -eq 0) {
    Write-Host "No source files found to analyze." -ForegroundColor $WarningColor
    exit 0
}

# Prepare clang-tidy arguments
$tidyArgs = @(
    "--quiet",
    "--header-filter=.*",
    "--checks=$Checks",
    "--format-style=file"
)

if ($Fix) {
    $tidyArgs += "--fix"
}

if (-not [string]::IsNullOrEmpty($ExportFixes)) {
    $tidyArgs += "--export-fixes=$ExportFixes"
}

if ($Threads -gt 0) {
    $tidyArgs += "-j=$Threads"
}

# Add compile commands if available
$compileCommands = Join-Path $buildPath "compile_commands.json"
if (Test-Path $compileCommands) {
    $tidyArgs += "-p=$buildPath"
} else {
    Write-Host "Warning: compile_commands.json not found. Analysis may be less accurate." -ForegroundColor $WarningColor
}

# Run clang-tidy
$tidyIssues = @()
$tidyOutput = @()

Write-Host "Running clang-tidy..." -ForegroundColor $InfoColor
foreach ($file in $sourceFiles) {
    $relativePath = $file.FullName.Substring($projectRoot.Length).TrimStart('\', '/')
    Write-Host "  Analyzing: $relativePath" -ForegroundColor $InfoColor -NoNewline
    
    $output = & clang-tidy $tidyArgs $file.FullName 2>&1 | Out-String
    
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 1) {
        Write-Host " [ERROR]" -ForegroundColor $ErrorColor
        Write-Host $output -ForegroundColor $ErrorColor
        $tidyIssues += @{ File = $file.FullName; Issues = $output }
    } elseif ($output.Trim()) {
        Write-Host " [ISSUES]" -ForegroundColor $WarningColor
        Write-Host $output -ForegroundColor $WarningColor
        $tidyIssues += @{ File = $file.FullName; Issues = $output }
    } else {
        Write-Host " [OK]" -ForegroundColor $SuccessColor
    }
    
    $tidyOutput += $output
}

# Run cppcheck
$cppcheckIssues = @()
$cppcheckOutput = @()

Write-Host "`nRunning cppcheck..." -ForegroundColor $InfoColor
$cppcheckArgs = @(
    "--enable=all",
    "--suppress=missingIncludeSystem",
    "--suppress=unusedFunction",
    "--suppress=missingInclude",
    "--inline-suppr",
    "--std=c++20",
    "--platform=win64",
    "-I$projectRoot/include",
    "-I$projectRoot/src"
)

# Add exclude patterns
foreach ($pattern in $Exclude) {
    $cppcheckArgs += "-i$pattern"
}

# Add source files
$sourceFilePaths = $sourceFiles | ForEach-Object { $_.FullName }
$cppcheckArgs += $sourceFilePaths

# Run cppcheck
$output = & cppcheck $cppcheckArgs 2>&1 | Out-String

if ($LASTEXITCODE -ne 0) {
    Write-Host $output -ForegroundColor $ErrorColor
    $cppcheckIssues += $output
} elseif ($output.Trim()) {
    Write-Host $output -ForegroundColor $WarningColor
    $cppcheckIssues += $output
} else {
    Write-Host "No issues found with cppcheck." -ForegroundColor $SuccessColor
}

# Generate report
$report = @"
# Static Code Analysis Report

## Summary
- Files analyzed: $($sourceFiles.Count)
- Issues found: $(($tidyIssues.Count + $cppcheckIssues.Count))

## clang-tidy
$($tidyIssues | ForEach-Object { "### $($_.File)
```
$($_.Issues)
```
" } -join "`n")

## cppcheck
$($cppcheckIssues -join "`n")
"@

# Save report to file
$reportFile = Join-Path $projectRoot "static-analysis-report.md"
$report | Out-File -FilePath $reportFile -Encoding utf8
Write-Host "`nAnalysis complete. Report saved to: $reportFile" -ForegroundColor $InfoColor

# Exit with appropriate status
if (($tidyIssues.Count + $cppcheckIssues.Count) -gt 0) {
    exit 1
} else {
    exit 0
}
