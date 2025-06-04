# PowerShell script to install advanced cross-platform Git hooks

# Colors for output
$Red = [System.Console]::ForegroundColor = "Red"
$Green = [System.Console]::ForegroundColor = "Green"
$Yellow = [System.Console]::ForegroundColor = "Yellow"
$Reset = [System.Console]::ResetColor()

Write-Host "Installing advanced cross-platform pre-commit hooks..." -ForegroundColor Yellow

# Get the root directory of the git repository
$RepoRoot = git rev-parse --show-toplevel

# Git hooks directory
$HooksDir = "$(git rev-parse --git-dir)/hooks"
if (-not (Test-Path $HooksDir)) {
    New-Item -ItemType Directory -Path $HooksDir -Force | Out-Null
    Write-Host "Created hooks directory: $HooksDir"
}

# Copy the hook files
Copy-Item -Path "$RepoRoot/hooks/ps1/advanced-pre-commit.ps1" -Destination "$HooksDir/" -Force
Copy-Item -Path "$RepoRoot/hooks/sh/advanced-pre-commit.sh" -Destination "$HooksDir/" -Force
Copy-Item -Path "$RepoRoot/hooks/sh/advanced-pre-commit-wrapper" -Destination "$HooksDir/" -Force

# Set the wrapper as the pre-commit hook
Copy-Item -Path "$HooksDir/advanced-pre-commit-wrapper" -Destination "$HooksDir/pre-commit" -Force

Write-Host "✅ Advanced pre-commit hooks installed successfully." -ForegroundColor Green
Write-Host "The hooks will automatically use:" -ForegroundColor Yellow
Write-Host "  - PowerShell script on Windows"
Write-Host "  - Bash script on Linux/macOS"
Write-Host "  - PowerShell Core (if available) on any platform"
Write-Host ""
Write-Host "To bypass these hooks for a single commit (not recommended):" -ForegroundColor Yellow
Write-Host "  git commit --no-verify -m `"Your commit message`"" 