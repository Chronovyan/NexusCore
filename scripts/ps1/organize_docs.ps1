# Documentation Organization Script
# This script helps organize and improve the project documentation

Write-Host "AI-First TextEditor Documentation Organization" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green

# Function to validate Markdown links
function Test-MarkdownLinks {
    param (
        [string]$Directory
    )
    
    Write-Host "Validating Markdown links..." -ForegroundColor Yellow
    
    $mdFiles = Get-ChildItem -Path $Directory -Recurse -Filter "*.md" -File
    $brokenLinks = @()
    
    foreach ($file in $mdFiles) {
        $content = Get-Content -Path $file.FullName -Raw
        
        # Find Markdown links [text](url)
        $linkMatches = [regex]::Matches($content, '\[([^\]]+)\]\(([^)]+)\)')
        
        foreach ($match in $linkMatches) {
            $linkText = $match.Groups[1].Value
            $linkUrl = $match.Groups[2].Value
            
            # Skip external URLs and anchors
            if ($linkUrl -match '^(http|https|ftp):\/\/') {
                continue
            }
            
            if ($linkUrl -match '^#') {
                # Internal anchor, skip for now
                continue
            }
            
            # Resolve relative path
            $linkedFilePath = [System.IO.Path]::GetDirectoryName($file.FullName)
            $linkedFilePath = [System.IO.Path]::Combine($linkedFilePath, $linkUrl)
            $linkedFilePath = [System.IO.Path]::GetFullPath($linkedFilePath)
            
            # Check if the linked file exists
            if (-not (Test-Path $linkedFilePath)) {
                $brokenLinks += [PSCustomObject]@{
                    SourceFile = $file.FullName
                    LineNumber = ($content.Substring(0, $match.Index).Split("`n")).Count
                    LinkText = $linkText
                    LinkTarget = $linkUrl
                    FullPath = $linkedFilePath
                }
            }
        }
    }
    
    return $brokenLinks
}

# Function to find outdated documentation
function Find-OutdatedDocs {
    param (
        [string]$Directory,
        [int]$DaysOld = 180 # Consider docs older than 6 months potentially outdated
    )
    
    Write-Host "Identifying potentially outdated documentation..." -ForegroundColor Yellow
    
    $cutoffDate = (Get-Date).AddDays(-$DaysOld)
    $mdFiles = Get-ChildItem -Path $Directory -Recurse -Filter "*.md" -File
    
    $outdatedDocs = @()
    
    foreach ($file in $mdFiles) {
        if ($file.LastWriteTime -lt $cutoffDate) {
            # Check if file contains outdated version numbers or dates
            $content = Get-Content -Path $file.FullName -Raw
            
            $outdatedDocs += [PSCustomObject]@{
                File = $file.FullName
                LastUpdated = $file.LastWriteTime
                DaysSinceUpdate = [math]::Round(((Get-Date) - $file.LastWriteTime).TotalDays)
            }
        }
    }
    
    return $outdatedDocs
}

# Function to analyze documentation structure
function Analyze-DocStructure {
    param (
        [string]$Directory
    )
    
    Write-Host "Analyzing documentation structure..." -ForegroundColor Yellow
    
    $docDirs = Get-ChildItem -Path $Directory -Directory
    $mdFiles = Get-ChildItem -Path $Directory -Recurse -Filter "*.md" -File
    
    # Check for index files in each directory
    $missingIndices = @()
    foreach ($dir in $docDirs) {
        $hasIndex = Test-Path (Join-Path $dir.FullName "index.md")
        $hasReadme = Test-Path (Join-Path $dir.FullName "README.md")
        
        if (-not ($hasIndex -or $hasReadme)) {
            $missingIndices += $dir.FullName
        }
    }
    
    # Check for main categories/sections
    $docStructure = @{
        Directory = $Directory
        TotalFiles = $mdFiles.Count
        Directories = $docDirs.Count
        MissingIndices = $missingIndices
        FilesByDirectory = @{}
    }
    
    foreach ($dir in $docDirs) {
        $dirFiles = Get-ChildItem -Path $dir.FullName -Filter "*.md" -File
        $docStructure.FilesByDirectory[$dir.Name] = $dirFiles.Count
    }
    
    return $docStructure
}

