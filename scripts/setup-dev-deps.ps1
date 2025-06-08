<#
.SYNOPSIS
    Installs development dependencies for Windows
.DESCRIPTION
    This script installs the necessary development tools and dependencies for
    building and developing the AI-First Text Editor on Windows.
.PARAMETER InstallVisualStudio
    If specified, installs Visual Studio Build Tools if not already installed.
.PARAMETER InstallVcpkg
    If specified, installs vcpkg if not already installed.
.PARAMETER InstallLLVM
    If specified, installs LLVM if not already installed.
.PARAMETER InstallGit
    If specified, installs Git if not already installed.
.PARAMETER InstallPython
    If specified, installs Python if not already installed.
.EXAMPLE
    .\scripts\setup-dev-deps.ps1 -InstallAll
    Installs all development dependencies.
#>

[CmdletBinding()]
param(
    [switch]$InstallVisualStudio,
    [switch]$InstallVcpkg,
    [switch]$InstallLLVM,
    [switch]$InstallGit,
    [switch]$InstallPython,
    [switch]$InstallAll
)

# Error handling
$ErrorActionPreference = "Stop"

# Enable TLS 1.2 for all web requests
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

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

# Function to check if a program is installed
function Test-ProgramInstalled {
    param($programName)
    $installed = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | 
                 Where-Object { $_.DisplayName -like "*$programName*" }) -ne $null
    return $installed
}

# Function to install Chocolatey
function Install-Chocolatey {
    if (-not (Test-CommandExists choco)) {
        Write-Host "Installing Chocolatey package manager..." -ForegroundColor $InfoColor
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
        
        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    } else {
        Write-Host "Chocolatey is already installed." -ForegroundColor $SuccessColor
    }
}

# Function to install a program using Chocolatey
function Install-WithChocolatey {
    param(
        [string]$packageName,
        [string]$displayName,
        [switch]$force
    )
    
    if ($force -or -not (Test-ProgramInstalled $displayName)) {
        Write-Host "Installing $displayName..." -ForegroundColor $InfoColor
        choco install $packageName -y
    } else {
        Write-Host "$displayName is already installed." -ForegroundColor $SuccessColor
    }
}

# Main script execution
Write-Host "Setting up development environment for Windows..." -ForegroundColor $InfoColor

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "This script requires administrator privileges. Please run as administrator." -ForegroundColor $ErrorColor
    exit 1
}

# Install Chocolatey if needed
Install-Chocolatey

# Install Git if needed
if ($InstallGit -or $InstallAll) {
    Install-WithChocolatey -packageName "git.install" -displayName "Git"
}

# Install Python if needed
if ($InstallPython -or $InstallAll) {
    Install-WithChocolatey -packageName "python" -displayName "Python"
    
    # Add Python to PATH if not already there
    $pythonPath = "$env:LocalAppData\Programs\Python\Python39"
    if ($env:Path -notlike "*$pythonPath*") {
        [System.Environment]::SetEnvironmentVariable("Path", "$env:Path;$pythonPath", [System.EnvironmentVariableTarget]::User)
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }
    
    # Install Python packages
    Write-Host "Installing Python packages..." -ForegroundColor $InfoColor
    python -m pip install --upgrade pip
    pip install conan cmake-format pre-commit
}

# Install Visual Studio Build Tools if needed
if ($InstallVisualStudio -or $InstallAll) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (-not (Test-Path $vsWhere)) {
        Write-Host "Installing Visual Studio Build Tools..." -ForegroundColor $InfoColor
        $vsBuildToolsUrl = "https://aka.ms/vs/17/release/vs_buildtools.exe"
        $vsInstaller = "$env:TEMP\vs_buildtools.exe"
        
        # Download Visual Studio Build Tools installer
        Invoke-WebRequest -Uri $vsBuildToolsUrl -OutFile $vsInstaller
        
        # Install with required components
        Start-Process -FilePath $vsInstaller -ArgumentList @(
            "--add Microsoft.VisualStudio.Workload.VCTools",
            "--add Microsoft.VisualStudio.Component.Windows10SDK",
            "--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "--add Microsoft.VisualStudio.Component.VC.CMake.Project",
            "--add Microsoft.VisualStudio.Component.Windows10SDK.19041",
            "--passive",
            "--norestart",
            "--wait"
        ) -Wait -NoNewWindow
        
        # Clean up
        Remove-Item $vsInstaller -Force
    } else {
        Write-Host "Visual Studio Build Tools is already installed." -ForegroundColor $SuccessColor
    }
}

# Install LLVM if needed
if ($InstallLLVM -or $InstallAll) {
    Install-WithChocolatey -packageName "llvm" -displayName "LLVM"
    
    # Add LLVM to PATH if not already there
    $llvmPath = "C:\Program Files\LLVM\bin"
    if ($env:Path -notlike "*$llvmPath*") {
        [System.Environment]::SetEnvironmentVariable("Path", "$env:Path;$llvmPath", [System.EnvironmentVariableTarget]::Machine)
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }
}

# Install vcpkg if needed
if ($InstallVcpkg -or $InstallAll) {
    $vcpkgDir = "$env:USERPROFILE\vcpkg"
    
    if (-not (Test-Path $vcpkgDir)) {
        Write-Host "Installing vcpkg..." -ForegroundColor $InfoColor
        
        # Clone vcpkg
        git clone https://github.com/Microsoft/vcpkg.git $vcpkgDir
        
        # Bootstrap vcpkg
        Push-Location $vcpkgDir
        .\bootstrap-vcpkg.bat
        
        # Add vcpkg to PATH
        [System.Environment]::SetEnvironmentVariable("Path", "$env:Path;$vcpkgDir", [System.EnvironmentVariableTarget]::User)
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
        
        Pop-Location
        
        # Install common dependencies
        Write-Host "Installing common dependencies with vcpkg..." -ForegroundColor $InfoColor
        vcpkg install glfw3 imgui[glfw-binding,opengl3-binding] stb fmt spdlog nlohmann-json cpr doctest catch2
    } else {
        Write-Host "vcpkg is already installed at $vcpkgDir" -ForegroundColor $SuccessColor
    }
}

# Setup pre-commit hooks
if (Test-CommandExists git) {
    Write-Host "Setting up pre-commit hooks..." -ForegroundColor $InfoColor
    git config core.hooksPath .githooks
    
    if (Test-CommandExists pre-commit) {
        pre-commit install
    } else {
        Write-Host "pre-commit not found. Please install it with 'pip install pre-commit'" -ForegroundColor $WarningColor
    }
}

Write-Host "`n✅ Development environment setup complete!" -ForegroundColor $SuccessColor
Write-Host "You can now build the project with the following commands:" -ForegroundColor $InfoColor
Write-Host "  .\scripts\build-debug.ps1" -ForegroundColor $InfoColor
Write-Host "  .\scripts\build-release.ps1" -ForegroundColor $InfoColor
