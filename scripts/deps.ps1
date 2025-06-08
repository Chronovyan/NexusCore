<#
.SYNOPSIS
    Manages project dependencies and versions.
.DESCRIPTION
    This script helps manage project dependencies, including:
    - Installing and updating dependencies
    - Pinning dependency versions
    - Generating dependency reports
    - Checking for outdated dependencies
.PARAMETER Install
    Install all project dependencies.
.PARAMETER Update
    Update all dependencies to their latest versions.
.PARAMETER Check
    Check for outdated dependencies.
.PARAMETER Report
    Generate a dependency report.
.PARAMETER Pin
    Pin dependency versions in the lock file.
.PARAMETER Clean
    Clean unused dependencies.
.PARAMETER NoColor
    Disable colored output.
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\deps.ps1 -Install
    Install all project dependencies.
.EXAMPLE
    .\scripts\deps.ps1 -Update
    Update all dependencies to their latest versions.
.EXAMPLE
    .\scripts\deps.ps1 -Check
    Check for outdated dependencies.
#>

[CmdletBinding()]
param(
    [switch]$Install,
    [switch]$Update,
    [switch]$Check,
    [switch]$Report,
    [switch]$Pin,
    [switch]$Clean,
    [switch]$NoColor,
    [switch]$VerboseOutput
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
$script:VerbosePreference = if ($VerboseOutput) { "Continue" } else { "SilentlyContinue" }

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
    if (-not $NoOutput) {
        Write-Host "$commandLine" -ForegroundColor $InfoColor
    }
    
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

# Function to install system dependencies
function Install-SystemDependencies {
    Write-Host "Installing system dependencies..." -ForegroundColor $InfoColor
    
    if ($IsWindows) {
        # Check if Chocolatey is installed
        if (-not (Test-CommandExists "choco")) {
            Write-Host "Chocolatey is required but not found. Installing Chocolatey..." -ForegroundColor $WarningColor
            
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
        
        # Install required packages
        $packages = @(
            "git",
            "cmake --installargs 'ADD_CMAKE_TO_PATH=System'",
            "ninja",
            "python --version=3.9.0",
            "llvm",
            "doxygen.install",
            "graphviz",
            "nodejs"
        )
        
        foreach ($pkg in $packages) {
            Write-Host "Installing $pkg..." -ForegroundColor $InfoColor
            $exitCode = Invoke-CommandChecked -Command "choco" -Arguments @("install", "-y", $pkg) -Fatal:$false
            if ($exitCode -ne 0) {
                Write-Host "Failed to install $pkg" -ForegroundColor $WarningColor
            }
        }
    }
    elseif ($IsLinux) {
        if (Test-CommandExists "apt-get") {
            # Update package lists
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("apt-get", "update") -Fatal
            if ($exitCode -ne 0) { return $false }
            
            # Install required packages
            $packages = @(
                "build-essential",
                "cmake",
                "ninja-build",
                "python3",
                "python3-pip",
                "clang",
                "llvm",
                "doxygen",
                "graphviz",
                "nodejs",
                "npm"
            )
            
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments (@("apt-get", "install", "-y") + $packages) -Fatal
            return $exitCode -eq 0
        }
        else {
            Write-Host "Unsupported package manager. Please install dependencies manually." -ForegroundColor $ErrorColor
            return $false
        }
    }
    elseif ($IsMacOS) {
        if (Test-CommandExists "brew") {
            # Install required packages
            $packages = @(
                "cmake",
                "ninja",
                "python@3.9",
                "llvm",
                "doxygen",
                "graphviz",
                "node"
            )
            
            $exitCode = Invoke-CommandChecked -Command "brew" -Arguments (@("install") + $packages) -Fatal
            return $exitCode -eq 0
        }
        else {
            Write-Host "Homebrew is required but not found. Please install it from https://brew.sh/" -ForegroundColor $ErrorColor
            return $false
        }
    }
    else {
        Write-Host "Unsupported operating system. Please install dependencies manually." -ForegroundColor $ErrorColor
        return $false
    }
    
    return $true
}

# Function to install Python dependencies
function Install-PythonDependencies {
    Write-Host "Installing Python dependencies..." -ForegroundColor $InfoColor
    
    # Upgrade pip
    $exitCode = Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "install", "--upgrade", "pip") -Fatal
    if ($exitCode -ne 0) { return $false }
    
    # Install required packages
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
        "gcovr"
    )
    
    foreach ($pkg in $packages) {
        Write-Host "Installing $pkg..." -ForegroundColor $InfoColor
        $exitCode = Invoke-CommandChecked -Command "python" -Arguments ("-m", "pip", "install", $pkg) -Fatal:$false
        if ($exitCode -ne 0) {
            Write-Host "Failed to install $pkg" -ForegroundColor $WarningColor
        }
    }
    
    # Install pre-commit hooks
    if (Test-CommandExists "pre-commit") {
        Set-Location $projectRoot
        $exitCode = Invoke-CommandChecked -Command "pre-commit" -Arguments @("install") -Fatal:$false
        if ($exitCode -ne 0) {
            Write-Host "Failed to install pre-commit hooks" -ForegroundColor $WarningColor
        }
    }
    
    return $true
}

