<#
.SYNOPSIS
    Sets up the development environment for the AI-First Text Editor.
.DESCRIPTION
    This script automates the setup of the development environment, including:
    - Installing system dependencies (compilers, build tools, etc.)
    - Setting up Python virtual environment
    - Installing Python dependencies
    - Configuring Git hooks
    - Setting up pre-commit hooks
    - Configuring VSCode (if installed)
.PARAMETER All
    Perform all setup steps.
.PARAMETER System
    Install system dependencies.
.PARAMETER Python
    Set up Python environment and install dependencies.
.PARAMETER GitHooks
    Set up Git hooks.
.PARAMETER VSCode
    Configure VSCode settings and extensions.
.PARAMETER Force
    Force reinstallation of dependencies.
.PARAMETER NoColor
    Disable colored output.
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\setup-dev.ps1 -All
    Performs a complete setup of the development environment.
.EXAMPLE
    .\scripts\setup-dev.ps1 -Python -GitHooks
    Sets up only the Python environment and Git hooks.
#>

[CmdletBinding()]
param(
    [switch]$All,
    [switch]$System,
    [switch]$Python,
    [switch]$GitHooks,
    [switch]$VSCode,
    [switch]$Force,
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

# Function to check if a command exists
function Test-CommandExists {
    param([string]$command)
    
    $exists = $null -ne (Get-Command $command -ErrorAction SilentlyContinue)
    Write-Verbose "Command '$command' exists: $exists"
    return $exists
}

# Function to install a package using the system package manager
function Install-SystemPackage {
    param(
        [string]$Name,
        [string]$WindowsPackage,
        [string]$LinuxPackage = $Name,
        [string]$MacPackage = $Name,
        [string]$TestCommand = $Name
    )
    
    if (Test-CommandExists $TestCommand) {
        Write-Host "$Name is already installed." -ForegroundColor $SuccessColor
        return $true
    }
    
    Write-Host "Installing $Name..." -ForegroundColor $InfoColor
    
    if ($IsWindows) {
        if (-not (Test-CommandExists "choco")) {
            Write-Host "Chocolatey package manager is required but not found. Installing Chocolatey..." -ForegroundColor $WarningColor
            
            # Install Chocolatey
            Set-ExecutionPolicy Bypass -Scope Process -Force
            [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
            Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
            
            # Refresh PATH
            $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
            
            if (-not (Test-CommandExists "choco")) {
                Write-Host "Failed to install Chocolatey. Please install it manually from https://chocolatey.org/" -ForegroundColor $ErrorColor
                return $false
            }
        }
        
        $chocoArgs = @("install", $WindowsPackage, "-y")
        if ($Force) { $chocoArgs += "--force" }
        
        $exitCode = Invoke-CommandChecked -Command "choco" -Arguments $chocoArgs -Fatal
        return $exitCode -eq 0
    }
    elseif ($IsLinux) {
        if (Test-CommandExists "apt-get") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("apt-get", "update") -Fatal
            if ($exitCode -ne 0) { return $false }
            
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("apt-get", "install", "-y", $LinuxPackage) -Fatal
            return $exitCode -eq 0
        }
        elseif (Test-CommandExists "dnf") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("dnf", "install", "-y", $LinuxPackage) -Fatal
            return $exitCode -eq 0
        }
        elseif (Test-CommandExists "yum") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("yum", "install", "-y", $LinuxPackage) -Fatal
            return $exitCode -eq 0
        }
        elseif (Test-CommandExists "pacman") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("pacman", "-S", "--noconfirm", $LinuxPackage) -Fatal
            return $exitCode -eq 0
        }
        else {
            Write-Host "Unsupported Linux distribution. Please install $Name manually." -ForegroundColor $ErrorColor
            return $false
        }
    }
    elseif ($IsMacOS) {
        if (Test-CommandExists "brew") {
            $exitCode = Invoke-CommandChecked -Command "brew" -Arguments @("install", $MacPackage) -Fatal
            return $exitCode -eq 0
        }
        else {
            Write-Host "Homebrew is required but not found. Please install it from https://brew.sh/" -ForegroundColor $ErrorColor
            return $false
        }
    }
    else {
        Write-Host "Unsupported operating system. Please install $Name manually." -ForegroundColor $ErrorColor
        return $false
    }
}

