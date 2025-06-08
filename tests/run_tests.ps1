# Run tests with timeouts and proper cleanup
$ErrorActionPreference = "Stop"

# Use temp directory for build to avoid permission issues
$tempDir = [System.IO.Path]::GetTempPath()
$buildDir = Join-Path $tempDir "TextEditorTests_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
$sourceDir = $PSScriptRoot

Write-Host "Using temporary build directory: $buildDir"

# Create build directory
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

try {
    # Configure with CMake
    Write-Host "Configuring CMake..."
    & cmake -G "Visual Studio 17 2022" -A x64 -S $sourceDir -B $buildDir
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }

    # Build the project
    Write-Host "Building project..."
    & cmake --build $buildDir --config Debug
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    # Run tests with timeout - look in both possible locations
    $testExecutable = Join-Path $buildDir "bin\Debug\SimpleEditorTest.exe"
    if (-not (Test-Path $testExecutable)) {
        $testExecutable = Join-Path $buildDir "Debug\SimpleEditorTest.exe"
    }
    if (-not (Test-Path $testExecutable)) {
        throw "Test executable not found at $testExecutable"
    }

    Write-Host "Running tests with 30 second timeout..."
    $process = Start-Process -FilePath $testExecutable -NoNewWindow -PassThru
    
    # Wait for process to complete with timeout
    $process | Wait-Process -Timeout 30 -ErrorAction SilentlyContinue
    
    if (-not $process.HasExited) {
        # If process is still running after timeout, kill it
        Write-Host "Test execution timed out after 30 seconds. Terminating..."
        Stop-Process -Id $process.Id -Force
        throw "Test execution timed out"
    }
    
    if ($process.ExitCode -ne 0) {
        throw "Tests failed with exit code $($process.ExitCode)"
    }
    
    Write-Host "Tests completed successfully!"
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
    exit 1
}
finally {
    # Clean up build directory
    if (Test-Path $buildDir) {
        Write-Host "Cleaning up temporary build directory..."
        Remove-Item -Path $buildDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

exit 0
