# Dead Code Identification Script
# This script identifies potential dead/unused code in the codebase

Write-Host "AI-First TextEditor Dead Code Identification" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green

# Function to detect large commented code blocks
function Find-CommentedCodeBlocks {
    param (
        [string]$Directory,
        [string[]]$Extensions = @(".cpp", ".h", ".hpp")
    )
    
    Write-Host "Scanning for large commented code blocks..." -ForegroundColor Yellow
    
    $results = @()
    
    foreach ($ext in $Extensions) {
        $files = Get-ChildItem -Path $Directory -Recurse -Filter "*$ext" -File
        
        foreach ($file in $files) {
            $content = Get-Content -Path $file.FullName -Raw
            $lines = $content -split "`n"
            
            $inCommentBlock = $false
            $blockStart = 0
            $commentBlockLines = @()
            
            for ($i = 0; $i -lt $lines.Count; $i++) {
                $line = $lines[$i]
                
                # Check for single-line comment pattern
                if ($line -match '^\s*\/\/\s*\w+') {
                    if (-not $inCommentBlock) {
                        $inCommentBlock = $true
                        $blockStart = $i
                    }
                    
                    $commentBlockLines += $line
                }
                else {
                    if ($inCommentBlock) {
                        # End of comment block
                        if ($commentBlockLines.Count -ge 5) {
                            # If block has at least 5 consecutive comment lines
                            $results += [PSCustomObject]@{
                                File = $file.FullName
                                StartLine = $blockStart + 1
                                EndLine = $i
                                LineCount = $commentBlockLines.Count
                                Preview = ($commentBlockLines[0..([Math]::Min(3, $commentBlockLines.Count-1))] -join "`n") + "..."
                            }
                        }
                        
                        $inCommentBlock = $false
                        $commentBlockLines = @()
                    }
                }
            }
            
            # Check if file ends with a comment block
            if ($inCommentBlock -and $commentBlockLines.Count -ge 5) {
                $results += [PSCustomObject]@{
                    File = $file.FullName
                    StartLine = $blockStart + 1
                    EndLine = $lines.Count
                    LineCount = $commentBlockLines.Count
                    Preview = ($commentBlockLines[0..([Math]::Min(3, $commentBlockLines.Count-1))] -join "`n") + "..."
                }
            }
        }
    }
    
    return $results
}

# Function to find deprecated code
function Find-DeprecatedCode {
    param (
        [string]$Directory,
        [string[]]$Extensions = @(".cpp", ".h", ".hpp")
    )
    
    Write-Host "Scanning for deprecated code..." -ForegroundColor Yellow
    
    $results = @()
    
    foreach ($ext in $Extensions) {
        $files = Get-ChildItem -Path $Directory -Recurse -Filter "*$ext" -File
        
        foreach ($file in $files) {
            $content = Get-Content -Path $file.FullName
            
            for ($i = 0; $i -lt $content.Count; $i++) {
                $line = $content[$i]
                
                if ($line -match '@deprecated|DEPRECATED|DO_NOT_USE|obsolete') {
                    $results += [PSCustomObject]@{
                        File = $file.FullName
                        Line = $i + 1
                        Content = $line.Trim()
                    }
                }
            }
        }
    }
    
    return $results
}

# Function to find backup/duplicate files
function Find-BackupFiles {
    param (
        [string]$Directory
    )
    
    Write-Host "Scanning for backup and duplicate files..." -ForegroundColor Yellow
    
    $files = Get-ChildItem -Path $Directory -Recurse -File -Include "*.bak", "*.old", "*.backup", "*.tmp", "*.temp", "*.copy" -ErrorAction SilentlyContinue
    
    $results = @()
    
    foreach ($file in $files) {
        # Get the original filename by removing the extension
        $originalName = $file.FullName -replace "\.(bak|old|backup|tmp|temp|copy)$", ""
        
        if (Test-Path $originalName) {
            $results += [PSCustomObject]@{
                BackupFile = $file.FullName
                OriginalFile = $originalName
                Size = [math]::Round($file.Length / 1KB, 2)
            }
        }
        else {
            $results += [PSCustomObject]@{
                BackupFile = $file.FullName
                OriginalFile = "N/A (Original not found)"
                Size = [math]::Round($file.Length / 1KB, 2)
            }
        }
    }
    
    return $results
}

# Function to find code with TODO/FIXME comments
function Find-TodoFixme {
    param (
        [string]$Directory,
        [string[]]$Extensions = @(".cpp", ".h", ".hpp")
    )
    
    Write-Host "Scanning for TODO/FIXME comments..." -ForegroundColor Yellow
    
    $results = @()
    
    foreach ($ext in $Extensions) {
        $files = Get-ChildItem -Path $Directory -Recurse -Filter "*$ext" -File
        
        foreach ($file in $files) {
            $content = Get-Content -Path $file.FullName
            
            for ($i = 0; $i -lt $content.Count; $i++) {
                $line = $content[$i]
                
                if ($line -match 'TODO|FIXME|XXX') {
                    $results += [PSCustomObject]@{
                        File = $file.FullName
                        Line = $i + 1
                        Content = $line.Trim()
                    }
                }
            }
        }
    }
    
    return $results
}

