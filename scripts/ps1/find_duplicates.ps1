# Duplicate File Finder
# This script finds and helps remove duplicate files across the workspace

Write-Host "AI-First TextEditor Duplicate File Finder" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green

# Function to calculate file hash
function Get-FileHash {
    param (
        [string]$FilePath,
        [string]$Algorithm = "SHA256"
    )
    
    try {
        $hashAlgorithm = [System.Security.Cryptography.HashAlgorithm]::Create($Algorithm)
        $stream = [System.IO.File]::OpenRead($FilePath)
        $hashBytes = $hashAlgorithm.ComputeHash($stream)
        $stream.Close()
        $hashAlgorithm.Dispose()
        
        return [System.BitConverter]::ToString($hashBytes).Replace("-", "").ToLowerInvariant()
    }
    catch {
        Write-Host "Error calculating hash for $FilePath" -ForegroundColor Red
        return $null
    }
}

# Function to find duplicate files
function Find-DuplicateFiles {
    param (
        [string]$Directory,
        [string[]]$ExcludeDirs = @(),
        [string[]]$IncludePatterns = @("*.*")
    )
    
    Write-Host "Scanning for duplicate files..." -ForegroundColor Yellow
    Write-Host "This might take a while for large repositories..." -ForegroundColor Yellow
    
    # Get all files, excluding specified directories
    $allFiles = @()
    foreach ($pattern in $IncludePatterns) {
        $files = Get-ChildItem -Path $Directory -Recurse -File -Include $pattern -ErrorAction SilentlyContinue
        $allFiles += $files
    }
    
    # Filter out excluded directories
    foreach ($excludeDir in $ExcludeDirs) {
        $excludePath = Join-Path $Directory $excludeDir
        $allFiles = $allFiles | Where-Object { -not $_.FullName.StartsWith($excludePath) }
    }
    
    Write-Host "Found $($allFiles.Count) files to analyze" -ForegroundColor Cyan
    
    # Group files by size first (quick filter)
    $sizeGroups = $allFiles | Group-Object -Property Length | Where-Object { $_.Count -gt 1 }
    
    Write-Host "Found $($sizeGroups.Count) groups of files with identical sizes" -ForegroundColor Cyan
    
    # For each size group, calculate hashes to find true duplicates
    $duplicateGroups = @()
    $progress = 0
    $totalGroups = $sizeGroups.Count
    
    foreach ($group in $sizeGroups) {
        $progress++
        Write-Progress -Activity "Analyzing potential duplicates" -Status "Group $progress of $totalGroups" -PercentComplete (($progress / $totalGroups) * 100)
        
        $hashGroups = @{}
        
        foreach ($file in $group.Group) {
            $hash = Get-FileHash -FilePath $file.FullName
            
            if ($hash) {
                if (-not $hashGroups.ContainsKey($hash)) {
                    $hashGroups[$hash] = @()
                }
                
                $hashGroups[$hash] += $file
            }
        }
        
        # Add only groups with duplicates
        foreach ($hash in $hashGroups.Keys) {
            if ($hashGroups[$hash].Count -gt 1) {
                $duplicateGroups += [PSCustomObject]@{
                    Hash = $hash
                    Files = $hashGroups[$hash]
                    Size = $hashGroups[$hash][0].Length
                    Count = $hashGroups[$hash].Count
                }
            }
        }
    }
    
    Write-Progress -Activity "Analyzing potential duplicates" -Completed
    
    return $duplicateGroups
}

# Main script
$excludeDirs = @(
    ".git",
    "build",
    "build_fast", 
    "build_clean", 
    "build_test", 
    "build_simple",
    "build_msvc",
    "build_new",
    "build_di",
    "build2",
    "build_focused",
    "out",
    "external" # Exclude third-party libraries to avoid false positives
)

$includePatterns = @("*.cpp", "*.h", "*.hpp", "*.c", "*.bat", "*.ps1", "*.py", "*.txt", "*.md")

$duplicateGroups = Find-DuplicateFiles -Directory "." -ExcludeDirs $excludeDirs -IncludePatterns $includePatterns

if ($duplicateGroups.Count -eq 0) {
    Write-Host "`nNo duplicate files found!" -ForegroundColor Green
    exit
}

# Sort duplicate groups by size (largest first)
$duplicateGroups = $duplicateGroups | Sort-Object -Property Size -Descending

# Calculate total waste
$totalWaste = 0
foreach ($group in $duplicateGroups) {
    $totalWaste += ($group.Count - 1) * $group.Size
}

# Display results
Write-Host "`nFound $($duplicateGroups.Count) groups of duplicate files" -ForegroundColor Yellow
Write-Host "Total wasted space: $([math]::Round($totalWaste / 1MB, 2)) MB" -ForegroundColor Yellow

