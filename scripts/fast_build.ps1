param (
    [switch]$withTests = $false,
    [switch]$clean = $false,
    [switch]$release = $false,
    [int]$jobs = 0
)

# Determine the number of parallel jobs if not specified
if ($jobs -eq 0) {
    $jobs = [int](Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
}

# Create build directory
$buildDir = "build_fast"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created build directory: $buildDir"
}

# Clean if requested
if ($clean) {
    Write-Host "Cleaning build directory..."
    Remove-Item -Path "$buildDir\*" -Recurse -Force
}

# Determine build type
$buildType = if ($release) { "Release" } else { "Debug" }

# Change to the build directory
Push-Location $buildDir

# Configure with optimized settings
$testFlag = if ($withTests) { "ON" } else { "OFF" }
$configureCmd = "cmake .. -G ""Visual Studio 17 2022"" -DCMAKE_BUILD_TYPE=$buildType -DBUILD_TESTS=$testFlag"

Write-Host "Configuring with command: $configureCmd"
Invoke-Expression $configureCmd

# Build with specified number of jobs
$buildCmd = "cmake --build . --config $buildType --parallel $jobs"
Write-Host "Building with command: $buildCmd"
Invoke-Expression $buildCmd

# Return to original directory
Pop-Location

Write-Host "Build completed!"
if (!$withTests) {
    Write-Host "Note: Tests were not built. Use -withTests to include them."
} 