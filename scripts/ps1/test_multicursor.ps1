# MultiCursor Feature Test Script
# This script builds and runs the multiple cursor tests

param (
    [switch]$skipBuild = $false,
    [switch]$onlyBuildTests = $false
)

$ErrorActionPreference = "Stop"
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootPath = Split-Path -Parent $scriptPath
$buildDir = Join-Path $rootPath "build"

Write-Host "=== MultiCursor Feature Test Script ===" -ForegroundColor Cyan
Write-Host "Root directory: $rootPath" -ForegroundColor Gray

# Build the tests if not skipping
if (-not $skipBuild) {
    Write-Host "`nStep 1: Building tests..." -ForegroundColor Yellow
    
    # Create build directory if it doesn't exist
    if (-not (Test-Path $buildDir)) {
        Write-Host "Creating build directory..." -ForegroundColor Gray
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }
    
    # Navigate to build directory
    Push-Location $buildDir
    
    try {
        # Configure with CMake
        Write-Host "Configuring with CMake..." -ForegroundColor Gray
        cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed with exit code $LASTEXITCODE"
        }
        
        # Build
        Write-Host "Building tests..." -ForegroundColor Gray
        cmake --build . --config Debug --target MultiCursorTest EditorMultiCursorTest
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE"
        }
        
        # Build the main application too if we're not only building tests
        if (-not $onlyBuildTests) {
            Write-Host "Building main application..." -ForegroundColor Gray
            cmake --build . --config Debug --target AITextEditor
            if ($LASTEXITCODE -ne 0) {
                throw "Main application build failed with exit code $LASTEXITCODE"
            }
        }
    }
    finally {
        # Return to original directory
        Pop-Location
    }
}

# Run the tests if we're not just building
if (-not $onlyBuildTests) {
    Write-Host "`nStep 2: Running MultiCursor tests..." -ForegroundColor Yellow
    
    $multiCursorTestPath = Join-Path $buildDir "tests\Debug\MultiCursorTest.exe"
    $editorMultiCursorTestPath = Join-Path $buildDir "tests\Debug\EditorMultiCursorTest.exe"
    
    # Run MultiCursorTest
    if (Test-Path $multiCursorTestPath) {
        Write-Host "`nRunning MultiCursorTest..." -ForegroundColor Magenta
        & $multiCursorTestPath
        if ($LASTEXITCODE -ne 0) {
            Write-Host "MultiCursorTest failed with exit code $LASTEXITCODE" -ForegroundColor Red
        } else {
            Write-Host "MultiCursorTest passed successfully!" -ForegroundColor Green
        }
    } else {
        Write-Host "MultiCursorTest executable not found at $multiCursorTestPath" -ForegroundColor Red
    }
    
    # Run EditorMultiCursorTest
    if (Test-Path $editorMultiCursorTestPath) {
        Write-Host "`nRunning EditorMultiCursorTest..." -ForegroundColor Magenta
        & $editorMultiCursorTestPath
        if ($LASTEXITCODE -ne 0) {
            Write-Host "EditorMultiCursorTest failed with exit code $LASTEXITCODE" -ForegroundColor Red
        } else {
            Write-Host "EditorMultiCursorTest passed successfully!" -ForegroundColor Green
        }
    } else {
        Write-Host "EditorMultiCursorTest executable not found at $editorMultiCursorTestPath" -ForegroundColor Red
    }
    
    # Offer to run the main application for manual testing
    $appPath = Join-Path $buildDir "Debug\AITextEditor.exe"
    if (Test-Path $appPath) {
        $runApp = Read-Host "`nWould you like to run the application for manual testing? (y/n)"
        if ($runApp.ToLower() -eq "y") {
            Write-Host "`nLaunching AITextEditor for manual testing..." -ForegroundColor Yellow
            Write-Host "Try the following multiple cursor operations:" -ForegroundColor Cyan
            Write-Host "- Press Ctrl+Alt+Up/Down to add cursors above/below" -ForegroundColor Cyan
            Write-Host "- Press Alt+Click to add cursors at specific positions" -ForegroundColor Cyan
            Write-Host "- Select a word and press Ctrl+Alt+D to add cursors at all occurrences" -ForegroundColor Cyan
            Write-Host "- Press Escape to return to single cursor mode" -ForegroundColor Cyan
            Write-Host "`nPress Ctrl+C to exit the application when done testing.`n" -ForegroundColor Gray
            
            & $appPath
        }
    } else {
        Write-Host "`nMain application executable not found at $appPath" -ForegroundColor Red
    }
}

Write-Host "`n=== Test script completed ===" -ForegroundColor Cyan 