# Performance Analysis Script
# This script analyzes code performance aspects of the workspace

Write-Host "AI-First TextEditor Performance Analysis" -ForegroundColor Green
Write-Host "=======================================" -ForegroundColor Green

# Function to format file size in readable format
function Format-FileSize {
    param (
        [long]$Size
    )
    
    if ($Size -ge 1GB) {
        return "$([math]::Round($Size / 1GB, 2)) GB"
    }
    elseif ($Size -ge 1MB) {
        return "$([math]::Round($Size / 1MB, 2)) MB"
    }
    elseif ($Size -ge 1KB) {
        return "$([math]::Round($Size / 1KB, 2)) KB"
    }
    else {
        return "$Size bytes"
    }
}

# Function to analyze large files
function Find-LargeFiles {
    param (
        [string]$Directory = ".",
        [long]$ThresholdBytes = 1MB,
        [string[]]$ExcludeDirs = @(".git", "build", "external")
    )
    
    Write-Host "`nScanning for large files (> $(Format-FileSize -Size $ThresholdBytes))..." -ForegroundColor Yellow
    
    $files = Get-ChildItem -Path $Directory -File -Recurse -ErrorAction SilentlyContinue | 
        Where-Object { 
            $file = $_
            $exclude = $false
            foreach ($dir in $ExcludeDirs) {
                if ($file.FullName -like "*\$dir\*") {
                    $exclude = $true
                    break
                }
            }
            -not $exclude
        } |
        Where-Object { $_.Length -gt $ThresholdBytes } |
        Sort-Object -Property Length -Descending |
        Select-Object -First 20
    
    if ($files.Count -eq 0) {
        Write-Host "No large files found." -ForegroundColor Cyan
        return $null
    }
    
    Write-Host "Found $($files.Count) large files:" -ForegroundColor Cyan
    
    $results = @()
    foreach ($file in $files) {
        $extension = $file.Extension
        $size = Format-FileSize -Size $file.Length
        
        Write-Host "  $($file.FullName) ($size)" -ForegroundColor White
        
        $results += [PSCustomObject]@{
            Path = $file.FullName
            Size = $file.Length
            SizeFormatted = $size
            Extension = $extension
        }
    }
    
    return $results
}

# Function to analyze complex code
function Find-ComplexCode {
    param (
        [string]$Directory = ".",
        [string[]]$Extensions = @(".cpp", ".h", ".hpp"),
        [string[]]$ExcludeDirs = @(".git", "build", "external")
    )
    
    Write-Host "`nScanning for potentially complex code..." -ForegroundColor Yellow
    
    $results = @()
    
    # Patterns that might indicate complex code
    $complexPatterns = @(
        @{ Pattern = 'for\s*\(.*for\s*\('; Description = "Nested loops" },
        @{ Pattern = 'if\s*\(.*if\s*\(.*if\s*\('; Description = "Deeply nested conditionals" },
        @{ Pattern = '(switch|case).*?(switch|case)'; Description = "Nested switch statements" },
        @{ Pattern = 'goto'; Description = "Goto statements" },
        @{ Pattern = 'while.*?while'; Description = "Nested while loops" },
        @{ Pattern = '(\/\/|\/\*).*TODO'; Description = "TODO comments" },
        @{ Pattern = '(\/\/|\/\*).*FIXME'; Description = "FIXME comments" },
        @{ Pattern = 'class\s+\w+\s*:\s*public\s+\w+\s*,\s*public'; Description = "Multiple inheritance" },
        @{ Pattern = 'template\s*<.*template\s*<'; Description = "Nested templates" }
    )
    
    # Get all files with specified extensions
    $files = Get-ChildItem -Path $Directory -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { 
            $file = $_
            $include = $Extensions -contains $file.Extension
            $exclude = $false
            foreach ($dir in $ExcludeDirs) {
                if ($file.FullName -like "*\$dir\*") {
                    $exclude = $true
                    break
                }
            }
            $include -and -not $exclude
        }
    
    Write-Host "Analyzing $($files.Count) code files..." -ForegroundColor Cyan
    
    $fileCount = 0
    foreach ($file in $files) {
        $fileCount++
        if ($fileCount % 20 -eq 0) {
            Write-Progress -Activity "Analyzing files for complexity" -Status "$fileCount of $($files.Count)" -PercentComplete (($fileCount / $files.Count) * 100)
        }
        
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction SilentlyContinue
        
        if ($content) {
            $lineCount = ($content.Split("`n")).Count
            $functionMatches = [regex]::Matches($content, "(?m)^\s*(static\s+)?(\w+\s+)+(\w+)\s*\([^;]*\)\s*\{")
            $functionCount = $functionMatches.Count
            
            $complexityFlags = @()
            
            foreach ($pattern in $complexPatterns) {
                $matches = [regex]::Matches($content, $pattern.Pattern)
                if ($matches.Count -gt 0) {
                    $complexityFlags += $pattern.Description
                }
            }
            
            # Calculate an estimated complexity score
            $complexityScore = 0
            $complexityScore += [math]::Min(10, [math]::Floor($lineCount / 500)) # Size factor
            $complexityScore += [math]::Min(10, $functionCount * 0.5) # Function count factor
            $complexityScore += [math]::Min(10, $complexityFlags.Count * 2) # Complexity patterns factor
            
            # Only add files with some complexity
            if ($complexityScore -ge 3 -or $complexityFlags.Count -gt 0) {
                $results += [PSCustomObject]@{
                    Path = $file.FullName
                    Lines = $lineCount
                    Functions = $functionCount
                    ComplexityScore = $complexityScore
                    ComplexityFlags = $complexityFlags -join ", "
                }
            }
        }
    }
    
    Write-Progress -Activity "Analyzing files for complexity" -Completed
    
    # Sort by complexity score
    $results = $results | Sort-Object -Property ComplexityScore -Descending | Select-Object -First 20
    
    if ($results.Count -eq 0) {
        Write-Host "No complex code patterns found." -ForegroundColor Cyan
        return $null
    }
    
    Write-Host "Found $($results.Count) files with potentially complex code:" -ForegroundColor Cyan
    
    foreach ($result in $results) {
        Write-Host "  $($result.Path) (Score: $($result.ComplexityScore), Lines: $($result.Lines))" -ForegroundColor White
        if ($result.ComplexityFlags) {
            Write-Host "    Flags: $($result.ComplexityFlags)" -ForegroundColor Gray
        }
    }
    
    return $results
}