# Function to find temp/fixed files
function Find-TempFixedFiles {
    param (
        [string]$Directory
    )
    
    Write-Host "Scanning for temp/fixed files..." -ForegroundColor Yellow
    
    $tempFixedFiles = Get-ChildItem -Path $Directory -Recurse -File | Where-Object {
        $_.Name -match '\.fixed\.|\.temp\.|temp\.|fixed\.'
    }
    
    $results = @()
    
    foreach ($file in $tempFixedFiles) {
        $results += [PSCustomObject]@{
            File = $file.FullName
            Size = [math]::Round($file.Length / 1KB, 2)
            LastModified = $file.LastWriteTime
        }
    }
    
    return $results
}

# Start the scan
$sourceDir = "."

# Collect results
$commentedBlocks = Find-CommentedCodeBlocks -Directory $sourceDir
$deprecatedCode = Find-DeprecatedCode -Directory $sourceDir
$backupFiles = Find-BackupFiles -Directory $sourceDir
$todoFixme = Find-TodoFixme -Directory $sourceDir
$tempFixedFiles = Find-TempFixedFiles -Directory $sourceDir

# Output results
Write-Host "`n===== RESULTS =====" -ForegroundColor Green

Write-Host "`nLarge Commented Code Blocks ($($commentedBlocks.Count) found):" -ForegroundColor Cyan
if ($commentedBlocks.Count -gt 0) {
    $commentedBlocks | Format-Table -Property File, StartLine, EndLine, LineCount -AutoSize
}

Write-Host "`nDeprecated Code ($($deprecatedCode.Count) found):" -ForegroundColor Cyan
if ($deprecatedCode.Count -gt 0) {
    $deprecatedCode | Format-Table -Property File, Line, Content -AutoSize
}

Write-Host "`nBackup/Duplicate Files ($($backupFiles.Count) found):" -ForegroundColor Cyan
if ($backupFiles.Count -gt 0) {
    $backupFiles | Format-Table -Property BackupFile, OriginalFile, Size -AutoSize
}

Write-Host "`nTODO/FIXME Items ($($todoFixme.Count) found):" -ForegroundColor Cyan
if ($todoFixme.Count -gt 0) {
    $todoFixme | Format-Table -Property File, Line, Content -AutoSize
}

Write-Host "`nTemp/Fixed Files ($($tempFixedFiles.Count) found):" -ForegroundColor Cyan
if ($tempFixedFiles.Count -gt 0) {
    $tempFixedFiles | Format-Table -Property File, Size, LastModified -AutoSize
}

# Generate summary report
$reportContent = @"
# Dead Code and Technical Debt Report

## Summary
- **Large Commented Code Blocks**: $($commentedBlocks.Count) found
- **Deprecated Code**: $($deprecatedCode.Count) instances
- **Backup/Duplicate Files**: $($backupFiles.Count) files
- **TODO/FIXME Items**: $($todoFixme.Count) items
- **Temp/Fixed Files**: $($tempFixedFiles.Count) files

## Recommendations
1. Review and remove commented code blocks that are no longer needed
2. Update or remove deprecated code
3. Clean up backup and temporary files after confirming they're no longer needed
4. Address TODO/FIXME items or convert them to proper issue tracking

## Detailed Findings
"@

# Ask user if they want to save the report
$saveReport = Read-Host "`nWould you like to save a detailed report to 'dead_code_report.md'? (y/n)"

if ($saveReport -eq "y") {
    $reportContent | Out-File -FilePath "dead_code_report.md" -Encoding utf8
    
    # Add detailed sections
    Add-Content -Path "dead_code_report.md" -Value "`n### Large Commented Code Blocks`n"
    foreach ($block in $commentedBlocks) {
        $reportLine = @"
- **$($block.File)** (lines $($block.StartLine)-$($block.EndLine), $($block.LineCount) lines)
  Preview: ```$($block.Preview)```
"@
        Add-Content -Path "dead_code_report.md" -Value $reportLine
    }
    
    Add-Content -Path "dead_code_report.md" -Value "`n### Deprecated Code`n"
    foreach ($item in $deprecatedCode) {
        Add-Content -Path "dead_code_report.md" -Value "- **$($item.File)** (line $($item.Line)): ``$($item.Content.Trim())``"
    }
    
    Add-Content -Path "dead_code_report.md" -Value "`n### Backup/Duplicate Files`n"
    foreach ($file in $backupFiles) {
        Add-Content -Path "dead_code_report.md" -Value "- **$($file.BackupFile)** ($($($file.Size.ToString()) + " KB")) - Original: $($file.OriginalFile)"
    }
    
    Add-Content -Path "dead_code_report.md" -Value "`n### TODO/FIXME Items`n"
    foreach ($item in $todoFixme) {
        Add-Content -Path "dead_code_report.md" -Value "- **$($item.File)** (line $($item.Line)): ``$($item.Content.Trim())``"
    }
    
    Add-Content -Path "dead_code_report.md" -Value "`n### Temp/Fixed Files`n"
    foreach ($file in $tempFixedFiles) {
        Add-Content -Path "dead_code_report.md" -Value "- **$($file.File)** ($($($file.Size.ToString()) + " KB")) - Last Modified: $($file.LastModified.ToString('yyyy-MM-dd HH:mm:ss'))"
    }
    
    Write-Host "Report saved to 'dead_code_report.md'" -ForegroundColor Green
}

Write-Host "`nDead code identification complete!" -ForegroundColor Green 