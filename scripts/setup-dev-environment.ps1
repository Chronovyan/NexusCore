#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Sets up the development environment for AI-First Text Editor.
.DESCRIPTION
    This script installs the necessary dependencies and sets up the development
    environment for the AI-First Text Editor project on Windows.
#>

# Stop on first error
$ErrorActionPreference = 'Stop'

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Error "This script requires administrator privileges. Please run as administrator."
    exit 1
}

# Check if Chocolatey is installed
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "Installing Chocolatey package manager..." -ForegroundColor Cyan
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
    
    # Refresh PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
}

# Install required tools
Write-Host "Installing required tools..." -ForegroundColor Cyan
choco install -y \
    git \
    cmake \
    ninja \
    llvm \
    python3 \
    visualstudio2022buildtools \
    visualstudio2022-workload-vctools \
    vcpkg

# Refresh PATH after installing tools
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# Install Python packages
Write-Host "Installing Python packages..." -ForegroundColor Cyan
python -m pip install --upgrade pip
pip install conan

# Clone the repository if not already cloned
if (-not (Test-Path ".git")) {
    Write-Host "Cloning repository..." -ForegroundColor Cyan
    git clone --recurse-submodules https://github.com/Chronovyan/TextEditor.git .
} else {
    Write-Host "Updating repository..." -ForegroundColor Cyan
    git submodule update --init --recursive
}

# Set up git hooks
Write-Host "Setting up git hooks..." -ForegroundColor Cyan
Copy-Item -Path ".githooks/*" -Destination ".git/hooks/" -Recurse -Force

# Build the project
Write-Host "Building the project..." -ForegroundColor Cyan
mkdir -p build
cd build
cmake .. -G Ninja
cmake --build .

Write-Host "`n✅ Setup completed successfully!" -ForegroundColor Green
Write-Host "You can now run the editor with: .\build\bin\editor" -ForegroundColor Green
