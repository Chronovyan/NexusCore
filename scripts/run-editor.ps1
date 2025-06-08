<#
.SYNOPSIS
    Runs the editor with various configurations and options.
.DESCRIPTION
    This script provides a convenient way to run the editor with different
    configurations, debug options, and profiling tools.
.PARAMETER Config
    The build configuration to run (Debug, Release, RelWithDebInfo, MinSizeRel).
.PARAMETER BuildDir
    The build directory to use (default: 'build-debug' or 'build-release' based on config).
.PARAMETER Args
    Additional arguments to pass to the editor.
.PARAMETER Gdb
    Run the editor under GDB (Linux/macOS).
.PARAMETER Lldb
    Run the editor under LLDB (macOS).
.PARAMETER Valgrind
    Run the editor under Valgrind (Linux).
.PARAMETER Perf
    Run the editor under Linux perf.
.PARAMETER Debug
    Run the editor in a debugger (Windows: Visual Studio, macOS: LLDB, Linux: GDB).
.PARAMETER Profile
    Enable profiling (if supported by the build).
.PARAMETER Verbose
    Enable verbose output.
.PARAMETER WorkingDir
    Set the working directory for the editor.
.PARAMETER Env
    Set environment variables in KEY=VALUE format (can be specified multiple times).
.EXAMPLE
    .\scripts\run-editor.ps1
    Runs the editor in debug mode.
.EXAMPLE
    .\scripts\run-editor.ps1 -Config Release -Args "--fullscreen"
    Runs the editor in release mode with fullscreen argument.
.EXAMPLE
    .\scripts\run-editor.ps1 -Debug
    Runs the editor in a debugger.
#>

[CmdletBinding(DefaultParameterSetName = 'Default')]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Config = 'Debug',
    
    [string]$BuildDir = "",
    
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args = @(),
    
    [Parameter(ParameterSetName = 'Gdb')]
    [switch]$Gdb,
    
    [Parameter(ParameterSetName = 'Lldb')]
    [switch]$Lldb,
    
    [Parameter(ParameterSetName = 'Valgrind')]
    [switch]$Valgrind,
    
    [Parameter(ParameterSetName = 'Perf')]
    [switch]$Perf,
    
    [Parameter(ParameterSetName = 'Debug')]
    [switch]$Debug,
    
    [switch]$Profile,
    [switch]$Verbose,
    
    [string]$WorkingDir = "",
    
    [string[]]$Env = @()
)

# Error handling
$ErrorActionPreference = "Stop"

# Colors for output
$ErrorColor = "Red"
$WarningColor = "Yellow"
$SuccessColor = "Green"
$InfoColor = "Cyan"

# Determine the build directory if not specified
if ([string]::IsNullOrEmpty($BuildDir)) {
    if ($Config -eq "Release") {
        $BuildDir = "build-release"
    } else {
        $BuildDir = "build-debug"
    }
}

# Get the project root directory
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $projectRoot $BuildDir

# Determine the executable path based on platform and configuration
$executableName = if ($IsWindows) { "editor.exe" } else { "editor" }
$executablePath = Join-Path $buildPath $Config $executableName

# Check if the executable exists
if (-not (Test-Path $executablePath)) {
    # Try without config directory (Ninja generator)
    $executablePath = Join-Path $buildPath $executableName
    
    if (-not (Test-Path $executablePath)) {
        Write-Host "Editor executable not found: $executablePath" -ForegroundColor $ErrorColor
        Write-Host "Please build the project first using .\scripts\build-$($Config.ToLower()).ps1" -ForegroundColor $InfoColor
        exit 1
    }
}

# Set up environment variables
$envVars = @{}

# Add user-specified environment variables
foreach ($envVar in $Env) {
    $parts = $envVar -split '=', 2
    if ($parts.Length -eq 2) {
        $envVars[$parts[0]] = $parts[1]
    }
}

# Set up profiling if requested
if ($Profile) {
    $envVars["EDITOR_PROFILE"] = "1"
}

# Set up working directory
if ([string]::IsNullOrEmpty($WorkingDir)) {
    $WorkingDir = $projectRoot
}

