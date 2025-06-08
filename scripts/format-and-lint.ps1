<#
.SYNOPSIS
    Formats and lints the codebase according to project standards.
.DESCRIPTION
    This script provides a unified interface for code formatting and linting.
    It ensures consistent code style and catches potential issues early.
.PARAMETER Check
    Only check for issues without making changes.
.PARAMETER Fix
    Automatically fix issues where possible.
.PARAMECTOR Include
    Comma-separated list of file patterns to include (default: *.cpp,*.hpp,*.h,*.cc,*.c,*.cxx,*.hxx).
.PARAMECTOR Exclude
    Comma-separated list of file patterns to exclude.
.PARAMECTOR NoColor
    Disable colored output.
.PARAMECTOR Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\format-and-lint.ps1 -Check
    Check for formatting and linting issues without making changes.
.EXAMPLE
    .\scripts\format-and-lint.ps1 -Fix
    Automatically fix formatting and linting issues.
#>

[CmdletBinding()]
param(
    [switch]$Check,
    [switch]$Fix,
    [string]$Include = "*.cpp,*.hpp,*.h,*.cc,*.c,*.cxx,*.hxx,*.py,*.md,*.yaml,*.yml,*.json",
    [string]$Exclude = "build/*,third_party/*,external/*,*.pb.*,*.pb.h,*.pb.cc",
    [switch]$NoColor,
    [switch]$Verbose
)

# Error handling
$ErrorActionPreference = "Stop"

# Colors for output
$ErrorColor = if ($NoColor) { "" } else { "Red" }
$WarningColor = if ($NoColor) { "" } else { "Yellow" }
$SuccessColor = if ($NoColor) { "" } else { "Green" }
$InfoColor = if ($NoColor) { "" } else { "Cyan" }

# Get the project root directory
$projectRoot = Split-Path -Parent $PSScriptRoot
$script:VerbosePreference = if ($Verbose) { "Continue" } else { "SilentlyContinue" }

# Function to run a command and check its exit code
function Invoke-CommandChecked {
    param(
        [string]$Command,
        [string[]]$Arguments,
        [string]$WorkingDirectory = $PWD.Path,
        [switch]$Fatal,
        [switch]$NoOutput
    )
    
    $commandLine = "$Command $($Arguments -join ' ')"
    Write-Host "$commandLine" -ForegroundColor $InfoColor
    
    $process = Start-Process -FilePath $Command -ArgumentList $Arguments -NoNewWindow -PassThru -WorkingDirectory $WorkingDirectory
    $process.WaitForExit()
    
    if ($process.ExitCode -ne 0 -and $Fatal) {
        Write-Host "Command failed with exit code $($process.ExitCode): $commandLine" -ForegroundColor $ErrorColor
        exit $process.ExitCode
    }
    
    return $process.ExitCode
}

# Function to find files matching patterns
function Get-FilesByPattern {
    param(
        [string]$Include,
        [string]$Exclude
    )
    
    $includePatterns = $Include -split ',' | ForEach-Object { $_.Trim() }
    $excludePatterns = $Exclude -split ',' | ForEach-Object { $_.Trim() }
    
    $files = @()
    
    # Process each include pattern
    foreach ($pattern in $includePatterns) {
        $includeFiles = Get-ChildItem -Path $projectRoot -Recurse -File -Filter $pattern -ErrorAction SilentlyContinue | 
                      Where-Object { $_.FullName -notmatch '\\(build|third_party|external|_deps)\\' }
        
        # Apply exclude patterns
        foreach ($excludePattern in $excludePatterns) {
            $includeFiles = $includeFiles | Where-Object { $_.FullName -notlike "*$excludePattern*" }
        }
        
        $files += $includeFiles
    }
    
    return $files | Sort-Object -Property FullName -Unique
}

# Function to format C++ files with clang-format
function Format-CppFiles {
    param(
        [array]$Files,
        [switch]$Check
    )
    
    if ($Files.Count -eq 0) {
        Write-Host "No C++ files found to format." -ForegroundColor $InfoColor
        return $true
    }
    
    Write-Host "Formatting $($Files.Count) C++ files..." -ForegroundColor $InfoColor
    
    $clangFormatArgs = @()
    
    if ($Check) {
        $clangFormatArgs += "--dry-run"
        $clangFormatArgs += "--Werror"
    } else {
        $clangFormatArgs += "-i"
    }
    
    $clangFormatArgs += "-style=file"
    $clangFormatArgs += "--verbose"
    
    # Process files in batches to avoid command line length limits
    $batchSize = 50
    $success = $true
    
    for ($i = 0; $i -lt $Files.Count; $i += $batchSize) {
        $batch = $Files | Select-Object -Skip $i -First $batchSize
        $fileList = $batch.FullName
        
        $tempFile = [System.IO.Path]::GetTempFileName()
        $fileList | Out-File -FilePath $tempFile -Encoding utf8
        
        $exitCode = Invoke-CommandChecked -Command "clang-format" -Arguments (@($clangFormatArgs) + "@$tempFile") -Fatal:$Fatal
        
        Remove-Item -Path $tempFile -Force
        
        if ($exitCode -ne 0) {
            $success = $false
            if (-not $Fatal) { break }
        }
    }
    
    return $success
}

