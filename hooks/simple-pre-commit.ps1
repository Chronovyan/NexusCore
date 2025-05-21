#!/usr/bin/env pwsh
#
# Very simple pre-commit hook for PowerShell (Windows)

Write-Host "Running simple pre-commit hook..." -ForegroundColor Yellow

# Store the current directory
$REPO_ROOT = (git rev-parse --show-toplevel)
$CURR_DIR = Get-Location

# Check if OpenAI_API_Client.cpp was modified
$modified_files = git diff --cached --name-only
$api_client_modified = $modified_files -match "OpenAI_API_Client.cpp"

if ($api_client_modified) {
    Write-Host "OpenAI_API_Client.cpp was modified - checking for stub implementation..." -ForegroundColor Yellow
    
    $api_client_path = Join-Path -Path $REPO_ROOT -ChildPath "src\OpenAI_API_Client.cpp"
    $content = Get-Content -Path $api_client_path -Raw
    
    if ($content -match "not fully implemented yet") {
        Write-Host "WARNING: OpenAI_API_Client.cpp still contains 'not fully implemented yet' comments." -ForegroundColor Yellow
        Write-Host "If you've implemented these methods, please update the comments." -ForegroundColor Yellow
    }
}

Write-Host "Pre-commit hook completed successfully." -ForegroundColor Green
exit 0 