Write-Host "`nDuplicate Files:" -ForegroundColor Cyan
foreach ($group in $duplicateGroups) {
    Write-Host "`nGroup (Size: $([math]::Round($group.Size / 1KB, 2)) KB, Count: $($group.Count))" -ForegroundColor Magenta
    
    $sortedFiles = $group.Files | Sort-Object -Property FullName
    for ($i = 0; $i -lt $sortedFiles.Count; $i++) {
        $file = $sortedFiles[$i]
        
        if ($i -eq 0) {
            Write-Host "  [Original] $($file.FullName)" -ForegroundColor White
        }
        else {
            Write-Host "  [Duplicate] $($file.FullName)" -ForegroundColor Gray
        }
    }
}

# Ask if user wants to save report
$saveReport = Read-Host "`nWould you like to save a detailed report? (y/n)"

if ($saveReport -eq "y") {
    $reportPath = "duplicate_files_report.md"
    
    # Generate report
    $reportContent = @"
# Duplicate Files Report

## Summary
- **Total Duplicate Groups**: $($duplicateGroups.Count)
- **Total Wasted Space**: $([math]::Round($totalWaste / 1MB, 2)) MB

## Duplicate Files
"@
    
    $reportContent | Out-File -FilePath $reportPath -Encoding utf8
    
    foreach ($group in $duplicateGroups) {
        $groupContent = @"

### Group (Size: $([math]::Round($group.Size / 1KB, 2)) KB, Count: $($group.Count))

"@
        $groupContent | Out-File -FilePath $reportPath -Append -Encoding utf8
        
        $sortedFiles = $group.Files | Sort-Object -Property FullName
        for ($i = 0; $i -lt $sortedFiles.Count; $i++) {
            $file = $sortedFiles[$i]
            
            if ($i -eq 0) {
                "- **[Original]** $($file.FullName)" | Out-File -FilePath $reportPath -Append -Encoding utf8
            }
            else {
                "- **[Duplicate]** $($file.FullName)" | Out-File -FilePath $reportPath -Append -Encoding utf8
            }
        }
    }
    
    Write-Host "Report saved to $reportPath" -ForegroundColor Green
}

# Ask if user wants to clean up duplicates
$cleanupDuplicates = Read-Host "`nWould you like to clean up duplicate files? (y/n)"

if ($cleanupDuplicates -eq "y") {
    Write-Host "`nCleanup Options:" -ForegroundColor Yellow
    Write-Host "1. Interactive cleanup (review each group)"
    Write-Host "2. Automatic cleanup (keep first file in each group)"
    Write-Host "3. Cancel cleanup"
    
    $cleanupChoice = Read-Host "Enter your choice (1-3)"
    
    switch ($cleanupChoice) {
        "1" {
            # Interactive cleanup
            foreach ($group in $duplicateGroups) {
                Write-Host "`nGroup (Size: $([math]::Round($group.Size / 1KB, 2)) KB, Count: $($group.Count))" -ForegroundColor Magenta
                
                $sortedFiles = $group.Files | Sort-Object -Property FullName
                for ($i = 0; $i -lt $sortedFiles.Count; $i++) {
                    $file = $sortedFiles[$i]
                    
                    if ($i -eq 0) {
                        Write-Host "  [$i] [Original] $($file.FullName)" -ForegroundColor White
                    }
                    else {
                        Write-Host "  [$i] [Duplicate] $($file.FullName)" -ForegroundColor Gray
                    }
                }
                
                $keepIndex = Read-Host "Enter the index of the file to keep (0-$($sortedFiles.Count - 1)), or 's' to skip this group"
                
                if ($keepIndex -eq "s") {
                    Write-Host "  Skipping this group" -ForegroundColor Yellow
                    continue
                }
                
                try {
                    $keepIndex = [int]$keepIndex
                    
                    if ($keepIndex -ge 0 -and $keepIndex -lt $sortedFiles.Count) {
                        # Remove all files except the one to keep
                        for ($i = 0; $i -lt $sortedFiles.Count; $i++) {
                            if ($i -ne $keepIndex) {
                                $fileToRemove = $sortedFiles[$i]
                                Write-Host "  Removing $($fileToRemove.FullName)" -ForegroundColor Cyan
                                Remove-Item -Path $fileToRemove.FullName -Force
                            }
                        }
                    }
                    else {
                        Write-Host "  Invalid index. Skipping this group" -ForegroundColor Red
                    }
                }
                catch {
                    Write-Host "  Invalid input. Skipping this group" -ForegroundColor Red
                }
            }
        }
        "2" {
            # Automatic cleanup
            foreach ($group in $duplicateGroups) {
                $sortedFiles = $group.Files | Sort-Object -Property FullName
                
                # Keep the first file, remove the rest
                for ($i = 1; $i -lt $sortedFiles.Count; $i++) {
                    $fileToRemove = $sortedFiles[$i]
                    Write-Host "Removing $($fileToRemove.FullName)" -ForegroundColor Cyan
                    Remove-Item -Path $fileToRemove.FullName -Force
                }
            }
        }
        "3" {
            Write-Host "Cleanup cancelled" -ForegroundColor Yellow
        }
        default {
            Write-Host "Invalid choice. Cleanup cancelled" -ForegroundColor Red
        }
    }
}

Write-Host "`nDuplicate file analysis complete!" -ForegroundColor Green 