# Function to install Node.js dependencies
function Install-NodeDependencies {
    Write-Host "Installing Node.js dependencies..." -ForegroundColor $InfoColor
    
    $packageJson = Join-Path $projectRoot "package.json"
    if (-not (Test-Path $packageJson)) {
        Write-Host "No package.json found. Skipping Node.js dependencies." -ForegroundColor $InfoColor
        return $true
    }
    
    # Check if npm is installed
    if (-not (Test-CommandExists "npm")) {
        Write-Host "npm is not installed. Skipping Node.js dependencies." -ForegroundColor $WarningColor
        return $false
    }
    
    # Install dependencies
    $exitCode = Invoke-CommandChecked -Command "npm" -Arguments @("install") -WorkingDirectory $projectRoot -Fatal:$false
    if ($exitCode -ne 0) {
        Write-Host "Failed to install Node.js dependencies" -ForegroundColor $WarningColor
        return $false
    }
    
    return $true
}

# Function to check for outdated dependencies
function Test-OutdatedDependencies {
    Write-Host "Checking for outdated dependencies..." -ForegroundColor $InfoColor
    
    $hasOutdated = $false
    
    # Check Python packages
    if (Test-CommandExists "pip") {
        Write-Host "`nPython packages:" -ForegroundColor $InfoColor
        $exitCode = Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "list", "--outdated") -Fatal:$false
        if ($exitCode -ne 0) {
            $hasOutdated = $true
        }
    }
    
    # Check Node.js packages
    if (Test-CommandExists "npm") {
        Write-Host "`nNode.js packages:" -ForegroundColor $InfoColor
        $exitCode = Invoke-CommandChecked -Command "npm" -Arguments @("outdated") -WorkingDirectory $projectRoot -Fatal:$false
        if ($exitCode -ne 0) {
            $hasOutdated = $true
        }
    }
    
    if (-not $hasOutdated) {
        Write-Host "All dependencies are up to date." -ForegroundColor $SuccessColor
    }
    
    return $hasOutdated
}

function Get-DependencyReport {
    [CmdletBinding()]
    param()
    
    Write-Host "Generating dependency report..." -ForegroundColor $InfoColor
    
    try {
        # Initialize report with system information
        $reportLines = @()
        $reportLines += "# Dependency Report"
        $reportLines += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
        $reportLines += ""
        $reportLines += "## System Information"
        $reportLines += "- OS: $($PSVersionTable.OS)"
        $reportLines += "- PowerShell Version: $($PSVersionTable.PSVersion)"
        $reportLines += "- CLR Version: $($PSVersionTable.BuildVersion)"
        
        # Add Python package versions
        $reportLines += ""
        $reportLines += "## Python Dependencies"
        
        if (Test-CommandExists "pip") {
            try {
                $pythonVersion = & python --version 2>&1
                $reportLines += ""
                $reportLines += "- Python Version: $pythonVersion"
                
                $packages = & pip list --format=json 2>&1 | ConvertFrom-Json
                $reportLines += ""
                $reportLines += "### Installed Packages"
                $reportLines += ""
                $reportLines += "| Package | Version |"
                $reportLines += "|---------|---------|"
                
                foreach ($pkg in $packages) {
                    $reportLines += "| $($pkg.name) | $($pkg.version) |"
                }
            } catch {
                $reportLines += ""
                $reportLines += "Error getting Python package information: $($_.Exception.Message)"
            }
        } else {
            $reportLines += ""
            $reportLines += "Python/pip is not installed."
        }
        
        # Add Node.js package versions
        $reportLines += ""
        $reportLines += "## Node.js Dependencies"
        $reportLines += ""
        
        if (Test-CommandExists "node") {
            try {
                $nodeVersion = & node --version 2>&1
                $npmVersion = & npm --version 2>&1
                $reportLines += "- Node.js Version: $nodeVersion"
                $reportLines += "- npm Version: $npmVersion"
                
                $packageJsonPath = Join-Path $projectRoot "package.json"
                if (Test-Path $packageJsonPath) {
                    $reportLines += ""
                    $reportLines += "### Installed Packages"
                    $reportLines += ""
                    $reportLines += "```json"
                    $jsonContent = Get-Content -Path $packageJsonPath -Raw
                    $reportLines += $jsonContent
                    $reportLines += '```'
                } else {
                    $reportLines += ""
                    $reportLines += 'No package.json found in project root.'
                }
            } catch {
                $reportLines += ""
                $reportLines += "Error getting Node.js package information: $($_.Exception.Message)"
            }
        } else {
            $reportLines += ""
            $reportLines += "Node.js/npm is not installed."
        }
        
        # Save report to file
        $reportFile = Join-Path $projectRoot "dependencies-report.md"
        try {
            $reportContent = $reportLines -join [System.Environment]::NewLine
            $reportContent | Out-File -FilePath $reportFile -Encoding utf8 -Force
            Write-Host "Dependency report saved to: $reportFile" -ForegroundColor $SuccessColor
            return $true
        } catch {
            Write-Host "Failed to save dependency report: $($_.Exception.Message)" -ForegroundColor $ErrorColor
            return $false
        }
    } catch {
        Write-Host "Error generating dependency report: $($_.Exception.Message)" -ForegroundColor $ErrorColor
        return $false
    }
}

