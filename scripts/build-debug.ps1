<#
.SYNOPSIS
    Builds the project in debug mode with debug symbols and sanitizers.
.DESCRIPTION
    This script configures and builds the project in debug mode with
    debug symbols and address/undefined behavior sanitizers enabled.
    It also sets up the build directory with the necessary CMake options.
.PARAMETER BuildDir
    The build directory to use (default: 'build-debug').
.PARAMETER Generator
    The CMake generator to use (default: 'Ninja' if available, otherwise 'Visual Studio 17 2022').
.PARAMETER Arch
    The target architecture (default: 'x64').
.PARAMETER VcpkgToolchain
    The path to the vcpkg toolchain file (if using vcpkg).
.EXAMPLE
    .\scripts\build-debug.ps1
    Builds the project in debug mode with default settings.
.EXAMPLE
    .\scripts\build-debug.ps1 -BuildDir "build" -Generator "Visual Studio 17 2022" -Arch "x64"
    Builds the project with custom build directory and generator.
#>

[CmdletBinding()]
param(
    [string]$BuildDir = "build-debug",
    [string]$Generator = "",
    [ValidateSet("Win32", "x64", "ARM", "ARM64")]
    [string]$Arch = "x64",
    [string]$VcpkgToolchain = ""
)

# Error handling
$ErrorActionPreference = "Stop"

# Colors for output
$ErrorColor = "Red"
$WarningColor = "Yellow"
$SuccessColor = "Green"
$InfoColor = "Cyan"

# Function to check if a command exists
function Test-CommandExists {
    param($command)
    $exists = $null -ne (Get-Command $command -ErrorAction SilentlyContinue)
    return $exists
}

# Function to find the best available generator
function Get-CMakeGenerator {
    param([string]$preferredGenerator = "")
    
    # If a preferred generator was specified, use it
    (-not [string]::IsNullOrEmpty($preferredGenerator)) {
        return $preferredGenerator
    }
    
    # Check for Ninja
    if (Test-CommandExists ninja) {
        return "Ninja"
    }
    
    # Check for Visual Studio 2022
    $vsWhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsInstallPath) {
            return "Visual Studio 17 2022"
        }
    }
    
    # Default to Visual Studio 2019
    return "Visual Studio 16 2019"
}

# Main script execution
Write-Host "Building project in debug mode..." -ForegroundColor $InfoColor

# Get the project root directory
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $projectRoot $BuildDir

# Create build directory if it doesn't exist
if (-not (Test-Path $buildPath)) {
    New-Item -ItemType Directory -Path $buildPath | Out-Null
}

# Determine the generator to use
$generator = Get-CMakeGenerator -preferredGenerator $Generator

# Prepare CMake arguments
$cmakeArgs = @(
    "-B", "`"$buildPath`"",
    "-S", "`"$projectRoot`"",
    "-G", "`"$generator`"",
    "-A", $Arch,
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DCMAKE_CXX_FLAGS=/Zi /FS /MP /sdl /W4 /WX /EHsc /permissive- /std:c++latest",
    "-DENABLE_ASAN=ON",
    "-DENABLE_UBSAN=ON",
    "-DENABLE_DEBUG_SYMBOLS=ON"
)

# Add vcpkg toolchain if specified
if (-not [string]::IsNullOrEmpty($VcpkgToolchain)) {
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=`"$VcpkgToolchain`""
}

# Run CMake configuration
Write-Host "Configuring build with $generator..." -ForegroundColor $InfoColor
& cmake $cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed with exit code $LASTEXITCODE" -ForegroundColor $ErrorColor
    exit $LASTEXITCODE
}

# Build the project
Write-Host "Building project..." -ForegroundColor $InfoColor
& cmake --build "$buildPath" --config Debug -- /m:8

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor $ErrorColor
    exit $LASTEXITCODE
}

Write-Host "`n✅ Build completed successfully!" -ForegroundColor $SuccessColor
Write-Host "Executable: $(Join-Path $buildPath "Debug\editor.exe")" -ForegroundColor $InfoColor