# Function to analyze build impact
function Analyze-BuildImpact {
    param (
        [string]$Directory = ".",
        [string[]]$ExcludeDirs = @(".git", "external")
    )
    
    Write-Host "`nAnalyzing code for potential build impact..." -ForegroundColor Yellow
    
    $results = @()
    
    # Patterns that might impact build performance
    $buildImpactPatterns = @(
        @{ Pattern = '#include\s+[<"].*\.cpp[>"]'; Description = "Including .cpp files" },
        @{ Pattern = '#include\s+[<"]boost/'; Description = "Boost includes" },
        @{ Pattern = '#include\s+[<"].*\.h[>"].*\n.*#include\s+[<"].*\.h[>"].*\n.*#include\s+[<"].*\.h[>"]'; Description = "Many includes" },
        @{ Pattern = '#define\s+\w+\s+\\'; Description = "Multi-line macros" },
        @{ Pattern = '#pragma\s+once'; Description = "Pragma once (vs include guards)" },
        @{ Pattern = 'template\s*<.*>.*class'; Description = "Template classes" },
        @{ Pattern = 'template\s*<.*>.*void'; Description = "Template functions" }
    )
    
    # Get all C++ header files
    $files = Get-ChildItem -Path $Directory -Recurse -File -Include *.h,*.hpp -ErrorAction SilentlyContinue |
        Where-Object { 
            $file = $_
            $exclude = $false
            foreach ($dir in $ExcludeDirs) {
                if ($file.FullName -like "*\$dir\*") {
                    $exclude = $true
                    break
                }
            }
            -not $exclude
        }
    
    Write-Host "Analyzing $($files.Count) header files..." -ForegroundColor Cyan
    
    $fileCount = 0
    foreach ($file in $files) {
        $fileCount++
        if ($fileCount % 20 -eq 0) {
            Write-Progress -Activity "Analyzing files for build impact" -Status "$fileCount of $($files.Count)" -PercentComplete (($fileCount / $files.Count) * 100)
        }
        
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction SilentlyContinue
        
        if ($content) {
            $includeCount = ([regex]::Matches($content, "#include")).Count
            $templateCount = ([regex]::Matches($content, "template\s*<")).Count
            
            $impactFlags = @()
            
            foreach ($pattern in $buildImpactPatterns) {
                $matches = [regex]::Matches($content, $pattern.Pattern)
                if ($matches.Count -gt 0) {
                    $impactFlags += $pattern.Description
                }
            }
            
            # Calculate an impact score
            $impactScore = 0
            $impactScore += [math]::Min(10, $includeCount * 0.5) # Include count factor
            $impactScore += [math]::Min(10, $templateCount * 2) # Template count factor
            $impactScore += [math]::Min(10, $impactFlags.Count * 2) # Impact pattern factor
            
            # Only add files with some impact
            if ($impactScore -ge 5 -or $impactFlags.Count -gt 0) {
                $results += [PSCustomObject]@{
                    Path = $file.FullName
                    Includes = $includeCount
                    Templates = $templateCount
                    ImpactScore = $impactScore
                    ImpactFlags = $impactFlags -join ", "
                }
            }
        }
    }
    
    Write-Progress -Activity "Analyzing files for build impact" -Completed
    
    # Sort by impact score
    $results = $results | Sort-Object -Property ImpactScore -Descending | Select-Object -First 20
    
    if ($results.Count -eq 0) {
        Write-Host "No files with significant build impact found." -ForegroundColor Cyan
        return $null
    }
    
    Write-Host "Found $($results.Count) files with potential build impact:" -ForegroundColor Cyan
    
    foreach ($result in $results) {
        Write-Host "  $($result.Path) (Score: $($result.ImpactScore), Includes: $($result.Includes))" -ForegroundColor White
        if ($result.ImpactFlags) {
            Write-Host "    Impact factors: $($result.ImpactFlags)" -ForegroundColor Gray
        }
    }
    
    return $results
}