# Function to install Python package
function Install-PythonPackage {
    param(
        [string]$Package,
        [string]$Version = "",
        [string]$ExtraIndex = ""
    )
    
    $packageSpec = $Package
    if ($Version) {
        $packageSpec = "$Package==$Version"
    }
    
    Write-Host "Installing Python package: $packageSpec" -ForegroundColor $InfoColor
    
    $pipArgs = @("install", "--upgrade", $packageSpec)
    if ($Force) { $pipArgs += "--force-reinstall" }
    if ($ExtraIndex) { $pipArgs += "--extra-index-url" + $ExtraIndex }
    
    $exitCode = Invoke-CommandChecked -Command "python" -Arguments ("-m", "pip" + $pipArgs) -Fatal
    return $exitCode -eq 0
}

# Function to set up Git hooks
function Set-GitHooks {
    Write-Host "Setting up Git hooks..." -ForegroundColor $InfoColor
    
    $hooksDir = Join-Path $projectRoot ".git" "hooks"
    $preCommitHook = Join-Path $hooksDir "pre-commit"
    
    if (-not (Test-Path $hooksDir)) {
        New-Item -ItemType Directory -Path $hooksDir -Force | Out-Null
    }
    
    # Create pre-commit hook
    @"
#!/bin/sh
# Auto-generated pre-commit hook for AI-First Text Editor

# Run code formatter
echo "Running code formatter..."
./scripts/format-and-lint.ps1 -Check
if [ `$? -ne 0 ]; then
    echo "Code formatting issues found. Run './scripts/format-and-lint.ps1' to fix them."
    exit 1
fi

# Run tests
echo "Running tests..."
./scripts/run-tests.ps1
if [ `$? -ne 0 ]; then
    echo "Tests failed. Please fix the issues before committing."
    exit 1
fi

echo "All checks passed!"
exit 0
"@ | Out-File -FilePath $preCommitHook -Encoding ASCII -Force
    
    # Make the hook executable
    if ($IsWindows) {
        icacls $preCommitHook /grant "%USERNAME%:RX" | Out-Null
    } else {
        chmod +x $preCommitHook
    }
    
    Write-Host "Git hooks have been set up successfully." -ForegroundColor $SuccessColor
}

# Function to configure VSCode
function Set-VSCodeSettings {
    Write-Host "Configuring VSCode..." -ForegroundColor $InfoColor
    
    $vscodeDir = Join-Path $projectRoot ".vscode"
    $settingsFile = Join-Path $vscodeDir "settings.json"
    
    # Create .vscode directory if it doesn't exist
    if (-not (Test-Path $vscodeDir)) {
        New-Item -ItemType Directory -Path $vscodeDir -Force | Out-Null
    }
    
    # Create or update settings.json
    $settings = @{
        "editor.formatOnSave" = $true
        "editor.codeActionsOnSave" = @{
            "source.organizeImports" = true
            "source.fixAll" = true
        }
        "editor.defaultFormatter" = "ms-vscode.cpptools"
        "[cpp]" = @{
            "editor.defaultFormatter" = "ms-vscode.cpptools"
        }
        "[python]" = @{
            "editor.defaultFormatter" = "ms-python.python"
        }
        "python.formatting.provider" = "black"
        "python.linting.enabled" = true
        "python.linting.pylintEnabled" = true
        "python.linting.mypyEnabled" = true
        "python.testing.pytestEnabled" = true
        "python.testing.unittestEnabled" = false
        "python.testing.nosetestsEnabled" = false
        "python.testing.pytestArgs" = @(
            "-v",
            "--cov=.",
            "--cov-report=html"
        )
        "files.exclude" = @{
            "**/.git" = true
            "**/.svn" = true
            "**/.hg" = true
            "**/CVS" = true
            "**/.DS_Store" = true
            "**/__pycache__" = true
            "**/*.pyc" = true
            "**/*.pyo" = true
            "**/*.pyd" = true
            "**/build" = true
            "**/dist" = true
        }
    }
    
    # Convert to JSON and write to file
    $settingsJson = $settings | ConvertTo-Json -Depth 10
    $settingsJson | Out-File -FilePath $settingsFile -Encoding utf8 -Force
    
    Write-Host "VSCode settings have been configured successfully." -ForegroundColor $SuccessColor
    
    # Install recommended extensions
    $extensions = @(
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "ms-python.python",
        "twxs.cmake",
        "yzhang.markdown-all-in-one",
        "ms-azuretools.vscode-docker",
        "ms-vscode.powershell",
        "eamodio.gitlens",
        "streetsidesoftware.code-spell-checker"
    )
    
    if (Test-CommandExists "code") {
        foreach ($extension in $extensions) {
            Write-Host "Installing VSCode extension: $extension" -ForegroundColor $InfoColor
            & code --install-extension $extension --force
        }
    } else {
        Write-Host "VSCode CLI ('code' command) not found. Please install VSCode extensions manually." -ForegroundColor $WarningColor
    }
}

