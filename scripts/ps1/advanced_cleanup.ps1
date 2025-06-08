# Advanced Workspace Cleanup Script
# This script performs more aggressive cleanup of the workspace

Write-Host "AI-First TextEditor Advanced Workspace Cleanup" -ForegroundColor Green
Write-Host "===============================================" -ForegroundColor Green

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

# ==============================
# 1. Clean up backup directories
# ==============================
Write-Host "`nCleaning up backup directories..." -ForegroundColor Yellow

$backupDirs = @(
    "temporal_echoes_backup",
    "backups"
)

foreach ($dir in $backupDirs) {
    if (Test-Path $dir) {
        $sizeMB = Get-DirSize -Path $dir
        if ($sizeMB -gt 0) {
            Write-Host "Found backup directory $dir ($sizeMB MB)" -ForegroundColor Cyan
            $totalSizeMB += $sizeMB
            
            # List files in the backup directory
            Write-Host "  Contents:" -ForegroundColor Gray
            Get-ChildItem -Path $dir -Recurse -File | ForEach-Object {
                Write-Host "    $($_.FullName) ($([math]::Round($_.Length / 1KB, 2)) KB)" -ForegroundColor Gray
            }
        }
    }
}

# ==============================
# 2. Clean up standalone tests
# ==============================
Write-Host "`nCleaning up standalone test artifacts..." -ForegroundColor Yellow

$testDirs = @(
    "standalone_test"
)

foreach ($dir in $testDirs) {
    if (Test-Path $dir) {
        # Check for compiled binaries in this directory
        $binaries = Get-ChildItem -Path $dir -Recurse -Include *.exe,*.obj,*.o,*.pdb,*.ilk -ErrorAction SilentlyContinue
        $binSize = ($binaries | Measure-Object -Property Length -Sum).Sum / 1MB
        $totalSizeMB += $binSize
        
        if ($binaries.Count -gt 0) {
            Write-Host "Found $($binaries.Count) binary files in $dir ($([math]::Round($binSize, 2)) MB)" -ForegroundColor Cyan
            $binaries | ForEach-Object {
                Write-Host "  $($_.FullName) ($([math]::Round($_.Length / 1KB, 2)) KB)" -ForegroundColor Gray
            }
        }
    }
}

# ==============================
# 3. Clean up redundant process files
# ==============================
Write-Host "`nCleaning up process log files..." -ForegroundColor Yellow

$processFiles = @(
    "process_diff.txt",
    "processes_after.txt",
    "processes_before.txt"
)

foreach ($file in $processFiles) {
    if (Test-Path $file) {
        $fileSize = (Get-Item $file).Length / 1MB
        $totalSizeMB += $fileSize
        Write-Host "Found process file $file ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Cyan
    }
}

# ==============================
# 4. Clean up all executables in the root directory
# ==============================
Write-Host "`nCleaning up executable files in root directory..." -ForegroundColor Yellow

$exeFiles = Get-ChildItem -Path . -File -Include *.exe -ErrorAction SilentlyContinue
$exeSize = ($exeFiles | Measure-Object -Property Length -Sum).Sum / 1MB
$totalSizeMB += $exeSize

if ($exeFiles.Count -gt 0) {
    Write-Host "Found $($exeFiles.Count) executable files ($([math]::Round($exeSize, 2)) MB)" -ForegroundColor Cyan
    $exeFiles | ForEach-Object {
        Write-Host "  $($_.FullName) ($([math]::Round($_.Length / 1KB, 2)) KB)" -ForegroundColor Gray
    }
}

# ==============================
# 5. Clean up temporary build scripts
# ==============================
Write-Host "`nCleaning up temporary build scripts..." -ForegroundColor Yellow

$buildScripts = @(
    "build_fast.bat",
    "debug_build.bat",
    "focused_debug.bat",
    "fix_script.ps1"
)

foreach ($script in $buildScripts) {
    if (Test-Path $script) {
        $fileSize = (Get-Item $script).Length / 1MB
        $totalSizeMB += $fileSize
        Write-Host "Found build script $script ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Cyan
    }
}

