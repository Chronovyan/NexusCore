# Setup script for RPG Demo
# This script sets up the build environment and runs the project

# Set error action preference
$ErrorActionPreference = "Stop"

# Function to check if a command exists
function Test-Command {
    param($command)
    $exists = $null -ne (Get-Command $command -ErrorAction SilentlyContinue)
    return $exists
}

# Check if vcpkg is in the PATH
$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot) {
    # Try to find vcpkg in common locations
    $possiblePaths = @(
        "${env:USERPROFILE}\vcpkg",
        "${env:PROGRAMFILES}\vcpkg",
        "C:\vcpkg"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path "$path\vcpkg.exe") {
            $vcpkgRoot = $path
            break
        }
    }
    
    if (-not $vcpkgRoot) {
        Write-Error "vcpkg not found. Please install vcpkg and ensure it's in your PATH or set VCPKG_ROOT environment variable."
        exit 1
    }
}

# Add vcpkg to PATH if not already there
$env:PATH = "$vcpkgRoot;$env:PATH"

# Check if CMake is installed
if (-not (Test-Command "cmake")) {
    Write-Error "CMake is not installed or not in PATH. Please install CMake and add it to your PATH."
    exit 1
}

# Create build directory if it doesn't exist
$buildDir = "$PSScriptRoot\build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Configure the project
Write-Host "Configuring the project..." -ForegroundColor Cyan
Push-Location $buildDir

# Run CMake with vcpkg toolchain
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$vcpkgRoot\scripts\buildsystems\vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    Pop-Location
    exit 1
}

# Build the project
Write-Host "`nBuilding the project..." -ForegroundColor Cyan
cmake --build . --config Debug -- /m

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    Pop-Location
    exit 1
}

# Run tests
Write-Host "`nRunning tests..." -ForegroundColor Cyan
ctest -C Debug --output-on-failure

if ($LASTEXITCODE -ne 0) {
    Write-Error "Some tests failed"
    Pop-Location
    exit 1
}

Pop-Location

Write-Host "`nSetup and build completed successfully!" -ForegroundColor Green
Write-Host "You can now run the application from: $buildDir\bin\Debug\ai_text_rpg.exe" -ForegroundColor Green
