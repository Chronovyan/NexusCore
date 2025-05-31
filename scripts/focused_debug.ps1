param (
    [switch]$clean = $false,
    [string]$testFilter = "",
    [string]$target = "",
    [int]$jobs = 0
)

# Determine the number of parallel jobs if not specified
if ($jobs -eq 0) {
    $jobs = [int](Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
}

# Create specialized debug build directory
$buildDir = "build_focused"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created focused debug build directory: $buildDir"
}

# Clean if requested
if ($clean) {
    Write-Host "Cleaning focused debug build directory..." -ForegroundColor Yellow
    Remove-Item -Path "$buildDir\*" -Recurse -Force
}

# Change to the build directory
Push-Location $buildDir

# Configure with debug optimizations and minimal dependencies
Write-Host "Configuring focused debug build..." -ForegroundColor Cyan
$configureCmd = "cmake .. -G ""Visual Studio 17 2022"" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON"
Invoke-Expression $configureCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Configuration failed with error code $LASTEXITCODE" -ForegroundColor Red
    Pop-Location
    exit $LASTEXITCODE
}

# Build only the specified target if provided, or just the essential components
if ($target) {
    Write-Host "Building target: $target with $jobs parallel jobs..." -ForegroundColor Cyan
    $buildCmd = "cmake --build . --config Debug --target $target --parallel $jobs"
} else {
    # Build only the minimal components needed for debugging
    Write-Host "Building essential components with $jobs parallel jobs..." -ForegroundColor Cyan
    # We could specify EditorLib or other core targets here
    $buildCmd = "cmake --build . --config Debug --parallel $jobs"
}

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

Write-Host "Focused debug build completed!" -ForegroundColor Green
Write-Host "Usage examples:"
Write-Host "  - Build specific target: .\scripts\focused_debug.ps1 -target EditorLib" -ForegroundColor Cyan
Write-Host "  - Run specific tests:   .\scripts\focused_debug.ps1 -testFilter CommandTest" -ForegroundColor Cyan
Write-Host "  - Clean and rebuild:    .\scripts\focused_debug.ps1 -clean" -ForegroundColor Cyan 