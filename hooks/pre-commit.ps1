#!/usr/bin/env pwsh
#
# Pre-commit hook for PowerShell (Windows)
# To install: Copy this file to .git\hooks\pre-commit.ps1 and create a .git\hooks\pre-commit file with:
#   #!/bin/sh
#   powershell.exe -ExecutionPolicy Bypass -File .git/hooks/pre-commit.ps1
#   exit $?

# Colors for output
$Red = [System.Console]::ForegroundColor = "Red"
$Green = [System.Console]::ForegroundColor = "Green"
$Yellow = [System.Console]::ForegroundColor = "Yellow"
$ResetColor = [System.Console]::ResetColor()

Write-Host "Running pre-commit hook..." -ForegroundColor Yellow
Write-Host "Building and testing project..." -ForegroundColor Yellow

# Store the current directory
$REPO_ROOT = git rev-parse --show-toplevel
Set-Location $REPO_ROOT

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    New-Item -Path "build" -ItemType Directory | Out-Null
}

# Build project with CMake
Set-Location "build"
& cmake ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake configuration failed. Fix build errors before committing." -ForegroundColor Red
    exit 1
}

& cmake --build . --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed. Fix build errors before committing." -ForegroundColor Red
    exit 1
}

# Run critical tests (a subset of tests that run quickly)
Write-Host "Running critical tests..." -ForegroundColor Yellow
& ".\tests\Debug\RunAllTests.exe" --gtest_filter=*_Critical*
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Critical tests failed. Please fix tests before committing." -ForegroundColor Red
    Write-Host "To bypass this check (NOT RECOMMENDED), use --no-verify option." -ForegroundColor Yellow
    exit 1
}

Write-Host "✅ Build and critical tests passed. Proceeding with commit." -ForegroundColor Green
exit 0 