# Main script execution
try {
    # Determine which steps to run
    if ($All) {
        $System = $true
        $Python = $true
        $GitHooks = $true
        $VSCode = $true
    }
    
    # Install system dependencies
    if ($System) {
        Write-Host "Installing system dependencies..." -ForegroundColor $InfoColor
        
        $packages = @(
            @{ Name = "Git"; WindowsPackage = "git"; TestCommand = "git" },
            @{ Name = "CMake"; WindowsPackage = "cmake --installargs 'ADD_CMAKE_TO_PATH=System'"; TestCommand = "cmake" },
            @{ Name = "Ninja"; WindowsPackage = "ninja"; TestCommand = "ninja" },
            @{ Name = "Python 3"; WindowsPackage = "python --version=3.9.0"; TestCommand = "python --version" },
            @{ Name = "LLVM"; WindowsPackage = "llvm"; TestCommand = "clang-format" },
            @{ Name = "Doxygen"; WindowsPackage = "doxygen.install"; TestCommand = "doxygen --version" },
            @{ Name = "Graphviz"; WindowsPackage = "graphviz"; TestCommand = "dot -V" },
            @{ Name = "Node.js"; WindowsPackage = "nodejs"; TestCommand = "node --version" }
        )
        
        foreach ($pkg in $packages) {
            Install-SystemPackage @pkg
        }
    }
    
    # Set up Python environment
    if ($Python) {
        Write-Host "Setting up Python environment..." -ForegroundColor $InfoColor
        
        # Upgrade pip
        Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "install", "--upgrade", "pip") -Fatal
        
        # Install Python packages
        $packages = @(
            "setuptools",
            "wheel",
            "conan",
            "pre-commit",
            "clang-format",
            "black",
            "isort",
            "mypy",
            "pylint",
            "pytest",
            "pytest-cov",
            "sphinx",
            "sphinx-rtd-theme",
            "breathe",
            "cmake-format",
            "cmakelint",
            "gcovr",
            "pre-commit"
        )
        
        foreach ($pkg in $packages) {
            Install-PythonPackage -Package $pkg
        }
        
        # Install pre-commit hooks
        if (Test-CommandExists "pre-commit") {
            Set-Location $projectRoot
            & pre-commit install
        }
    }
    
    # Set up Git hooks
    if ($GitHooks) {
        Set-GitHooks
    }
    
    # Configure VSCode
    if ($VSCode) {
        Set-VSCodeSettings
    }
    
    Write-Host "`nDevelopment environment setup completed successfully!" -ForegroundColor $SuccessColor
    Write-Host "Next steps:" -ForegroundColor $InfoColor
    Write-Host "1. Build the project: .\scripts\build-debug.ps1"
    Write-Host "2. Run tests: .\scripts\run-tests.ps1"
    Write-Host "3. Start coding!"
}
catch {
    Write-Host "`nAn error occurred during setup:" -ForegroundColor $ErrorColor
    Write-Host $_.Exception.Message -ForegroundColor $ErrorColor
    Write-Host $_.ScriptStackTrace -ForegroundColor $ErrorColor
    exit 1
}
