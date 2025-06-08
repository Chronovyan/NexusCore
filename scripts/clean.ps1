<#
.SYNOPSIS
    Cleans the project by removing build artifacts and temporary files.
.DESCRIPTION
    This script removes build directories, generated files, and other artifacts
    to ensure a clean state for the next build.
.PARAMETER All
    If specified, also removes downloaded dependencies and IDE-specific files.
.PARAMETER BuildDirs
    Specifies custom build directories to clean (comma-separated).
    Default: 'build,build-debug,build-release,out,bin,lib,bin-int'.
.PARAMETER RemoveDirs
    Additional directories to remove (comma-separated).
.EXAMPLE
    .\scripts\clean.ps1
    Removes standard build artifacts.
.EXAMPLE
    .\scripts\clean.ps1 -All
    Removes all build artifacts, including downloaded dependencies.
#>

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [switch]$All,
    [string]$BuildDirs = "build,build-debug,build-release,out,bin,lib,bin-int",
    [string]$RemoveDirs = ""
)

# Error handling
$ErrorActionPreference = "Stop"

# Colors for output
$ErrorColor = "Red"
$WarningColor = "Yellow"
$SuccessColor = "Green"
$InfoColor = "Cyan"

# Get the project root directory
$projectRoot = Split-Path -Parent $PSScriptRoot

# Combine default and additional build directories
$allBuildDirs = $BuildDirs
if (-not [string]::IsNullOrEmpty($RemoveDirs)) {
    $allBuildDirs = "$BuildDirs,$RemoveDirs"
}

# Convert comma-separated strings to arrays
$buildDirsToRemove = $allBuildDirs -split ',' | ForEach-Object { $_.Trim() } | Where-Object { -not [string]::IsNullOrEmpty($_) }

# Files and directories to remove
$filesToRemove = @(
    "CMakeCache.txt",
    "CMakeFiles",
    "CMakeScripts",
    "Makefile",
    "cmake_install.cmake",
    "install_manifest.txt",
    "compile_commands.json",
    "*.sln",
    "*.vcxproj*",
    "*.vcxproj.filters",
    "*.vcxproj.user",
    "*.vcxproj.filters.user",
    "*.suo",
    "*.user",
    "*.userosscache",
    "*.sln.docstates",
    "*.opensdf",
    "*.sdf",
    "*.VC.db",
    "*.VC.VC.opendb",
    "*.log",
    "*.tlog",
    "*.pdb",
    "*.ilk",
    "*.exp",
    "*.lib",
    "*.dll",
    "*.exe",
    "*.dylib",
    "*.so",
    "*.a",
    "*.o",
    "*.obj",
    "*.i",
    "*.ii",
    "*.s",
    "*.bmp",
    "*.pch",
    "*.res",
    "*.rc",
    "*.manifest",
    ".vs",
    ".vscode",
    ".idea",
    ".clang-tidy",
    ".clangd",
    "_deps",
    "Testing"
)

# Additional files to remove if -All is specified
$additionalFilesToRemove = @(
    "vcpkg",
    "packages",
    "conanbuildinfo.*",
    "conaninfo.txt",
    "conanbuildinfo.txt",
    "conan_imports_manifest.txt",
    "conanbuildinfo.json",
    "conanfile.txt",
    "conan.lock",
    "graph_info.json",
    "conan_imports_manifest.txt",
    "conan_imports_manifest_debug.txt",
    "conan_imports_manifest_release.txt"
)

# Function to remove a directory if it exists
function Remove-DirectoryIfExists {
    param([string]$path)
    
    if (Test-Path $path) {
        if ($PSCmdlet.ShouldProcess($path, "Remove directory")) {
            try {
                Remove-Item -Path $path -Recurse -Force -ErrorAction Stop
                Write-Host "Removed directory: $path" -ForegroundColor $SuccessColor
            } catch {
                Write-Host "Failed to remove directory: $path" -ForegroundColor $ErrorColor
                Write-Host $_.Exception.Message -ForegroundColor $ErrorColor
            }
        }
    }
}

# Function to remove files matching a pattern
function Remove-FilesMatchingPattern {
    param([string]$pattern)
    
    $files = Get-ChildItem -Path $projectRoot -Recurse -Force -Include $pattern -ErrorAction SilentlyContinue
    foreach ($file in $files) {
        if ($PSCmdlet.ShouldProcess($file.FullName, "Remove file")) {
            try {
                Remove-Item -Path $file.FullName -Force -ErrorAction Stop
                Write-Host "Removed file: $($file.FullName)" -ForegroundColor $SuccessColor
            } catch {
                Write-Host "Failed to remove file: $($file.FullName)" -ForegroundColor $ErrorColor
                Write-Host $_.Exception.Message -ForegroundColor $ErrorColor
            }
        }
    }
}

# Main script execution
Write-Host "Cleaning project..." -ForegroundColor $InfoColor

# Remove build directories
foreach ($dir in $buildDirsToRemove) {
    $fullPath = Join-Path $projectRoot $dir
    Remove-DirectoryIfExists -path $fullPath
}

# Remove files and directories
foreach ($item in $filesToRemove) {
    if ($item -match "\*" -or $item -match "\?") {
        # It's a pattern, use Remove-FilesMatchingPattern
        Remove-FilesMatchingPattern -pattern $item
    } else {
        # It's a specific file or directory
        $fullPath = Join-Path $projectRoot $item
        if (Test-Path $fullPath -PathType Container) {
            Remove-DirectoryIfExists -path $fullPath
        } else {
            Remove-FilesMatchingPattern -pattern $item
        }
    }
}

# Remove additional files if -All is specified
if ($All) {
    Write-Host "Removing additional files..." -ForegroundColor $InfoColor
    
    foreach ($item in $additionalFilesToRemove) {
        $fullPath = Join-Path $projectRoot $item
        if (Test-Path $fullPath -PathType Container) {
            Remove-DirectoryIfExists -path $fullPath
        } else {
            Remove-FilesMatchingPattern -pattern $item
        }
    }
    
    # Clean NuGet packages
    $nugetPackages = Join-Path $env:USERPROFILE ".nuget\packages"
    if (Test-Path $nugetPackages) {
        Get-ChildItem -Path $nugetPackages -Directory | ForEach-Object {
            Remove-DirectoryIfExists -path $_.FullName
        }
    }
}

Write-Host "`n✅ Project cleaned successfully!" -ForegroundColor $SuccessColor
