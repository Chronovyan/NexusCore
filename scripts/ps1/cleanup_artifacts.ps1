# Workspace Cleanup Script
# This script cleans up build artifacts and temporary files from the workspace

Write-Host "AI-First TextEditor Workspace Cleanup" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Green

# Function to calculate directory size
function Get-DirSize {
    param (
        [string]$Path
    )
    
    if (Test-Path -Path $Path) {
        $size = (Get-ChildItem -Path $Path -Recurse -Force -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum
        return [math]::Round($size / 1MB, 2)
    }
    return 0
}

# Track total size saved
$totalSizeMB = 0

# 1. Clean up build directories
$buildDirs = @(
    "build/CMakeFiles",
    "build/Debug",
    "build/Testing",
    "build_fast/CMakeFiles",
    "build_fast/Debug",
    "build_fast/Testing",
    "build_clean/CMakeFiles",
    "build_clean/Debug",
    "build_clean/Testing",
    "build_test/CMakeFiles",
    "build_test/Debug", 
    "build_test/Testing",
    "build_simple/CMakeFiles",
    "build_simple/Debug",
    "build_simple/Testing",
    "build_msvc/CMakeFiles", 
    "build_msvc/Debug",
    "build_msvc/Testing",
    "build_new/CMakeFiles",
    "build_new/Debug",
    "build_new/Testing",
    "build_di/CMakeFiles",
    "build_di/Debug",
    "build_di/Testing",
    "build2/CMakeFiles",
    "build2/Debug",
    "build2/Testing",
    "build_focused/CMakeFiles",
    "build_focused/Debug",
    "build_focused/Testing",
    "out"
)

Write-Host "Scanning build directories..." -ForegroundColor Yellow
foreach ($dir in $buildDirs) {
    if (Test-Path $dir) {
        $sizeMB = Get-DirSize -Path $dir
        if ($sizeMB -gt 0) {
            Write-Host "Found $dir ($sizeMB MB)" -ForegroundColor Cyan
            $totalSizeMB += $sizeMB
        }
    }
}

# 2. Find backup and temporary files
Write-Host "`nScanning for backup and temporary files..." -ForegroundColor Yellow
$backupFiles = Get-ChildItem -Path . -Recurse -File -Include *.bak,*.tmp,*.temp,*.old,*.orig,*.copy,*~ -ErrorAction SilentlyContinue
$bakSize = ($backupFiles | Measure-Object -Property Length -Sum).Sum / 1MB
$totalSizeMB += $bakSize

if ($backupFiles.Count -gt 0) {
    Write-Host "Found $($backupFiles.Count) backup/temporary files ($([math]::Round($bakSize, 2)) MB)" -ForegroundColor Cyan
    $backupFiles | ForEach-Object {
        Write-Host "  $($_.FullName) ($([math]::Round($_.Length / 1KB, 2)) KB)" -ForegroundColor Gray
    }
}

# 3. Find .obj, .o, .pdb, .ilk and other compiled artifacts not in build directories
Write-Host "`nScanning for compiled artifacts outside build directories..." -ForegroundColor Yellow
$compiledArtifacts = Get-ChildItem -Path . -Recurse -File -Include *.obj,*.o,*.pdb,*.ilk,*.exp,*.idb,*.tlog,*.log,*.lastbuildstate -Exclude build*\*,out\* -ErrorAction SilentlyContinue
$artifactSize = ($compiledArtifacts | Measure-Object -Property Length -Sum).Sum / 1MB
$totalSizeMB += $artifactSize

if ($compiledArtifacts.Count -gt 0) {
    Write-Host "Found $($compiledArtifacts.Count) compiled artifacts ($([math]::Round($artifactSize, 2)) MB)" -ForegroundColor Cyan
    $compiledArtifacts | ForEach-Object {
        Write-Host "  $($_.FullName) ($([math]::Round($_.Length / 1KB, 2)) KB)" -ForegroundColor Gray
    }
}

Write-Host "`nTotal space that can be freed: $([math]::Round($totalSizeMB, 2)) MB" -ForegroundColor Green

Write-Host "`nWould you like to:"
Write-Host "1. View full report only"
Write-Host "2. Delete all identified artifacts"
Write-Host "3. Selectively choose which artifacts to delete"
$choice = Read-Host "Enter your choice (1-3)"

switch ($choice) {
    "1" {
        Write-Host "Report generated. No files were deleted." -ForegroundColor Green
    }
    "2" {
        # Delete build directories
        foreach ($dir in $buildDirs) {
            if (Test-Path $dir) {
                Write-Host "Removing $dir..." -ForegroundColor Yellow
                Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete backup files
        if ($backupFiles.Count -gt 0) {
            Write-Host "Removing backup/temporary files..." -ForegroundColor Yellow
            $backupFiles | ForEach-Object {
                Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete compiled artifacts
        if ($compiledArtifacts.Count -gt 0) {
            Write-Host "Removing compiled artifacts..." -ForegroundColor Yellow
            $compiledArtifacts | ForEach-Object {
                Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
            }
        }
        
        Write-Host "Cleanup complete! Freed approximately $([math]::Round($totalSizeMB, 2)) MB of disk space." -ForegroundColor Green
    }
    "3" {
        # Handle selective deletion here
        Write-Host "Selective deletion - Review each category:" -ForegroundColor Yellow
        
        # Prompt for build directories
        $deleteBuilds = Read-Host "Delete build artifacts in CMakeFiles, Debug, and Testing directories? (y/n)"
        if ($deleteBuilds -eq "y") {
            foreach ($dir in $buildDirs) {
                if (Test-Path $dir) {
                    Write-Host "Removing $dir..." -ForegroundColor Yellow
                    Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Prompt for backup files
        $deleteBackups = Read-Host "Delete all backup/temporary files? (y/n)"
        if ($deleteBackups -eq "y") {
            if ($backupFiles.Count -gt 0) {
                Write-Host "Removing backup/temporary files..." -ForegroundColor Yellow
                $backupFiles | ForEach-Object {
                    Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Prompt for compiled artifacts
        $deleteArtifacts = Read-Host "Delete compiled artifacts outside build directories? (y/n)"
        if ($deleteArtifacts -eq "y") {
            if ($compiledArtifacts.Count -gt 0) {
                Write-Host "Removing compiled artifacts..." -ForegroundColor Yellow
                $compiledArtifacts | ForEach-Object {
                    Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        Write-Host "Selective cleanup complete!" -ForegroundColor Green
    }
    default {
        Write-Host "Invalid choice. No files were deleted." -ForegroundColor Red
    }
} 