# Function to suggest documentation improvements
function Suggest-DocImprovements {
    param (
        [array]$BrokenLinks,
        [array]$OutdatedDocs,
        [object]$DocStructure
    )
    
    $suggestions = @()
    
    # Suggestions for broken links
    if ($BrokenLinks.Count -gt 0) {
        $suggestions += "Fix $($BrokenLinks.Count) broken links in documentation"
    }
    
    # Suggestions for outdated docs
    if ($OutdatedDocs.Count -gt 0) {
        $suggestions += "Review and update $($OutdatedDocs.Count) potentially outdated documentation files"
    }
    
    # Suggestions for structure
    if ($DocStructure.MissingIndices.Count -gt 0) {
        $suggestions += "Add index.md or README.md files to $($DocStructure.MissingIndices.Count) directories"
    }
    
    # Suggestions for missing categories
    $commonCategories = @("getting-started", "user-guide", "api", "developer", "troubleshooting", "faq")
    foreach ($category in $commonCategories) {
        $hasCategory = $false
        foreach ($dir in $DocStructure.FilesByDirectory.Keys) {
            if ($dir -like "*$category*") {
                $hasCategory = $true
                break
            }
        }
        
        if (-not $hasCategory) {
            $suggestions += "Consider adding a '$category' section to documentation"
        }
    }
    
    return $suggestions
}

# Main script execution
$docsDir = "docs"
if (-not (Test-Path $docsDir)) {
    Write-Host "Documentation directory 'docs' not found!" -ForegroundColor Red
    exit 1
}

# Analyze documentation
$brokenLinks = Test-MarkdownLinks -Directory $docsDir
$outdatedDocs = Find-OutdatedDocs -Directory $docsDir
$docStructure = Analyze-DocStructure -Directory $docsDir
$suggestions = Suggest-DocImprovements -BrokenLinks $brokenLinks -OutdatedDocs $outdatedDocs -DocStructure $docStructure

# Display results
Write-Host "`n===== DOCUMENTATION ANALYSIS RESULTS =====" -ForegroundColor Green

Write-Host "`nBroken Links ($($brokenLinks.Count) found):" -ForegroundColor Cyan
if ($brokenLinks.Count -gt 0) {
    $brokenLinks | Format-Table -Property SourceFile, LineNumber, LinkText, LinkTarget -AutoSize
}

Write-Host "`nPotentially Outdated Documentation ($($outdatedDocs.Count) files):" -ForegroundColor Cyan
if ($outdatedDocs.Count -gt 0) {
    $outdatedDocs | Format-Table -Property File, LastUpdated, DaysSinceUpdate -AutoSize
}

Write-Host "`nDocumentation Structure:" -ForegroundColor Cyan
Write-Host "  Total documentation files: $($docStructure.TotalFiles)" -ForegroundColor White
Write-Host "  Documentation directories: $($docStructure.Directories)" -ForegroundColor White

Write-Host "`n  Files by directory:" -ForegroundColor White
foreach ($dir in $docStructure.FilesByDirectory.Keys) {
    Write-Host "    ${dir}: $($docStructure.FilesByDirectory[$dir]) files" -ForegroundColor Gray
}

Write-Host "`n  Directories missing index files:" -ForegroundColor White
if ($docStructure.MissingIndices.Count -gt 0) {
    foreach ($dir in $docStructure.MissingIndices) {
        Write-Host "    $dir" -ForegroundColor Gray
    }
} else {
    Write-Host "    None" -ForegroundColor Gray
}

Write-Host "`nImprovement Suggestions:" -ForegroundColor Cyan
foreach ($suggestion in $suggestions) {
    Write-Host "  - $suggestion" -ForegroundColor White
}

