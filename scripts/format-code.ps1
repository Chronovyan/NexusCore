<#
.SYNOPSIS
    Formats C++ code using clang-format
.DESCRIPTION
    This script formats all C++ source files in the project using clang-format.
    It can be used to check formatting or automatically fix formatting issues.
.PARAMETER CheckOnly
    If specified, only checks formatting without making changes.
.PARAMETER Fix
    If specified, fixes formatting issues in place.
.EXAMPLE
    .\scripts\format-code.ps1 -CheckOnly
    Checks code formatting without making changes.
.EXAMPLE
    .\scripts\format-code.ps1 -Fix
    Fixes formatting issues in place.
#>

param(
    [Parameter(ParameterSetName='CheckOnly')]
    [switch]$CheckOnly,
    
    [Parameter(ParameterSetName='Fix')]
    [switch]$Fix
)

# Error handling
$ErrorActionPreference = "Stop"

# Check if clang-format is installed
$clangFormat = "clang-format"
if (-not (Get-Command $clangFormat -ErrorAction SilentlyContinue)) {
    Write-Error "clang-format is not installed. Please install it and try again."
    exit 1
}

# Get the root directory of the project
$rootDir = Split-Path -Parent $PSScriptRoot

# Find all C++ files
$cppFiles = Get-ChildItem -Path $rootDir -Recurse -Include *.h,*.hpp,*.cpp,*.c,*.cc,*.cxx | 
    Where-Object { $_.FullName -notmatch '\\build\\|\\third_party\\|\\external\\|\\.git\\|\\.vs\\|\\cmake-*' }

$formatErrors = @()

foreach ($file in $cppFiles) {
    $filePath = $file.FullName
    
    if ($CheckOnly) {
        # Check formatting without making changes
        $output = & $clangFormat -style=file -output-replacements-xml $filePath | Select-String -Pattern "<replacement " -SimpleMatch
        
        if ($output) {
            $formatErrors += $filePath
            Write-Host "[CHECK] Format issues found in: $filePath" -ForegroundColor Red
        } else {
            Write-Host "[CHECK] Format OK: $filePath" -ForegroundColor Green
        }
    } 
    elseif ($Fix) {
        # Fix formatting in place
        Write-Host "[FIX] Formatting: $filePath" -ForegroundColor Yellow
        & $clangFormat -style=file -i $filePath
    }
}

# Report results
if ($CheckOnly) {
    if ($formatErrors.Count -gt 0) {
        Write-Host "`n❌ Found formatting issues in $($formatErrors.Count) files:" -ForegroundColor Red
        $formatErrors | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
        Write-Host "`nTo fix these issues, run: .\scripts\format-code.ps1 -Fix" -ForegroundColor Yellow
        exit 1
    } else {
        Write-Host "`n✅ All files are properly formatted!" -ForegroundColor Green
    }
} 
elseif ($Fix) {
    Write-Host "`n✅ Formatting complete!" -ForegroundColor Green
}
