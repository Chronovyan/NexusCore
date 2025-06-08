<#
.SYNOPSIS
    Sets up the development environment and project dependencies.
.DESCRIPTION
    This script automates the setup of the development environment, including
    installing system dependencies, Python packages, and configuring Git hooks.
.PARAMETER InstallDeps
    Install system dependencies (requires administrator privileges).
.PARAMETER PythonDeps
    Install Python dependencies.
.PARAMETER GitHooks
    Set up Git hooks.
.PARAMETER VSCode
    Install recommended VSCode extensions.
.PARAMETER All
    Perform all setup steps.
.PARAMETER Force
    Force reinstallation of dependencies.
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\setup-project.ps1 -All
    Performs a complete setup of the development environment.
.EXAMPLE
    .\scripts\setup-project.ps1 -InstallDeps -PythonDeps
    Installs only system and Python dependencies.
#>

[CmdletBinding()]
param(
    [switch]$InstallDeps,
    [switch]$PythonDeps,
    [switch]$GitHooks,
    [switch]$VSCode,
    [switch]$All,
    [switch]$Force,
    [switch]$Verbose
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
$script:VerbosePreference = if ($Verbose) { "Continue" } else { "SilentlyContinue" }

# Function to check if a command exists
function Test-CommandExists {
    param([string]$command)
    $exists = $null -ne (Get-Command $command -ErrorAction SilentlyContinue)
    Write-Verbose "Command '$command' exists: $exists"
    return $exists
}

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

# Function to install a package using the system package manager
function Install-Package {
    param(
        [string]$Name,
        [string]$WindowsPackage,
        [string]$LinuxPackage = $Name,
        [string]$MacPackage = $Name,
        [string]$TestCommand = $Name,
        [string[]]$AdditionalArgs = @()
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
            $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
            
            if (-not (Test-CommandExists "choco")) {
                Write-Host "Failed to install Chocolatey. Please install it manually from https://chocolatey.org/" -ForegroundColor $ErrorColor
                return $false
            }
        }
        
        $chocoArgs = @("install", $WindowsPackage, "-y")
        if ($Force) { $chocoArgs += "--force" }
        $chocoArgs += $AdditionalArgs
        
        $exitCode = Invoke-CommandChecked -Command "choco" -Arguments $chocoArgs -Fatal
        return $exitCode -eq 0
    }
    elseif ($IsLinux) {
        if (Test-CommandExists "apt-get") {
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

# Function to setup Git hooks
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
./scripts/format-code.ps1 --check
if [ `$? -ne 0 ]; then
    echo "Code formatting issues found. Run './scripts/format-code.ps1' to fix them."
    exit 1
fi

# Run static analysis
echo "Running static analysis..."
./scripts/analyze-code.ps1
if [ `$? -ne 0 ]; then
    echo "Static analysis found issues. Please fix them before committing."
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

# Function to install VSCode extensions
function Install-VSCodeExtensions {
    $extensions = @(
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "ms-python.python",
        "twxs.cmake",
        "xaver.clang-format",
        "yzhang.markdown-all-in-one",
        "ms-azuretools.vscode-docker",
        "ms-vscode.powershell",
        "ms-vsliveshare.vsliveshare",
        "eamodio.gitlens",
        "gruntfuggly.todo-tree",
        "streetsidesoftware.code-spell-checker",
        "ms-vscode-remote.remote-containers",
        "ms-vscode-remote.remote-wsl",
        "ms-vscode.test-adapter-converter",
        "matepek.vscode-catch2-test-adapter"
    )
    
    if (-not (Test-CommandExists "code")) {
        Write-Host "VSCode CLI ('code' command) not found. Please add it to your PATH." -ForegroundColor $WarningColor
        return
    }
    
    Write-Host "Installing VSCode extensions..." -ForegroundColor $InfoColor
    
    foreach ($extension in $extensions) {
        Write-Host "  Installing $extension..." -NoNewline
        $output = & code --install-extension $extension 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host " [OK]" -ForegroundColor $SuccessColor
        } else {
            Write-Host " [FAILED]" -ForegroundColor $ErrorColor
            Write-Host "  $output" -ForegroundColor $ErrorColor
        }
    }
}

# Main script execution
try {
    # Determine which steps to run
    if ($All) {
        $InstallDeps = $true
        $PythonDeps = $true
        $GitHooks = $true
        $VSCode = $true
    }
    
    # Install system dependencies
    if ($InstallDeps) {
        Write-Host "Installing system dependencies..." -ForegroundColor $InfoColor
        
        # Common build tools
        $packages = @(
            @{ Name = "Git"; WindowsPackage = "git"; TestCommand = "git" },
            @{ Name = "CMake"; WindowsPackage = "cmake --installargs 'ADD_CMAKE_TO_PATH=System'"; TestCommand = "cmake" },
            @{ Name = "Ninja"; WindowsPackage = "ninja"; TestCommand = "ninja" },
            @{ Name = "Python 3"; WindowsPackage = "python --version=3.9.0"; TestCommand = "python --version" },
            @{ Name = "LLVM"; WindowsPackage = "llvm"; TestCommand = "clang" },
            @{ Name = "Doxygen"; WindowsPackage = "doxygen.install"; TestCommand = "doxygen --version" },
            @{ Name = "Graphviz"; WindowsPackage = "graphviz"; TestCommand = "dot -V" }
        )
        
        foreach ($pkg in $packages) {
            Install-Package @pkg
        }
        
        # Install vcpkg
        if (-not (Test-CommandExists "vcpkg")) {
            Write-Host "Installing vcpkg..." -ForegroundColor $InfoColor
            $vcpkgDir = Join-Path $env:USERPROFILE "vcpkg"
            
            if (-not (Test-Path $vcpkgDir)) {
                git clone https://github.com/Microsoft/vcpkg.git $vcpkgDir
            }
            
            & "$vcpkgDir\bootstrap-vcpkg.bat"
            
            # Add vcpkg to PATH
            $vcpkgPath = "$vcpkgDir"
            $path = [System.Environment]::GetEnvironmentVariable("Path", "User")
            
            if ($path -notlike "*$vcpkgPath*") {
                [System.Environment]::SetEnvironmentVariable("Path", "$path;$vcpkgPath", "User")
                $env:Path += ";$vcpkgPath"
            }
        }
    }
    
    # Install Python dependencies
    if ($PythonDeps) {
        Write-Host "Installing Python dependencies..." -ForegroundColor $InfoColor
        
        $pythonPackages = @(
            "pip",
            "setuptools",
            "wheel",
            "conan",
            "pre-commit",
            "clang-format",
            "black",
            "mypy",
            "pylint",
            "pytest",
            "pytest-cov",
            "sphinx",
            "sphinx-rtd-theme",
            "breathe",
            "cmake-format",
            "cmakelint",
            "gcovr"
        )
        
        # Upgrade pip first
        Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "install", "--upgrade", "pip") -Fatal
        
        # Install packages
        foreach ($pkg in $pythonPackages) {
            Install-PythonPackage -Package $pkg
        }
        
        # Install pre-commit hooks
        if (Test-CommandExists "pre-commit") {
            Set-Location $projectRoot
            & pre-commit install
        }
    }
    
    # Setup Git hooks
    if ($GitHooks) {
        Set-GitHooks
    }
    
    # Install VSCode extensions
    if ($VSCode) {
        Install-VSCodeExtensions
    }
    
    Write-Host "`nSetup completed successfully!" -ForegroundColor $SuccessColor
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
