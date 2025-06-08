# Comprehensive Cleanup Summary Generator
# This script collects and summarizes all cleanup analyses into a single report

Write-Host "AI-First TextEditor Cleanup Summary Generator" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green

# Function to check if file exists
function Test-ReportExists {
    param (
        [string]$ReportPath
    )
    
    if (Test-Path $ReportPath) {
        return $true
    }
    return $false
}

# Function to extract content from markdown reports
function Get-MarkdownContent {
    param (
        [string]$FilePath,
        [string]$SectionHeader = $null
    )
    
    if (-not (Test-Path $FilePath)) {
        return "Report not found: $FilePath"
    }
    
    $content = Get-Content -Path $FilePath -Raw
    
    if ($SectionHeader) {
        $pattern = "(?s)## $SectionHeader\s*\r?\n(.*?)(?:\r?\n## |$)"
        $match = [regex]::Match($content, $pattern)
        if ($match.Success) {
            return $match.Groups[1].Value.Trim()
        }
        return "Section '$SectionHeader' not found in $FilePath"
    }
    
    return $content
}

# Function to extract summaries from reports
function Get-ReportSummary {
    param (
        [string]$FilePath,
        [string]$ReportType
    )
    
    if (-not (Test-Path $FilePath)) {
        return "## $ReportType`n`nReport not found: $FilePath"
    }
    
    $content = Get-Content -Path $FilePath -Raw
    
    # Extract the summary section
    $summaryPattern = "(?s)## Summary\s*\r?\n(.*?)(?:\r?\n## |$)"
    $summaryMatch = [regex]::Match($content, $summaryPattern)
    
    if ($summaryMatch.Success) {
        return "## $ReportType`n`n$($summaryMatch.Groups[1].Value.Trim())"
    }
    
    # If no summary section, extract the first few lines
    $lines = $content -split "`n"
    $firstLines = $lines | Select-Object -First 10 | Out-String
    
    return "## $ReportType`n`n$firstLines"
}

# Check for available reports
$reportFiles = @{
    "Basic Artifacts" = "cleanup_artifacts_report.md"
    "Advanced Cleanup" = "advanced_cleanup_report.md"
    "Build Consolidation" = "build_consolidation_report.md"
    "Documentation" = "documentation_report.md"
    "Dead Code" = "dead_code_report.md"
    "Duplicate Files" = "duplicate_files_report.md"
    "Performance" = "performance_analysis_report.md"
    "Dependencies" = "dependency_analysis_report.md"
}

$availableReports = @()
$unavailableReports = @()

foreach ($report in $reportFiles.GetEnumerator()) {
    if (Test-ReportExists -ReportPath $report.Value) {
        $availableReports += $report
    }
    else {
        $unavailableReports += $report
    }
}

Write-Host "`nAvailable Reports: $($availableReports.Count)" -ForegroundColor Cyan
foreach ($report in $availableReports) {
    Write-Host "  $($report.Key): $($report.Value)" -ForegroundColor White
}

Write-Host "`nUnavailable Reports: $($unavailableReports.Count)" -ForegroundColor Yellow
foreach ($report in $unavailableReports) {
    Write-Host "  $($report.Key): $($report.Value)" -ForegroundColor Gray
}

# Generate the master summary report
$summaryPath = "master_cleanup_summary.md"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

$summaryContent = @"
# Master Cleanup Summary Report
Generated on: $timestamp

This report summarizes the findings from all cleanup tools that have been run.

## Overview

"@

# Add list of available and unavailable reports
$summaryContent += "`n### Reports Included`n`n"
if ($availableReports.Count -gt 0) {
    foreach ($report in $availableReports) {
        $summaryContent += "- $($report.Key)`n"
    }
}
else {
    $summaryContent += "No reports found. Please run the cleanup tools first.`n"
}

if ($unavailableReports.Count -gt 0) {
    $summaryContent += "`n### Reports Not Available`n`n"
    foreach ($report in $unavailableReports) {
        $summaryContent += "- $($report.Key) _(not yet generated)_`n"
    }
}

# Calculate total issues found and space saved
$totalIssues = 0
$totalSpaceSaved = 0

# Add summaries from each available report
foreach ($report in $availableReports) {
    $summaryContent += "`n---`n`n"
    $summaryContent += Get-ReportSummary -FilePath $report.Value -ReportType $report.Key
}

# Add recommendations section
$summaryContent += @"

---

# Consolidated Recommendations

Based on all the analyses, here are the consolidated recommendations for improving your codebase:

"@

# Add specific recommendations based on available reports
if ($availableReports | Where-Object { $_.Key -eq "Basic Artifacts" -or $_.Key -eq "Advanced Cleanup" }) {
    $summaryContent += @"

## Cleanup Recommendations

- Remove all identified build artifacts and temporary files
- Delete backup directories that are no longer needed
- Clean up standalone test binaries and process log files
- Consider removing empty directories
"@
}

if ($availableReports | Where-Object { $_.Key -eq "Build Consolidation" }) {
    $summaryContent += @"

## Build System Recommendations

- Maintain a single consolidated build directory
- Update all build scripts to reference the consolidated directory
- Consider implementing a more structured build process
"@
}

if ($availableReports | Where-Object { $_.Key -eq "Documentation" }) {
    $summaryContent += @"

## Documentation Recommendations

- Fix broken links in documentation
- Update outdated documentation
- Improve documentation structure
- Create missing index files
"@
}

if ($availableReports | Where-Object { $_.Key -eq "Dead Code" }) {
    $summaryContent += @"

## Code Quality Recommendations

- Remove identified dead code
- Address TODO/FIXME comments
- Delete backup and duplicate code files
"@
}

if ($availableReports | Where-Object { $_.Key -eq "Duplicate Files" }) {
    $summaryContent += @"

## File Management Recommendations

- Remove duplicate files to save space and reduce confusion
- Implement a better file organization system
"@
}

if ($availableReports | Where-Object { $_.Key -eq "Performance" }) {
    $summaryContent += @"

## Performance Recommendations

- Split large files into smaller, more focused components
- Refactor complex code patterns
- Optimize header dependencies
- Address build impact issues
"@
}

if ($availableReports | Where-Object { $_.Key -eq "Dependencies" }) {
    $summaryContent += @"

## Dependency Management Recommendations

- Remove unused dependencies
- Evaluate underused dependencies
- Standardize on single versions of dependencies
- Document necessary multiple versions
"@
}

# Add next steps
$summaryContent += @"

# Next Steps

1. **Review and Implement**: Go through each report in detail and implement the suggested changes
2. **Prioritize**: Focus on high-impact changes first (large space savings, significant performance improvements)
3. **Test**: After making changes, thoroughly test the codebase to ensure functionality is preserved
4. **Automate**: Consider setting up automation to prevent similar issues in the future
5. **Document**: Update your documentation to reflect the changes made

The cleanup process is iterative. Consider running these tools periodically to maintain code quality.
"@

# Save the summary report
$summaryContent | Out-File -FilePath $summaryPath -Encoding utf8

Write-Host "`nMaster summary report generated!" -ForegroundColor Green
Write-Host "Report saved to: $summaryPath" -ForegroundColor Green

# Ask if user wants to open the report
$openReport = Read-Host "`nWould you like to open the summary report now? (y/n)"
if ($openReport -eq "y") {
    Invoke-Item $summaryPath
} 