# Advanced Pre-commit Hook: Static checks + Build and Critical tests

Write-Host "Running advanced pre-commit checks..."

$failures = @()
$exitCode = 0

# 1. Check for modifications to critical API implementation files
$criticalFiles = @("OpenAI_API_Client.cpp", "OpenAI_API_Client.h", "IOpenAI_API_Client.h")
$stagedFiles = git diff --cached --name-only

foreach ($file in $criticalFiles) {
    if ($stagedFiles -contains $file) {
        Write-Host "Critical file staged: $file"
        # 2. Warn if "not implemented yet" is still present
        if (Test-Path $file) {
            $content = Get-Content $file -Raw
            if ($content -match "not implemented yet") {
                Write-Warning "'not implemented yet' found in $file. Please complete implementation before committing."
                $failures += "'not implemented yet' in $file"
            }
        }
    }
}

# 3. Check interface compatibility (basic signature check)
if ((Test-Path "IOpenAI_API_Client.h") -and (Test-Path "OpenAI_API_Client.h")) {
    $interface = Get-Content "IOpenAI_API_Client.h" -Raw
    $impl = Get-Content "OpenAI_API_Client.h" -Raw
    if (-not ($impl -match "listModels") -or -not ($impl -match "retrieveModel") -or -not ($impl -match "createEmbedding")) {
        Write-Warning "OpenAI_API_Client.h may not fully implement all methods from IOpenAI_API_Client.h"
        $failures += "API client missing interface methods"
    }
}

# 4. Warn if test files were modified but no critical tests added
$testFiles = $stagedFiles | Where-Object { $_ -like "tests/*" }
foreach ($t in $testFiles) {
    if (Test-Path $t) {
        $testContent = Get-Content $t -Raw
        if (-not ($testContent -match "CRITICAL_TEST|Critical")) {
            Write-Warning "Test file $t was modified, but no critical tests detected. Consider tagging important tests as CRITICAL."
        }
    }
}

# 5. If any static analysis failed, block the commit before building
if ($failures.Count -gt 0) {
    Write-Error "Enhanced pre-commit checks failed:`n$($failures -join "`n")"
    exit 1
}

# 6. Build and run "Critical" tests
Write-Host "Static checks passed. Building project and running critical tests..." -ForegroundColor Cyan

cmake -S . -B build -DCMAKE_CXX_COMPILER=g++
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configure failed."
    exit 1
}
cmake --build build
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit 1
}

Set-Location build
ctest -R Critical --output-on-failure
if ($LASTEXITCODE -ne 0) {
    Write-Error "Critical tests failed."
    exit 1
}
Set-Location ..

Write-Host "All advanced pre-commit checks and critical tests passed. Commit allowed." -ForegroundColor Green
exit 0 