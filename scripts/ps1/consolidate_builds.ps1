# Build Directory Consolidation Script
# This script helps consolidate multiple build directories into one

Write-Host "AI-First TextEditor Build Directory Consolidation" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green

# List all build directories
$buildDirs = Get-ChildItem -Path . -Directory | Where-Object { $_.Name -match "^build" }

# Display build directories
Write-Host "`nFound $($buildDirs.Count) build directories:" -ForegroundColor Yellow
$buildDirs | ForEach-Object {
    $size = [math]::Round((Get-ChildItem -Path $_.FullName -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB, 2)
    Write-Host "  $($_.Name) ($size MB)" -ForegroundColor Cyan
}

# Function to get most recent builds
function Get-MostRecentBuilds {
    param (
        [array]$BuildDirectories
    )
    
    $mostRecentBuilds = @()
    
    # Check for different configurations
    $configs = @("Debug", "Release", "RelWithDebInfo", "MinSizeRel")
    
    foreach ($config in $configs) {
        $configDirs = $BuildDirectories | Where-Object { Test-Path (Join-Path $_.FullName $config) }
        
        if ($configDirs.Count -gt 0) {
            # Find the most recently modified directory for this configuration
            $mostRecent = $configDirs | Sort-Object -Property { 
                $configPath = Join-Path $_.FullName $config
                $lastWrite = (Get-ChildItem -Path $configPath -Recurse -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1).LastWriteTime
                if ($lastWrite) { $lastWrite } else { [DateTime]::MinValue }
            } -Descending | Select-Object -First 1
            
            $lastWriteTime = (Get-ChildItem -Path (Join-Path $mostRecent.FullName $config) -Recurse -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1).LastWriteTime
            
            if ($lastWriteTime) {
                $mostRecentBuilds += [PSCustomObject]@{
                    BuildDir = $mostRecent.Name
                    Configuration = $config
                    LastModified = $lastWriteTime
                    Path = Join-Path $mostRecent.FullName $config
                }
            }
        }
    }
    
    return $mostRecentBuilds
}

# Get most recent builds
$recentBuilds = Get-MostRecentBuilds -BuildDirectories $buildDirs

# Display recent builds
Write-Host "`nMost recent builds by configuration:" -ForegroundColor Yellow
$recentBuilds | ForEach-Object {
    Write-Host "  $($_.Configuration): $($_.BuildDir) (Last modified: $($_.LastModified))" -ForegroundColor Cyan
}

# Ask which build directory to keep
Write-Host "`nWhich build directory would you like to keep?" -ForegroundColor Yellow
Write-Host "1. Create a new 'build_consolidated' directory (recommended)"
Write-Host "2. Keep an existing directory"
Write-Host "3. Exit without consolidation"
$choice = Read-Host "Enter your choice (1-3)"

$targetDir = ""

switch ($choice) {
    "1" {
        $targetDir = "build_consolidated"
        if (Test-Path $targetDir) {
            Write-Host "Directory $targetDir already exists." -ForegroundColor Red
            $overwrite = Read-Host "Overwrite? (y/n)"
            if ($overwrite -ne "y") {
                Write-Host "Consolidation cancelled." -ForegroundColor Yellow
                exit
            }
            Remove-Item -Path $targetDir -Recurse -Force
        }
        New-Item -Path $targetDir -ItemType Directory | Out-Null
        Write-Host "Created new directory: $targetDir" -ForegroundColor Green
    }
    "2" {
        Write-Host "Available build directories:" -ForegroundColor Yellow
        for ($i = 0; $i -lt $buildDirs.Count; $i++) {
            Write-Host "  $($i+1). $($buildDirs[$i].Name)" -ForegroundColor Cyan
        }
        
        $dirChoice = Read-Host "Enter the number of the directory to keep (1-$($buildDirs.Count))"
        $dirIndex = [int]$dirChoice - 1
        
        if ($dirIndex -ge 0 -and $dirIndex -lt $buildDirs.Count) {
            $targetDir = $buildDirs[$dirIndex].Name
            Write-Host "Selected directory: $targetDir" -ForegroundColor Green
        }
        else {
            Write-Host "Invalid choice. Consolidation cancelled." -ForegroundColor Red
            exit
        }
    }
    "3" {
        Write-Host "Consolidation cancelled." -ForegroundColor Yellow
        exit
    }
    default {
        Write-Host "Invalid choice. Consolidation cancelled." -ForegroundColor Red
        exit
    }
}

# Consolidate build directories
Write-Host "`nConsolidating build directories to $targetDir..." -ForegroundColor Yellow

# Copy recent builds to target directory
foreach ($build in $recentBuilds) {
    $configDir = Join-Path $targetDir $build.Configuration
    
    # Skip if the build is already in the target directory
    if ($build.BuildDir -eq $targetDir) {
        Write-Host "  Skipping $($build.Configuration) from $($build.BuildDir) (already in target)" -ForegroundColor Gray
        continue
    }
    
    Write-Host "  Copying $($build.Configuration) from $($build.BuildDir)..." -ForegroundColor Cyan
    
    # Create configuration directory in target
    if (-not (Test-Path $configDir)) {
        New-Item -Path $configDir -ItemType Directory | Out-Null
    }
    
    # Copy files
    Copy-Item -Path (Join-Path $build.Path "*") -Destination $configDir -Recurse -Force
}

# Copy CMake configuration files from the most recent build directory
$mostRecentBuildDir = $recentBuilds | Sort-Object -Property LastModified -Descending | Select-Object -First 1
if ($mostRecentBuildDir) {
    $sourceBuildDir = $mostRecentBuildDir.BuildDir
    Write-Host "  Copying CMake configuration from $sourceBuildDir..." -ForegroundColor Cyan
    
    # Copy CMake files but exclude the CMakeFiles directory
    Get-ChildItem -Path $sourceBuildDir -Exclude "CMakeFiles", "Debug", "Release", "RelWithDebInfo", "MinSizeRel" | 
        Copy-Item -Destination $targetDir -Recurse -Force
}

Write-Host "`nBuild consolidation complete!" -ForegroundColor Green

# Ask if user wants to delete other build directories
Write-Host "`nWould you like to delete other build directories?" -ForegroundColor Yellow
$deleteOthers = Read-Host "Enter 'y' to delete, 'n' to keep"

if ($deleteOthers -eq "y") {
    foreach ($dir in $buildDirs) {
        if ($dir.Name -ne $targetDir) {
            Write-Host "  Removing $($dir.Name)..." -ForegroundColor Cyan
            Remove-Item -Path $dir.FullName -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-Host "Other build directories removed." -ForegroundColor Green
}

# Update CMake build directory in any build scripts
$buildScripts = Get-ChildItem -Path . -Include *.bat, *.ps1 -File | Where-Object { $_.Name -match "build" }

Write-Host "`nWould you like to update build directory references in build scripts?" -ForegroundColor Yellow
$updateScripts = Read-Host "Enter 'y' to update, 'n' to skip"

if ($updateScripts -eq "y") {
    foreach ($script in $buildScripts) {
        Write-Host "  Checking $($script.Name)..." -ForegroundColor Cyan
        
        $content = Get-Content -Path $script.FullName -Raw
        $updated = $false
        
        foreach ($dir in $buildDirs) {
            if ($dir.Name -ne $targetDir -and $content -match [regex]::Escape($dir.Name)) {
                $content = $content -replace [regex]::Escape($dir.Name), $targetDir
                $updated = $true
            }
        }
        
        if ($updated) {
            Set-Content -Path $script.FullName -Value $content
            Write-Host "    Updated references in $($script.Name)" -ForegroundColor Green
        }
        else {
            Write-Host "    No references found in $($script.Name)" -ForegroundColor Gray
        }
    }
}

Write-Host "`nBuild directory consolidation process complete!" -ForegroundColor Green 