# Function to run a command with environment variables
function Invoke-WithEnvironment {
    param(
        [string]$Command,
        [string[]]$Arguments,
        [hashtable]$Environment,
        [string]$WorkingDirectory
    )
    
    # Set environment variables
    $originalEnv = @{}
    foreach ($key in $Environment.Keys) {
        $originalEnv[$key] = [System.Environment]::GetEnvironmentVariable($key)
        [System.Environment]::SetEnvironmentVariable($key, $Environment[$key])
    }
    
    try {
        # Run the command
        $process = Start-Process -FilePath $Command -ArgumentList $Arguments -NoNewWindow -PassThru -WorkingDirectory $WorkingDirectory
        $process.WaitForExit()
        return $process.ExitCode
    } finally {
        # Restore environment variables
        foreach ($key in $originalEnv.Keys) {
            [System.Environment]::SetEnvironmentVariable($key, $originalEnv[$key])
        }
    }
}

# Build the command to run
$command = $executablePath
$commandArgs = $Args
$useDebugger = $false

# Determine if we should use a debugger
if ($Debug -or $Gdb -or $Lldb -or $Valgrind -or $Perf) {
    $useDebugger = $true
    
    if ($Gdb -or ($IsLinux -and ($Debug -or $Gdb))) {
        # GDB on Linux
        $command = "gdb"
        $commandArgs = @("--args", $executablePath) + $Args
    }
    elseif ($Lldb -or ($IsMacOS -and ($Debug -or $Lldb))) {
        # LLDB on macOS
        $command = "lldb"
        $commandArgs = @("--", $executablePath) + $Args
    }
    elseif ($Valgrind -and $IsLinux) {
        # Valgrind on Linux
        $command = "valgrind"
        $commandArgs = @("--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes", "--verbose", "--log-file=valgrind-out.txt", $executablePath) + $Args
    }
    elseif ($Perf -and $IsLinux) {
        # Linux perf
        $command = "perf"
        $commandArgs = @("record", "-g", "--call-graph=dwarf", "--", $executablePath) + $Args
    }
    elseif ($IsWindows -and $Debug) {
        # Windows debugger
        $vsWhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vsWhere) {
            $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            if ($vsPath) {
                $devenvPath = Join-Path $vsPath "Common7\IDE\devenv.exe"
                if (Test-Path $devenvPath) {
                    $command = $devenvPath
                    $commandArgs = @("/DebugExe", "`"$executablePath`"") + $Args
                }
            }
        }
        
        # Fall back to WinDbg if available
        if (-not (Test-Path $command)) {
            $windbgPath = "${env:ProgramFiles(x86)}\Windows Kits\10\Debuggers\x64\windbg.exe"
            if (Test-Path $windbgPath) {
                $command = $windbgPath
                $commandArgs = @("$executablePath") + $Args
            }
        }
    }
}

# Print command being executed
if ($Verbose) {
    Write-Host "Working Directory: $WorkingDir" -ForegroundColor $InfoColor
    Write-Host "Environment Variables:" -ForegroundColor $InfoColor
    foreach ($key in $envVars.Keys) {
        Write-Host "  $key=$($envVars[$key])" -ForegroundColor $InfoColor
    }
    
    $commandLine = "$command $($commandArgs -join ' ')"
    Write-Host "Executing: $commandLine" -ForegroundColor $InfoColor
}

# Run the command
try {
    $exitCode = Invoke-WithEnvironment -Command $command -Arguments $commandArgs -Environment $envVars -WorkingDirectory $WorkingDir
    
    if ($exitCode -ne 0) {
        Write-Host "Process exited with code $exitCode" -ForegroundColor $ErrorColor
    } else {
        Write-Host "Process completed successfully" -ForegroundColor $SuccessColor
    }
    
    # Post-process perf data if needed
    if ($Perf -and $IsLinux -and (Test-CommandExists "perf")) {
        Write-Host "Generating perf report..." -ForegroundColor $InfoColor
        & perf report -i perf.data
    }
    
    exit $exitCode
} catch {
    Write-Host "Error: $_" -ForegroundColor $ErrorColor
    exit 1
}
