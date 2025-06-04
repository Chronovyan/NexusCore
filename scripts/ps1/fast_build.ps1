param (
    [switch]$clean = $false,
    [string]$target = "",
    [string]$config = "Debug",
    [int]$jobs = 0,
    [switch]$unity = $true,
    [switch]$ninja = $false
)

# Determine the number of parallel jobs if not specified
if ($jobs -eq 0) {
    # Use logical processors plus 2 for best performance with hyperthreading
    $cpuCount = [int](Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
    $jobs = $cpuCount + 2
    
    # Limit to a reasonable number to prevent system from becoming unresponsive
    if ($jobs -gt 16) { $jobs = 16 }
}

# Create build directory
$buildDir = "build_fast"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created fast build directory: $buildDir"
}

# Clean if requested
if ($clean) {
    Write-Host "Cleaning fast build directory..." -ForegroundColor Yellow
    Remove-Item -Path "$buildDir\*" -Recurse -Force
}

# Change to the build directory
Push-Location $buildDir

# Set generator and build tool options
$generator = "Visual Studio 17 2022"
$buildToolOptions = ""

if ($ninja) {
    # Check if ninja is available
    $ninjaPath = Get-Command ninja -ErrorAction SilentlyContinue
    if ($ninjaPath) {
        $generator = "Ninja"
        Write-Host "Using Ninja build system for faster builds" -ForegroundColor Green
    } else {
        Write-Host "Ninja not found in PATH. Falling back to Visual Studio generator." -ForegroundColor Yellow
        Write-Host "Consider installing Ninja for faster builds: choco install ninja" -ForegroundColor Yellow
    }
}

# Configure with optimizations
Write-Host "Configuring optimized build with $jobs parallel jobs..." -ForegroundColor Cyan
$vcpkgToolchainFile = "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
$configureCmd = "cmake .. -G ""$generator"" -DCMAKE_BUILD_TYPE=$config -DCMAKE_TOOLCHAIN_FILE=""$vcpkgToolchainFile"" -DENABLE_UNITY_BUILD=$($unity.ToString().ToLower()) -DCMAKE_BUILD_PARALLEL_LEVEL=$jobs"

# Set environment variable for parallel builds
$env:CMAKE_BUILD_PARALLEL_LEVEL = $jobs

Invoke-Expression $configureCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Configuration failed with error code $LASTEXITCODE" -ForegroundColor Red
    Pop-Location
    exit $LASTEXITCODE
}

# Build target
$buildTarget = if ($target) { "--target $target" } else { "" }

Write-Host "Building with $jobs parallel jobs..." -ForegroundColor Cyan
$buildCmd = "cmake --build . --config $config $buildTarget --parallel $jobs"
Invoke-Expression $buildCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed with error code $LASTEXITCODE" -ForegroundColor Red
    Pop-Location
    exit $LASTEXITCODE
}

# Return to original directory
Pop-Location

Write-Host "Fast build completed successfully!" -ForegroundColor Green
Write-Host "Usage examples:"
Write-Host "  - Basic build:          .\scripts\fast_build.ps1" -ForegroundColor Cyan
Write-Host "  - Clean and rebuild:    .\scripts\fast_build.ps1 -clean" -ForegroundColor Cyan
Write-Host "  - Build specific target: .\scripts\fast_build.ps1 -target EditorLib" -ForegroundColor Cyan
Write-Host "  - Release build:        .\scripts\fast_build.ps1 -config Release" -ForegroundColor Cyan
Write-Host "  - Build with Ninja:     .\scripts\fast_build.ps1 -ninja" -ForegroundColor Cyan
Write-Host "  - Disable unity builds: .\scripts\fast_build.ps1 -unity:$false" -ForegroundColor Cyan 