# Main execution
$largeFiles = Find-LargeFiles -ThresholdBytes 500KB
$complexCode = Find-ComplexCode
$buildImpact = Analyze-BuildImpact

# Generate report
$reportPath = "performance_analysis_report.md"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

$reportContent = @"
# Performance Analysis Report
Generated on: $timestamp

## Summary

This report identifies potential performance bottlenecks in the codebase:

1. **Large Files**: Files exceeding 500KB which may slow down compilation or loading
2. **Complex Code**: Files with complex code patterns (nested loops, deep conditionals, etc.)
3. **Build Impact**: Header files that may negatively impact build times (excessive includes, templates, etc.)

## Large Files

"@

if ($largeFiles -eq $null -or $largeFiles.Count -eq 0) {
    $reportContent += "No large files found exceeding the threshold of 500KB.`n"
}
else {
    $reportContent += "The following files exceed 500KB in size:`n`n"
    $reportContent += "| File | Size |`n|------|------|\n"
    
    foreach ($file in $largeFiles) {
        $reportContent += "| $($file.Path) | $($file.SizeFormatted) |`n"
    }
}

$reportContent += @"

## Complex Code

"@

if ($complexCode -eq $null -or $complexCode.Count -eq 0) {
    $reportContent += "No significantly complex code patterns were identified.`n"
}
else {
    $reportContent += "The following files contain potentially complex code patterns:`n`n"
    $reportContent += "| File | Complexity Score | Lines | Flags |`n|------|-----------------|-------|-------|\n"
    
    foreach ($file in $complexCode) {
        $reportContent += "| $($file.Path) | $($file.ComplexityScore) | $($file.Lines) | $($file.ComplexityFlags) |`n"
    }
}

$reportContent += @"

## Build Impact

"@

if ($buildImpact -eq $null -or $buildImpact.Count -eq 0) {
    $reportContent += "No files with significant build impact were identified.`n"
}
else {
    $reportContent += "The following header files may have negative impact on build times:`n`n"
    $reportContent += "| File | Impact Score | Includes | Impact Factors |`n|------|-------------|----------|----------------|\n"
    
    foreach ($file in $buildImpact) {
        $reportContent += "| $($file.Path) | $($file.ImpactScore) | $($file.Includes) | $($file.ImpactFlags) |`n"
    }
}

$reportContent += @"

## Recommendations

Based on the analysis, consider the following actions:

1. **For large files**:
   - Split large implementation files into smaller, more focused files
   - Move rarely-changed code into separate files
   - Consider if data could be stored in a more efficient format

2. **For complex code**:
   - Refactor nested loops and deep conditionals into separate functions
   - Address TODO/FIXME comments
   - Simplify complex logic where possible

3. **For build impact**:
   - Reduce header dependencies where possible
   - Use forward declarations instead of includes where appropriate
   - Consider precompiled headers for frequently included standard libraries
"@

# Save report
$reportContent | Out-File -FilePath $reportPath -Encoding utf8

Write-Host "`nPerformance analysis complete!" -ForegroundColor Green
Write-Host "Report saved to: $reportPath" -ForegroundColor Green

# Ask if user wants to open the report
$openReport = Read-Host "`nWould you like to open the report now? (y/n)"
if ($openReport -eq "y") {
    Invoke-Item $reportPath
} 