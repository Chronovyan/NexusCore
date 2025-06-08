<#
.SYNOPSIS
    Updates project dependencies to their latest versions.
.DESCRIPTION
    This script updates all project dependencies, including system packages,
    Python packages, and vcpkg packages, to their latest versions.
.PARAMETER System
    Update system packages.
.PARAMETER Python
    Update Python packages.
.PARAMETER Vcpkg
    Update vcpkg packages.
.PARAMETER All
    Update all dependencies (default).
.PARAMETER Force
    Force reinstallation of all packages.
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\update-deps.ps1 -All
    Updates all dependencies to their latest versions.
.EXAMPLE
    .\scripts\update-deps.ps1 -Python
    Updates only Python packages.
#>

[CmdletBinding()]
param(
    [switch]$System,
    [switch]$Python,
    [switch]$Vcpkg,
    [switch]$All = $true,
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

# Update system packages
function Update-SystemPackages {
    Write-Host "Updating system packages..." -ForegroundColor $InfoColor
    
    if ($IsWindows) {
        if (Test-CommandExists "choco") {
            $chocoArgs = @("upgrade", "all", "-y")
            if ($Force) { $chocoArgs += "--force" }
            
            $exitCode = Invoke-CommandChecked -Command "choco" -Arguments $chocoArgs -Fatal
            return $exitCode -eq 0
        } else {
            Write-Host "Chocolatey not found. Please install it first." -ForegroundColor $WarningColor
            return $false
        }
    }
    elseif ($IsLinux) {
        if (Test-CommandExists "apt-get") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("apt-get", "update") -Fatal
            if ($exitCode -ne 0) { return $false }
            
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("apt-get", "upgrade", "-y") -Fatal
            return $exitCode -eq 0
        }
        elseif (Test-CommandExists "dnf") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("dnf", "upgrade", "-y") -Fatal
            return $exitCode -eq 0
        }
        elseif (Test-CommandExists "yum") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("yum", "update", "-y") -Fatal
            return $exitCode -eq 0
        }
        elseif (Test-CommandExists "pacman") {
            $exitCode = Invoke-CommandChecked -Command "sudo" -Arguments @("pacman", "-Syu", "--noconfirm") -Fatal
            return $exitCode -eq 0
        }
        else {
            Write-Host "Unsupported Linux distribution. Please update packages manually." -ForegroundColor $ErrorColor
            return $false
        }
    }
    elseif ($IsMacOS) {
        if (Test-CommandExists "brew") {
            $exitCode = Invoke-CommandChecked -Command "brew" -Arguments @("update") -Fatal
            if ($exitCode -ne 0) { return $false }
            
            $exitCode = Invoke-CommandChecked -Command "brew" -Arguments @("upgrade") -Fatal
            return $exitCode -eq 0
        } else {
            Write-Host "Homebrew not found. Please install it first." -ForegroundColor $ErrorColor
            return $false
        }
    }
    else {
        Write-Host "Unsupported operating system. Please update packages manually." -ForegroundColor $ErrorColor
        return $false
    }
}

# Update Python packages
function Update-PythonPackages {
    Write-Host "Updating Python packages..." -ForegroundColor $InfoColor
    
    # Get list of outdated packages
    $outdated = & python -m pip list --outdated --format=json | ConvertFrom-Json
    
    if ($outdated.Count -eq 0) {
        Write-Host "All Python packages are up to date." -ForegroundColor $SuccessColor
        return $true
    }
    
    Write-Host "Found $($outdated.Count) outdated packages:" -ForegroundColor $InfoColor
    $outdated | ForEach-Object { Write-Host "  - $($_.name) ($($_.version) -> $($_.latest_version))" }
    
    # Update packages
    $packagesToUpdate = $outdated | ForEach-Object { $_.name }
    $pipArgs = @("install", "--upgrade") + $packagesToUpdate
    
    if ($Force) { $pipArgs += "--force-reinstall" }
    
    $exitCode = Invoke-CommandChecked -Command "python" -Arguments ("-m", "pip" + $pipArgs) -Fatal
    
    if ($exitCode -eq 0) {
        Write-Host "Successfully updated Python packages." -ForegroundColor $SuccessColor
        return $true
    } else {
        Write-Host "Failed to update some Python packages." -ForegroundColor $WarningColor
        return $false
    }
}