# Function to clean unused dependencies
function Remove-UnusedDependencies {
    Write-Host "Cleaning unused dependencies..." -ForegroundColor $InfoColor
    
    # Clean Python packages
    if (Test-CommandExists "pip") {
        Write-Host "`nCleaning Python packages..." -ForegroundColor $InfoColor
        $exitCode = Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "autoremove", "-y") -Fatal:$false
        if ($exitCode -ne 0) {
            Write-Host "Failed to clean Python packages" -ForegroundColor $WarningColor
        }
    }
    
    # Clean Node.js packages
    if (Test-CommandExists "npm") {
        Write-Host "`nCleaning Node.js packages..." -ForegroundColor $InfoColor
        $exitCode = Invoke-CommandChecked -Command "npm" -Arguments @("prune") -WorkingDirectory $projectRoot -Fatal:$false
        if ($exitCode -ne 0) {
            Write-Host "Failed to clean Node.js packages" -ForegroundColor $WarningColor
        }
    }
    
    Write-Host "Cleanup completed." -ForegroundColor $SuccessColor
    return $true
}

# Main script execution
try {
    # Set up error handling
    $ErrorActionPreference = "Stop"
    
    # Main script execution starts here
    # Determine which action to perform
    if ($Install) {
        $success = $true
        $success = $success -and (Install-SystemDependencies)
        $success = $success -and (Install-PythonDependencies)
        $success = $success -and (Install-NodeDependencies)
        
        if ($success) {
            Write-Host ("`nAll dependencies installed successfully!") -ForegroundColor $SuccessColor
        } else {
            Write-Host ("`nSome dependencies failed to install. See above for details.") -ForegroundColor $ErrorColor
            exit 1
        }
    }
    elseif ($Update) {
        # Implementation for updating dependencies
        Write-Host "Updating dependencies..." -ForegroundColor $InfoColor
        
        if (Test-CommandExists "pip") {
            Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "install", "--upgrade", "pip", "setuptools", "wheel") -Fatal:$false
            Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "install", "--upgrade", "-r", "requirements.txt") -Fatal:$false
        }
        
        if (Test-CommandExists "npm") {
            Invoke-CommandChecked -Command "npm" -Arguments @("update") -WorkingDirectory $projectRoot -Fatal:$false
        }
        
        Write-Host "`nDependencies have been updated." -ForegroundColor $SuccessColor
    }
    elseif ($Check) {
        $hasOutdated = Test-OutdatedDependencies
        if ($hasOutdated) {
            exit 1
        }
    }
    elseif ($Report) {
        Get-DependencyReport
    }
    elseif ($Pin) {
        # Implementation for pinning dependency versions
        Write-Host "Pinning dependency versions..." -ForegroundColor $InfoColor
        
        # Create or update requirements.txt
        if (Test-CommandExists "pip") {
            Invoke-CommandChecked -Command "python" -Arguments @("-m", "pip", "freeze", ">", "requirements.txt") -WorkingDirectory $projectRoot -Fatal:$false
            Write-Host "Python dependencies pinned to requirements.txt" -ForegroundColor $SuccessColor
        }
        
        # Update package-lock.json
        if (Test-CommandExists "npm") {
            Invoke-CommandChecked -Command "npm" -Arguments @("install", "--package-lock-only") -WorkingDirectory $projectRoot -Fatal:$false
            Write-Host "Node.js dependencies pinned to package-lock.json" -ForegroundColor $SuccessColor
        }
    }
    elseif ($Clean) {
        Remove-UnusedDependencies
    }
    else {
        # Default action: show help
        Write-Host "Usage: .\scripts\deps.ps1 [options]" -ForegroundColor $InfoColor
        Write-Host "Options:" -ForegroundColor $InfoColor
        Write-Host "  -Install      Install all project dependencies" -ForegroundColor $InfoColor
        Write-Host "  -Update       Update all dependencies to their latest versions" -ForegroundColor $InfoColor
        Write-Host "  -Check        Check for outdated dependencies" -ForegroundColor $InfoColor
        Write-Host "  -Report       Generate a dependency report" -ForegroundColor $InfoColor
        Write-Host "  -Pin          Pin dependency versions in the lock file" -ForegroundColor $InfoColor
        Write-Host "  -Clean        Clean unused dependencies" -ForegroundColor $InfoColor
        Write-Host "  -NoColor      Disable colored output" -ForegroundColor $InfoColor
        Write-Host "  -Verbose      Show detailed output" -ForegroundColor $InfoColor
        exit 0
    }
}
catch {
    Write-Host -Object ("`nAn error occurred:") -ForegroundColor $ErrorColor
    Write-Host -Object ($_.Exception.Message) -ForegroundColor $ErrorColor
    Write-Host -Object ($_.ScriptStackTrace) -ForegroundColor $ErrorColor
    exit 1
}
