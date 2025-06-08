# Test script to verify MDC configuration

# Install markdownlint-cli locally if not already installed
if (-not (Test-Path "node_modules/.bin/markdownlint")) {
    Write-Host "markdownlint-cli is not installed. Installing now..." -ForegroundColor Yellow
    npm install --save-dev markdownlint-cli
}

# Run markdownlint with our MDC configuration
Write-Host "\nTesting MDC configuration..." -ForegroundColor Cyan
npx markdownlint -c .mdc/config.json TEST_MDC.md

# Check for any errors
if ($LASTEXITCODE -eq 0) {
    Write-Host "\n✅ MDC configuration is working correctly!" -ForegroundColor Green
} else {
    Write-Host "\n❌ Found some issues with MDC configuration:" -ForegroundColor Red
    Write-Host "Check the output above for details." -ForegroundColor Yellow
}

Write-Host "\nTest completed."