# Update vcpkg packages
function Update-VcpkgPackages {
    Write-Host "Updating vcpkg packages..." -ForegroundColor $InfoColor
    
    if (-not (Test-CommandExists "vcpkg")) {
        Write-Host "vcpkg not found. Please install it first." -ForegroundColor $WarningColor
        return $false
    }
    
    # Update vcpkg itself
    $vcpkgRoot = & vcpkg help | Select-String -Pattern "vcpkg root: (.*)" | ForEach-Object { $_.Matches.Groups[1].Value }
    
    if (-not $vcpkgRoot) {
        Write-Host "Could not determine vcpkg root directory." -ForegroundColor $ErrorColor
        return $false
    }
    
    Push-Location $vcpkgRoot
    
    try {
        # Update vcpkg
        $exitCode = Invoke-CommandChecked -Command "git" -Arguments @("pull") -Fatal
        if ($exitCode -ne 0) { return $false }
        
        # Update ports
        $exitCode = Invoke-CommandChecked -Command "git" -Arguments @("-C", "ports", "pull", "origin", "master") -Fatal
        if ($exitCode -ne 0) { return $false }
        
        # Rebuild vcpkg
        $bootstrapScript = if ($IsWindows) { ".\bootstrap-vcpkg.bat" } else { "./bootstrap-vcpkg.sh" }
        $exitCode = Invoke-CommandChecked -Command $bootstrapScript -Fatal
        if ($exitCode -ne 0) { return $false }
        
        # Update installed packages
        $installed = & vcpkg list --x-json | ConvertFrom-Json
        $packagesToUpdate = $installed.PSObject.Properties | Where-Object { $_.Value.Version -ne $_.Value.VersionFeature } | 
                          Select-Object -ExpandProperty Name | ForEach-Object { $_.Split(':')[0] }
        
        if ($packagesToUpdate.Count -eq 0) {
            Write-Host "All vcpkg packages are up to date." -ForegroundColor $SuccessColor
            return $true
        }
        
        Write-Host "Found $($packagesToUpdate.Count) outdated packages:" -ForegroundColor $InfoColor
        $packagesToUpdate | ForEach-Object { Write-Host "  - $_" }
        
        # Update packages
        foreach ($pkg in $packagesToUpdate) {
            $exitCode = Invoke-CommandChecked -Command "vcpkg" -Arguments @("upgrade", "--no-dry-run", $pkg) -Fatal
            if ($exitCode -ne 0) {
                Write-Host "Failed to update package: $pkg" -ForegroundColor $WarningColor
            }
        }
        
        # Clean up
        $exitCode = Invoke-CommandChecked -Command "vcpkg" -Arguments @("remove", "--outdated", "--recurse") -Fatal
        
        Write-Host "Successfully updated vcpkg packages." -ForegroundColor $SuccessColor
        return $true
    }
    finally {
        Pop-Location
    }
}

# Main script execution
try {
    # Determine which steps to run
    if ($All) {
        $System = $true
        $Python = $true
        $Vcpkg = $true
    }
    
    $success = $true
    
    # Update system packages
    if ($System) {
        $success = $success -and (Update-SystemPackages)
    }
    
    # Update Python packages
    if ($Python) {
        $success = $success -and (Update-PythonPackages)
    }
    
    # Update vcpkg packages
    if ($Vcpkg) {
        $success = $success -and (Update-VcpkgPackages)
    }
    
    if ($success) {
        Write-Host "`nAll updates completed successfully!" -ForegroundColor $SuccessColor
        exit 0
    } else {
        Write-Host "`nSome updates failed. Please check the output above for details." -ForegroundColor $WarningColor
        exit 1
    }
}
catch {
    Write-Host "`nAn error occurred during dependency update:" -ForegroundColor $ErrorColor
    Write-Host $_.Exception.Message -ForegroundColor $ErrorColor
    Write-Host $_.ScriptStackTrace -ForegroundColor $ErrorColor
    exit 1
}