# ==============================
# 6. Clean up all temporary test output files
# ==============================
Write-Host "`nCleaning up test output files..." -ForegroundColor Yellow

$testOutputs = @(
    "test_output.txt",
    "test_results.xml"
)

foreach ($file in $testOutputs) {
    if (Test-Path $file) {
        $fileSize = (Get-Item $file).Length / 1MB
        $totalSizeMB += $fileSize
        Write-Host "Found test output file $file ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Cyan
    }
}

# ==============================
# 7. Clean up temporary code files
# ==============================
Write-Host "`nCleaning up temporary code files..." -ForegroundColor Yellow

$tempCodeFiles = @(
    "fix_file.cpp",
    "fix_file.py",
    "fix_file.exe",
    "SimpleTextBufferTest.cpp",
    "SimpleInsertTest.cpp",
    "debug_log_format.cpp"
)

foreach ($file in $tempCodeFiles) {
    if (Test-Path $file) {
        $fileSize = (Get-Item $file).Length / 1MB
        $totalSizeMB += $fileSize
        Write-Host "Found temporary code file $file ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Cyan
    }
}

# ==============================
# 8. Clean up empty directories
# ==============================
Write-Host "`nIdentifying empty directories..." -ForegroundColor Yellow

$emptyDirs = @()
Get-ChildItem -Path . -Directory -Recurse | ForEach-Object {
    $dir = $_
    $files = Get-ChildItem -Path $dir.FullName -Recurse -File
    if ($files.Count -eq 0) {
        $emptyDirs += $dir.FullName
        Write-Host "Found empty directory: $($dir.FullName)" -ForegroundColor Cyan
    }
}

# ==============================
# Summary and Cleanup Options
# ==============================
Write-Host "`nTotal space that can be freed: $([math]::Round($totalSizeMB, 2)) MB" -ForegroundColor Green

Write-Host "`nAdvanced Cleanup Options:"
Write-Host "1. Generate report only (no deletion)"
Write-Host "2. Delete all identified trash files"
Write-Host "3. Selectively delete identified trash files"
$choice = Read-Host "Enter your choice (1-3)"