# Function to format Python files with black and isort
function Format-PythonFiles {
    param(
        [array]$Files,
        [switch]$Check
    )
    
    $pythonFiles = $Files | Where-Object { $_.Extension -eq ".py" }
    
    if ($pythonFiles.Count -eq 0) {
        Write-Host "No Python files found to format." -ForegroundColor $InfoColor
        return $true
    }
    
    Write-Host "Formatting $($pythonFiles.Count) Python files..." -ForegroundColor $InfoColor
    
    $blackArgs = @(
        "black",
        "--line-length", "100"
    )
    
    $isortArgs = @(
        "isort",
        "--profile", "black",
        "--line-length", "100"
    )
    
    if ($Check) {
        $blackArgs += "--check"
        $isortArgs += "--check-only"
    }
    
    $blackArgs += $pythonFiles.FullName
    $isortArgs += $pythonFiles.FullName
    
    $success = $true
    
    # Run isort
    $exitCode = Invoke-CommandChecked -Command "python" -Arguments ("-m") -Fatal:$Fatal
    if ($exitCode -ne 0) { $success = $false }
    
    # Run black
    $exitCode = Invoke-CommandChecked -Command "python" -Arguments ("-m" + $blackArgs) -Fatal:$Fatal
    if ($exitCode -ne 0) { $success = $false }
    
    return $success
}

# Function to format Markdown, YAML, and JSON files with prettier
function Format-OtherFiles {
    param(
        [array]$Files,
        [switch]$Check
    )
    
    $otherFiles = $Files | Where-Object { 
        $_.Extension -in @(".md", ".yaml", ".yml", ".json") 
    }
    
    if ($otherFiles.Count -eq 0) {
        Write-Host "No Markdown/YAML/JSON files found to format." -ForegroundColor $InfoColor
        return $true
    }
    
    Write-Host "Formatting $($otherFiles.Count) Markdown/YAML/JSON files..." -ForegroundColor $InfoColor
    
    $prettierArgs = @(
        "prettier",
        "--config", "$projectRoot/.prettierrc",
        "--ignore-path", "$projectRoot/.prettierignore"
    )
    
    if ($Check) {
        $prettierArgs += "--check"
    } else {
        $prettierArgs += "--write"
    }
    
    $prettierArgs += $otherFiles.FullName
    
    $exitCode = Invoke-CommandChecked -Command "npx" -Arguments $prettierArgs -Fatal:$Fatal
    return $exitCode -eq 0
}

# Function to run clang-tidy for static analysis
function Invoke-StaticAnalysis {
    param(
        [array]$Files,
        [switch]$Fix
    )
    
    $cppFiles = $Files | Where-Object { 
        $_.Extension -in @(".cpp", ".cc", ".cxx", ".hpp", ".h", ".hxx") 
    }
    
    if ($cppFiles.Count -eq 0) {
        Write-Host "No C++ files found for static analysis." -ForegroundColor $InfoColor
        return $true
    }
    
    Write-Host "Running static analysis on $($cppFiles.Count) C++ files..." -ForegroundColor $InfoColor
    
    $tidyArgs = @(
        "--quiet",
        "--header-filter=.*",
        "--checks=*",
        "--warnings-as-errors=*",
        "--format-style=file"
    )
    
    if ($Fix) {
        $tidyArgs += "--fix"
        $tidyArgs += "--fix-errors"
    }
    
    # Check for compile_commands.json
    $buildDir = Join-Path $projectRoot "build"
    $compileCommands = Join-Path $buildDir "compile_commands.json"
    
    if (Test-Path $compileCommands) {
        $tidyArgs += "-p=$buildDir"
    } else {
        Write-Host "Warning: compile_commands.json not found. Analysis may be less accurate." -ForegroundColor $WarningColor
    }
    
    $success = $true
    
    # Process files in batches to avoid command line length limits
    $batchSize = 10
    
    for ($i = 0; $i -lt $cppFiles.Count; $i += $batchSize) {
        $batch = $cppFiles | Select-Object -Skip $i -First $batchSize
        
        foreach ($file in $batch) {
            $fileArgs = $tidyArgs + $file.FullName
            $exitCode = Invoke-CommandChecked -Command "clang-tidy" -Arguments $fileArgs -Fatal:$false
            
            if ($exitCode -ne 0) {
                $success = $false
                if (-not $Fatal) { break }
            }
        }
    }
    
    return $success
}

# Main script execution
try {
    # Find all files matching include/exclude patterns
    $files = Get-FilesByPattern -Include $Include -Exclude $Exclude
    
    if ($files.Count -eq 0) {
        Write-Host "No files found matching the specified patterns." -ForegroundColor $WarningColor
        exit 0
    }
    
    Write-Host "Found $($files.Count) files to process." -ForegroundColor $InfoColor
    
    $success = $true
    
    # Format C++ files
    $cppFiles = $files | Where-Object { 
        $_.Extension -in @(".cpp", ".cc", ".cxx", ".hpp", ".h", ".hxx") 
    }
    
    if (-not (Format-CppFiles -Files $cppFiles -Check:$Check)) {
        $success = $false
        if (-not $Fatal) { exit 1 }
    }
    
    # Format Python files
    if (-not (Format-PythonFiles -Files $files -Check:$Check)) {
        $success = $false
        if (-not $Fatal) { exit 1 }
    }
    
    # Format other files (Markdown, YAML, JSON)
    if (-not (Format-OtherFiles -Files $files -Check:$Check)) {
        $success = $false
        if (-not $Fatal) { exit 1 }
    }
    
    # Run static analysis
    if (-not (Invoke-StaticAnalysis -Files $files -Fix:$Fix)) {
        $success = $false
        if (-not $Fatal) { exit 1 }
    }
    
    if ($success) {
        Write-Host "All checks passed successfully!" -ForegroundColor $SuccessColor
        exit 0
    } else {
        Write-Host "Some checks failed. See above for details." -ForegroundColor $ErrorColor
        exit 1
    }
}
catch {
    Write-Host "An error occurred:" -ForegroundColor $ErrorColor
    Write-Host $_.Exception.Message -ForegroundColor $ErrorColor
    Write-Host $_.ScriptStackTrace -ForegroundColor $ErrorColor
    exit 1
}
