#!/usr/bin/env pwsh
#
# Simplified Pre-commit hook for PowerShell (Windows)
# This script performs basic checks without requiring a full build

function Write-ColorOutput($message, $color) {
    $prevColor = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $color
    Write-Output $message
    $host.UI.RawUI.ForegroundColor = $prevColor
}

Write-ColorOutput "Running pre-commit hook..." "Yellow"

try {
    # Store the current directory
    $REPO_ROOT = git rev-parse --show-toplevel
    $CurrentDir = Get-Location
    Set-Location $REPO_ROOT
    
    # Get list of staged C++ files
    $stagedFiles = git diff --cached --name-only --diff-filter=ACMR | Where-Object { $_ -match "\.(cpp|h|hpp)$" }
    
    if ($stagedFiles.Count -eq 0) {
        Write-ColorOutput "No C++ files staged for commit. Skipping checks." "Green"
        Set-Location $CurrentDir
        exit 0
    }
    
    Write-ColorOutput "Checking syntax for staged C++ files..." "Yellow"
    $hasErrors = $false
    
    # Check syntax with clang or g++ without building
    foreach ($file in $stagedFiles) {
        Write-ColorOutput "Checking $file..." "Yellow"
        
        # Try to use clang or g++ for syntax checking
        if (Get-Command "clang++" -ErrorAction SilentlyContinue) {
            # Capture both output and error streams
            $output = & clang++ -fsyntax-only -Wall $file 2>&1
            $exitCode = $LASTEXITCODE
        } elseif (Get-Command "g++" -ErrorAction SilentlyContinue) {
            # Capture both output and error streams
            $output = & g++ -fsyntax-only -Wall $file 2>&1
            $exitCode = $LASTEXITCODE
        } else {
            Write-ColorOutput "No C++ compiler found for syntax checking. Skipping." "Yellow"
            continue
        }
        
        if ($exitCode -ne 0) {
            Write-ColorOutput "Syntax error in $file:" "Red"
            Write-ColorOutput $output "Red"
            $hasErrors = $true
        }
    }
    
    if ($hasErrors) {
        Write-ColorOutput "Syntax errors found. Please fix them before committing." "Red"
        Write-ColorOutput "To bypass this check (NOT RECOMMENDED), use --no-verify option." "Yellow"
        Set-Location $CurrentDir
        exit 1
    }
    
    # Simple check for OpenAI_API_Client.cpp implementation
    $apiClientPath = Join-Path $REPO_ROOT "src" "OpenAI_API_Client.cpp"
    if (Test-Path $apiClientPath) {
        $content = Get-Content $apiClientPath -Raw
        if ($content -match "not fully implemented yet") {
            Write-ColorOutput "Warning: OpenAI_API_Client.cpp contains 'not fully implemented yet' comments." "Yellow"
            Write-ColorOutput "If you've implemented these methods, please update the comments." "Yellow"
        }
    }
    
    Write-ColorOutput "All syntax checks passed. Proceeding with commit." "Green"
    Set-Location $CurrentDir
    exit 0
}
catch {
    Write-ColorOutput "An error occurred in the pre-commit hook" -ForegroundColor Red
    Write-ColorOutput $_.Exception.Message -ForegroundColor Red
    Write-ColorOutput "To bypass this check (use caution!), use --no-verify option." -ForegroundColor Yellow
    if ($CurrentDir) {
        Set-Location $CurrentDir
    }
    exit 1
}

# Advanced Pre-commit Hook: Build and run all Critical tests before allowing commit

Write-Host "Running pre-commit: build and run critical tests..." -ForegroundColor Yellow

# Store original directory
$originalDir = Get-Location

try {
    # Step 1: Build the project
    Write-Host "Configuring project with CMake..." -ForegroundColor Cyan
    cmake -S . -B build
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configure failed." -ForegroundColor Red
        exit 1
    }

    Write-Host "Building project..." -ForegroundColor Cyan
    cmake --build build
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed." -ForegroundColor Red
        exit 1
    }

    # Step 2: Run only critical tests
    Write-Host "Running critical tests..." -ForegroundColor Cyan
    Set-Location build
    ctest -R Critical --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Critical tests failed." -ForegroundColor Red
        exit 1
    }

    Write-Host "All critical tests passed. Commit allowed." -ForegroundColor Green
    exit 0
}
catch {
    Write-Host "An error occurred in the pre-commit hook" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
finally {
    # Always return to original directory
    Set-Location $originalDir
} 