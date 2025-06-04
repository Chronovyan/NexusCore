#!/usr/bin/env pwsh
#
# PowerShell script to install Git hooks
#

# Colors for output
function Write-ColorOutput($ForegroundColor) {
    # Save the current colors
    $fc = $host.UI.RawUI.ForegroundColor
    
    # Set the new colors
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    
    # Execute the script block
    if ($args) {
        Write-Output $args
    }
    
    # Restore the original colors
    $host.UI.RawUI.ForegroundColor = $fc
}

Write-ColorOutput Yellow "Installing Git hooks..."

# Get the directory of this script
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$HOOKS_DIR = Join-Path (git rev-parse --git-dir) "hooks"

# Create hooks directory if it doesn't exist
if (-not (Test-Path $HOOKS_DIR)) {
    New-Item -Path $HOOKS_DIR -ItemType Directory | Out-Null
}

# Copy the PowerShell pre-commit hook
Copy-Item -Path (Join-Path $SCRIPT_DIR "pre-commit.ps1") -Destination (Join-Path $HOOKS_DIR "pre-commit.ps1") -Force

# Create shell wrapper for PowerShell script
$preCommitPath = Join-Path $HOOKS_DIR "pre-commit"
@"
#!/bin/sh
powershell.exe -ExecutionPolicy Bypass -File "$(Join-Path '.git' 'hooks' 'pre-commit.ps1' -Replace '\\', '/')"
exit `$?
"@ | Set-Content -Path $preCommitPath -Encoding ASCII

Write-ColorOutput Green "✅ Successfully installed pre-commit hook to $HOOKS_DIR"
Write-ColorOutput Green "Git hooks installed successfully. Your commits will now be tested automatically." 