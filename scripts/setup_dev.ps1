# Setup script for AI-First Text Editor development environment
# This script helps set up the development environment on Windows

# Stop on first error
$ErrorActionPreference = "Stop"

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "This script requires administrator privileges. Please run as administrator." -ForegroundColor Red
    exit 1
}

# Check if Chocolatey is installed
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "Installing Chocolatey..." -ForegroundColor Cyan
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    
    # Refresh environment variables
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
}

# Install required tools
$tools = @(
    "git",
    "cmake --version=3.25.0",
    "visualstudio2022buildtools",
    "visualstudio2022-workload-vctools",
    "python --version=3.10.0",
    "llvm",
    "ninja",
    "vcpkg",
    "conan",
    "7zip"
)

foreach ($tool in $tools) {
    Write-Host "Installing/Updating $tool..." -ForegroundColor Cyan
    choco install $tool -y --no-progress
}

# Install Python packages
$pythonPackages = @(
    "conan",
    "cmake-format",
    "clang-format",
    "clang-tools",
    "black",
    "isort",
    "mypy",
    "pylint",
    "pytest",
    "pytest-cov",
    "sphinx",
    "sphinx-rtd-theme",
    "breathe",
    "cmakelang"
)

Write-Host "Installing Python packages..." -ForegroundColor Cyan
pip install -U $pythonPackages

# Clone the repository if not already cloned
if (-not (Test-Path .git)) {
    $repoUrl = Read-Host "Enter the Git repository URL (or press Enter to skip)"
    if ($repoUrl) {
        $parentDir = Split-Path -Parent $PSScriptRoot
        Set-Location $parentDir
        git clone $repoUrl
        Set-Location (Split-Path -Leaf $repoUrl -ErrorAction SilentlyContinue)
    }
}

# Initialize git submodules
if (Test-Path .gitmodules) {
    Write-Host "Initializing git submodules..." -ForegroundColor Cyan
    git submodule update --init --recursive
}

# Set up vcpkg
if (-not (Test-Path vcpkg)) {
    Write-Host "Setting up vcpkg..." -ForegroundColor Cyan
    git clone https://github.com/Microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat
}

# Configure the project
Write-Host "Configuring the project..." -ForegroundColor Cyan
$buildDir = "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}
Set-Location $buildDir

$cmakeArgs = @(
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake",
    "-DENABLE_TESTS=ON",
    "-DENABLE_COVERAGE=ON",
    "-DENABLE_DOCS=ON"
)

cmake .. @cmakeArgs

# Build the project
Write-Host "Building the project..." -ForegroundColor Cyan
cmake --build . --config Debug

# Run tests
Write-Host "Running tests..." -ForegroundColor Cyan
ctest -C Debug --output-on-failure

# Set up pre-commit hooks if .pre-commit-config.yaml exists
if (Test-Path "../.pre-commit-config.yaml") {
    Write-Host "Setting up pre-commit hooks..." -ForegroundColor Cyan
    pip install pre-commit
    pre-commit install
}

Write-Host "Setup completed successfully!" -ForegroundColor Green
Write-Host "You can now build the project by running: cmake --build $buildDir" -ForegroundColor Green
