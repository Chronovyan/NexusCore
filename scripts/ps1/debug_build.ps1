param (
    [switch]$clean = $false,
    [string]$testFilter = "",
    [int]$jobs = 0
)

# Determine the number of parallel jobs if not specified
if ($jobs -eq 0) {
    $jobs = [int](Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
}

# Create specialized debug build directory
$buildDir = "build_debug"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created debug build directory: $buildDir"
}

# Clean if requested
if ($clean) {
    Write-Host "Cleaning debug build directory..."
    Remove-Item -Path "$buildDir\*" -Recurse -Force
}

# Change to the build directory
Push-Location $buildDir

# Configure with debug optimizations
Write-Host "Configuring optimized debug build..."
$configureCmd = "cmake .. -G ""Visual Studio 17 2022"" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON"
Invoke-Expression $configureCmd

# Build with specified number of jobs
Write-Host "Building with $jobs parallel jobs..."
$buildCmd = "cmake --build . --config Debug --parallel $jobs"
Invoke-Expression $buildCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed with error code $LASTEXITCODE" -ForegroundColor Red
    Pop-Location
    exit $LASTEXITCODE
}

# Run specific test if filter is provided
if ($testFilter) {
    Write-Host "Running tests matching filter: $testFilter" -ForegroundColor Cyan
    $testCmd = "ctest -C Debug -R ""$testFilter"" --output-on-failure"
    Invoke-Expression $testCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Tests passed successfully!" -ForegroundColor Green
    } else {
        Write-Host "Tests failed with error code $LASTEXITCODE" -ForegroundColor Red
    }
}

# Return to original directory
Pop-Location

Write-Host "Debug build completed successfully!" -ForegroundColor Green
Write-Host "You can run specific tests with: .\scripts\debug_build.ps1 -testFilter CommandTest" 