# Generate report
$suggestionsList = ""
foreach ($suggestion in $suggestions) {
    $suggestionsList += "- $suggestion`n"
}

$reportContent = @"
# Documentation Organization Report

## Summary
- **Total Documentation Files**: $($docStructure.TotalFiles)
- **Documentation Directories**: $($docStructure.Directories)
- **Broken Links**: $($brokenLinks.Count)
- **Potentially Outdated Files**: $($outdatedDocs.Count)
- **Directories Missing Index Files**: $($docStructure.MissingIndices.Count)

## Improvement Suggestions
$suggestionsList

## Detailed Findings
"@

# Ask user if they want to save the report
$saveReport = Read-Host "`nWould you like to save a detailed report to 'documentation_report.md'? (y/n)"

if ($saveReport -eq "y") {
    $reportContent | Out-File -FilePath "documentation_report.md" -Encoding utf8
    
    # Add detailed sections
    Add-Content -Path "documentation_report.md" -Value "`n### Broken Links`n"
    if ($brokenLinks.Count -gt 0) {
        foreach ($link in $brokenLinks) {
            Add-Content -Path "documentation_report.md" -Value "- In **$($link.SourceFile)** (line $($link.LineNumber)): Link [$($link.LinkText)]($($link.LinkTarget)) is broken"
        }
    } else {
        Add-Content -Path "documentation_report.md" -Value "No broken links found."
    }
    
    Add-Content -Path "documentation_report.md" -Value "`n### Potentially Outdated Documentation`n"
    if ($outdatedDocs.Count -gt 0) {
        foreach ($doc in $outdatedDocs) {
            Add-Content -Path "documentation_report.md" -Value "- **$($doc.File)** (Last updated: $($doc.LastUpdated), $($doc.DaysSinceUpdate) days ago)"
        }
    } else {
        Add-Content -Path "documentation_report.md" -Value "No potentially outdated documentation found."
    }
    
    Add-Content -Path "documentation_report.md" -Value "`n### Documentation Structure`n"
    Add-Content -Path "documentation_report.md" -Value "#### Files by Directory`n"
    foreach ($dir in $docStructure.FilesByDirectory.Keys) {
        Add-Content -Path "documentation_report.md" -Value "- **${dir}**: $($docStructure.FilesByDirectory[$dir]) files"
    }
    
    Add-Content -Path "documentation_report.md" -Value "`n#### Directories Missing Index Files`n"
    if ($docStructure.MissingIndices.Count -gt 0) {
        foreach ($dir in $docStructure.MissingIndices) {
            Add-Content -Path "documentation_report.md" -Value "- $dir"
        }
    } else {
        Add-Content -Path "documentation_report.md" -Value "No directories are missing index files."
    }
    
    Write-Host "Report saved to 'documentation_report.md'" -ForegroundColor Green
}

# Ask user if they want to create missing index files
$createIndices = Read-Host "`nWould you like to create missing index.md files? (y/n)"

if ($createIndices -eq "y" -and $docStructure.MissingIndices.Count -gt 0) {
    foreach ($dir in $docStructure.MissingIndices) {
        $dirName = Split-Path $dir -Leaf
        
        # Generate contents list
        $contentsList = ""
        $dirFiles = Get-ChildItem -Path $dir -Filter "*.md" -File
        foreach ($file in $dirFiles) {
            $contentsList += "- [$($file.BaseName)]($($file.Name))`n"
        }
        
        $indexContent = @"
# $dirName Documentation

This directory contains documentation related to $dirName.

## Contents

$contentsList

## Overview

*TODO: Add an overview of this documentation section.*
"@
        
        $indexPath = Join-Path $dir "index.md"
        $indexContent | Out-File -FilePath $indexPath -Encoding utf8
        Write-Host "Created index.md in $dir" -ForegroundColor Green
    }
}

Write-Host "`nDocumentation organization analysis complete!" -ForegroundColor Green 