switch ($choice) {
    "1" {
        Write-Host "Report generated. No files were deleted." -ForegroundColor Green
    }
    "2" {
        # Delete backup directories
        foreach ($dir in $backupDirs) {
            if (Test-Path $dir) {
                Write-Host "Removing backup directory $dir..." -ForegroundColor Yellow
                Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete binaries in standalone test directories
        foreach ($dir in $testDirs) {
            if (Test-Path $dir) {
                $binaries = Get-ChildItem -Path $dir -Recurse -Include *.exe,*.obj,*.o,*.pdb,*.ilk -ErrorAction SilentlyContinue
                if ($binaries.Count -gt 0) {
                    Write-Host "Removing binary files in $dir..." -ForegroundColor Yellow
                    $binaries | ForEach-Object {
                        Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
                    }
                }
            }
        }
        
        # Delete process files
        foreach ($file in $processFiles) {
            if (Test-Path $file) {
                Write-Host "Removing process file $file..." -ForegroundColor Yellow
                Remove-Item $file -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete exe files in root directory
        if ($exeFiles.Count -gt 0) {
            Write-Host "Removing executable files..." -ForegroundColor Yellow
            $exeFiles | ForEach-Object {
                Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete build scripts
        foreach ($script in $buildScripts) {
            if (Test-Path $script) {
                Write-Host "Removing build script $script..." -ForegroundColor Yellow
                Remove-Item $script -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete test output files
        foreach ($file in $testOutputs) {
            if (Test-Path $file) {
                Write-Host "Removing test output file $file..." -ForegroundColor Yellow
                Remove-Item $file -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete temporary code files
        foreach ($file in $tempCodeFiles) {
            if (Test-Path $file) {
                Write-Host "Removing temporary code file $file..." -ForegroundColor Yellow
                Remove-Item $file -Force -ErrorAction SilentlyContinue
            }
        }
        
        # Delete empty directories
        foreach ($dir in $emptyDirs) {
            if (Test-Path $dir) {
                Write-Host "Removing empty directory $dir..." -ForegroundColor Yellow
                Remove-Item -Path $dir -Force -ErrorAction SilentlyContinue
            }
        }
        
        Write-Host "Advanced cleanup complete! Freed approximately $([math]::Round($totalSizeMB, 2)) MB of disk space." -ForegroundColor Green
    }
    "3" {
        # Selective deletion
        
        # Backup directories
        $deleteBackupDirs = Read-Host "Delete backup directories (temporal_echoes_backup, backups)? (y/n)"
        if ($deleteBackupDirs -eq "y") {
            foreach ($dir in $backupDirs) {
                if (Test-Path $dir) {
                    Write-Host "Removing backup directory $dir..." -ForegroundColor Yellow
                    Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Standalone test binaries
        $deleteTestBinaries = Read-Host "Delete binaries in standalone test directories? (y/n)"
        if ($deleteTestBinaries -eq "y") {
            foreach ($dir in $testDirs) {
                if (Test-Path $dir) {
                    $binaries = Get-ChildItem -Path $dir -Recurse -Include *.exe,*.obj,*.o,*.pdb,*.ilk -ErrorAction SilentlyContinue
                    if ($binaries.Count -gt 0) {
                        Write-Host "Removing binary files in $dir..." -ForegroundColor Yellow
                        $binaries | ForEach-Object {
                            Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
                        }
                    }
                }
            }
        }
        
        # Process files
        $deleteProcessFiles = Read-Host "Delete process log files (process_diff.txt, processes_*.txt)? (y/n)"
        if ($deleteProcessFiles -eq "y") {
            foreach ($file in $processFiles) {
                if (Test-Path $file) {
                    Write-Host "Removing process file $file..." -ForegroundColor Yellow
                    Remove-Item $file -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Executables
        $deleteExeFiles = Read-Host "Delete executable files in root directory? (y/n)"
        if ($deleteExeFiles -eq "y") {
            if ($exeFiles.Count -gt 0) {
                Write-Host "Removing executable files..." -ForegroundColor Yellow
                $exeFiles | ForEach-Object {
                    Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Build scripts
        $deleteBuildScripts = Read-Host "Delete temporary build scripts (build_fast.bat, debug_build.bat, etc.)? (y/n)"
        if ($deleteBuildScripts -eq "y") {
            foreach ($script in $buildScripts) {
                if (Test-Path $script) {
                    Write-Host "Removing build script $script..." -ForegroundColor Yellow
                    Remove-Item $script -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Test output files
        $deleteTestOutputs = Read-Host "Delete test output files (test_output.txt, test_results.xml)? (y/n)"
        if ($deleteTestOutputs -eq "y") {
            foreach ($file in $testOutputs) {
                if (Test-Path $file) {
                    Write-Host "Removing test output file $file..." -ForegroundColor Yellow
                    Remove-Item $file -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Temporary code files
        $deleteTempCodeFiles = Read-Host "Delete temporary code files (fix_file.cpp, SimpleTextBufferTest.cpp, etc.)? (y/n)"
        if ($deleteTempCodeFiles -eq "y") {
            foreach ($file in $tempCodeFiles) {
                if (Test-Path $file) {
                    Write-Host "Removing temporary code file $file..." -ForegroundColor Yellow
                    Remove-Item $file -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        # Empty directories
        $deleteEmptyDirs = Read-Host "Delete empty directories? (y/n)"
        if ($deleteEmptyDirs -eq "y") {
            foreach ($dir in $emptyDirs) {
                if (Test-Path $dir) {
                    Write-Host "Removing empty directory $dir..." -ForegroundColor Yellow
                    Remove-Item -Path $dir -Force -ErrorAction SilentlyContinue
                }
            }
        }
        
        Write-Host "Selective advanced cleanup complete!" -ForegroundColor Green
    }
    default {
        Write-Host "Invalid choice. No files were deleted." -ForegroundColor Red
    }
} 