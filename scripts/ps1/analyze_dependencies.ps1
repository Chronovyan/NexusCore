# Dependency Analysis Script
# This script analyzes project dependencies and suggests cleanup opportunities

Write-Host "AI-First TextEditor Dependency Analysis" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# Function to find external dependencies and library usage
function Find-ExternalDependencies {
    param (
        [string]$Directory = ".",
        [string[]]$ExcludeDirs = @(".git", "build", "build_consolidated")
    )
    
    Write-Host "`nScanning for external dependencies..." -ForegroundColor Yellow
    
    # Look for common dependency indicators
    $dependencyFiles = @(
        "CMakeLists.txt",
        "vcpkg.json",
        "packages.config",
        "package.json",
        "requirements.txt",
        "conanfile.txt",
        "Cargo.toml"
    )
    
    $results = @{}
    
    # Find all dependency definition files
    foreach ($depFile in $dependencyFiles) {
        $files = Get-ChildItem -Path $Directory -Recurse -File -Include $depFile -ErrorAction SilentlyContinue | 
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
        
        if ($files.Count -gt 0) {
            Write-Host "Found $($files.Count) $depFile files" -ForegroundColor Cyan
            
            foreach ($file in $files) {
                Write-Host "  Analyzing $($file.FullName)" -ForegroundColor White
                
                $content = Get-Content -Path $file.FullName -Raw -ErrorAction SilentlyContinue
                
                switch -Wildcard ($depFile) {
                    "CMakeLists.txt" {
                        # Find find_package, target_link_libraries, etc.
                        $findPackages = [regex]::Matches($content, 'find_package\s*\(\s*([^\s)]+)')
                        $linkLibraries = [regex]::Matches($content, 'target_link_libraries\s*\(\s*\w+\s+([^)]+)')
                        
                        foreach ($match in $findPackages) {
                            if ($match.Groups.Count -gt 1) {
                                $package = $match.Groups[1].Value
                                if (-not $results.ContainsKey($package)) {
                                    $results[$package] = @{
                                        Name = $package
                                        Type = "CMake Package"
                                        References = @()
                                        Files = @()
                                    }
                                }
                                $results[$package].Files += $file.FullName
                            }
                        }
                        
                        foreach ($match in $linkLibraries) {
                            if ($match.Groups.Count -gt 1) {
                                $libs = $match.Groups[1].Value -split '\s+'
                                foreach ($lib in $libs) {
                                    $lib = $lib.Trim()
                                    if ($lib -and -not $lib.StartsWith('$')) {
                                        if (-not $results.ContainsKey($lib)) {
                                            $results[$lib] = @{
                                                Name = $lib
                                                Type = "Linked Library"
                                                References = @()
                                                Files = @()
                                            }
                                        }
                                        $results[$lib].Files += $file.FullName
                                    }
                                }
                            }
                        }
                    }
                    "package.json" {
                        # Parse JSON for dependencies
                        try {
                            $json = $content | ConvertFrom-Json
                            $depTypes = @("dependencies", "devDependencies")
                            
                            foreach ($depType in $depTypes) {
                                if ($json.PSObject.Properties[$depType]) {
                                    $deps = $json.$depType.PSObject.Properties
                                    foreach ($dep in $deps) {
                                        $depName = $dep.Name
                                        if (-not $results.ContainsKey($depName)) {
                                            $results[$depName] = @{
                                                Name = $depName
                                                Type = "NPM Package"
                                                Version = $dep.Value
                                                References = @()
                                                Files = @()
                                            }
                                        }
                                        $results[$depName].Files += $file.FullName
                                    }
                                }
                            }
                        }
                        catch {
                            Write-Host "    Error parsing $($file.FullName): $_" -ForegroundColor Red
                        }
                    }
                    "requirements.txt" {
                        # Parse Python requirements
                        $lines = $content -split "`n"
                        foreach ($line in $lines) {
                            $line = $line.Trim()
                            if ($line -and -not $line.StartsWith('#')) {
                                $package = ($line -split '[=<>~]')[0].Trim()
                                if (-not $results.ContainsKey($package)) {
                                    $results[$package] = @{
                                        Name = $package
                                        Type = "Python Package"
                                        Version = $line
                                        References = @()
                                        Files = @()
                                    }
                                }
                                $results[$package].Files += $file.FullName
                            }
                        }
                    }
                    "vcpkg.json" {
                        # Parse VCPKG dependencies
                        try {
                            $json = $content | ConvertFrom-Json
                            if ($json.dependencies) {
                                foreach ($dep in $json.dependencies) {
                                    $depName = if ($dep.GetType().Name -eq 'String') { $dep } else { $dep.name }
                                    if (-not $results.ContainsKey($depName)) {
                                        $results[$depName] = @{
                                            Name = $depName
                                            Type = "VCPKG Package"
                                            References = @()
                                            Files = @()
                                        }
                                    }
                                    $results[$depName].Files += $file.FullName
                                }
                            }
                        }
                        catch {
                            Write-Host "    Error parsing $($file.FullName): $_" -ForegroundColor Red
                        }
                    }
                    default {
                        # Generic handling for other dependency files
                        Write-Host "    Basic analysis for $depFile" -ForegroundColor Gray
                    }
                }
            }
        }
    }
    
    return $results
}

# Function to check header includes in source files
function Find-HeaderIncludes {
    param (
        [string]$Directory = ".",
        [string[]]$Extensions = @(".cpp", ".c", ".cc", ".cxx"),
        [string[]]$ExcludeDirs = @(".git", "build", "external", "build_consolidated"),
        [hashtable]$DependencyResults
    )
    
    Write-Host "`nAnalyzing source files for includes..." -ForegroundColor Yellow
    
    $files = Get-ChildItem -Path $Directory -Recurse -File -Include ($Extensions | ForEach-Object { "*$_" }) -ErrorAction SilentlyContinue |
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
    
    Write-Host "Analyzing $($files.Count) source files..." -ForegroundColor Cyan
    
    $includePattern = '#include\s+[<"]([^>"]+)[>"]'
    $fileCount = 0
    
    foreach ($file in $files) {
        $fileCount++
        if ($fileCount % 20 -eq 0) {
            Write-Progress -Activity "Analyzing source files for includes" -Status "$fileCount of $($files.Count)" -PercentComplete (($fileCount / $files.Count) * 100)
        }
        
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction SilentlyContinue
        
        if ($content) {
            $matches = [regex]::Matches($content, $includePattern)
            
            foreach ($match in $matches) {
                if ($match.Groups.Count -gt 1) {
                    $include = $match.Groups[1].Value
                    
                    # Check if this include might reference an external dependency
                    foreach ($dep in $DependencyResults.Keys) {
                        if ($include -like "*$dep*") {
                            if (-not $DependencyResults[$dep].References.Contains($file.FullName)) {
                                $DependencyResults[$dep].References += $file.FullName
                            }
                        }
                    }
                }
            }
        }
    }
    
    Write-Progress -Activity "Analyzing source files for includes" -Completed
    
    return $DependencyResults
}

# Function to identify potentially unused dependencies
function Find-UnusedDependencies {
    param (
        [hashtable]$Dependencies
    )
    
    Write-Host "`nIdentifying potentially unused dependencies..." -ForegroundColor Yellow
    
    $unused = @()
    $potentiallyUnused = @()
    $multipleVersions = @{}
    
    foreach ($key in $Dependencies.Keys) {
        $dep = $Dependencies[$key]
        
        # Check if there are no references to this dependency
        if ($dep.References.Count -eq 0) {
            $unused += $dep
        }
        # Check if there are very few references (potentially unused)
        elseif ($dep.References.Count -lt 3) {
            $potentiallyUnused += $dep
        }
        
        # Check for multiple versions/instances of the same dependency
        $baseName = $dep.Name -replace '\d+(\.\d+)*', ''
        if ($baseName -ne $dep.Name) {
            if (-not $multipleVersions.ContainsKey($baseName)) {
                $multipleVersions[$baseName] = @()
            }
            $multipleVersions[$baseName] += $dep
        }
    }
    
    # Filter multiple versions to only include cases with multiple entries
    $multipleVersions = $multipleVersions.GetEnumerator() | 
        Where-Object { $_.Value.Count -gt 1 } |
        ForEach-Object { 
            [PSCustomObject]@{
                BaseName = $_.Key
                Versions = $_.Value
            }
        }
    
    $result = [PSCustomObject]@{
        Unused = $unused
        PotentiallyUnused = $potentiallyUnused
        MultipleVersions = $multipleVersions
    }
    
    return $result
}

# Main execution
$dependencies = Find-ExternalDependencies
$dependencies = Find-HeaderIncludes -DependencyResults $dependencies

# Count total dependencies
$totalDeps = $dependencies.Count
Write-Host "`nFound $totalDeps total dependencies" -ForegroundColor Cyan

# Analyze usage
$analysis = Find-UnusedDependencies -Dependencies $dependencies

# Display results
Write-Host "`nUnused Dependencies: $($analysis.Unused.Count)" -ForegroundColor Yellow
foreach ($dep in $analysis.Unused) {
    Write-Host "  $($dep.Name) ($($dep.Type))" -ForegroundColor White
    Write-Host "    Defined in: $($dep.Files -join ', ')" -ForegroundColor Gray
}

Write-Host "`nPotentially Underused Dependencies: $($analysis.PotentiallyUnused.Count)" -ForegroundColor Yellow
foreach ($dep in $analysis.PotentiallyUnused) {
    Write-Host "  $($dep.Name) ($($dep.Type)) - $($dep.References.Count) references" -ForegroundColor White
    Write-Host "    Defined in: $($dep.Files -join ', ')" -ForegroundColor Gray
    Write-Host "    Referenced in: $($dep.References -join ', ')" -ForegroundColor Gray
}

Write-Host "`nPotential Multiple Versions: $($analysis.MultipleVersions.Count)" -ForegroundColor Yellow
foreach ($item in $analysis.MultipleVersions) {
    Write-Host "  $($item.BaseName)" -ForegroundColor White
    foreach ($version in $item.Versions) {
        Write-Host "    $($version.Name) ($($version.Type)) - $($version.References.Count) references" -ForegroundColor Gray
    }
}

# Generate report
$reportPath = "dependency_analysis_report.md"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

$reportContent = @"
# Dependency Analysis Report
Generated on: $timestamp

## Summary

- **Total Dependencies**: $totalDeps
- **Unused Dependencies**: $($analysis.Unused.Count)
- **Potentially Underused Dependencies**: $($analysis.PotentiallyUnused.Count)
- **Multiple Versions**: $($analysis.MultipleVersions.Count)

## Unused Dependencies

These dependencies appear to be defined but not referenced in the codebase:

"@

if ($analysis.Unused.Count -eq 0) {
    $reportContent += "No unused dependencies found.`n"
}
else {
    $reportContent += "| Dependency | Type | Defined In |`n|------------|------|-----------|`n"
    foreach ($dep in $analysis.Unused) {
        $reportContent += "| $($dep.Name) | $($dep.Type) | $($dep.Files -join '<br>') |`n"
    }
}

$reportContent += @"

## Potentially Underused Dependencies

These dependencies have very few references and might be candidates for removal:

"@

if ($analysis.PotentiallyUnused.Count -eq 0) {
    $reportContent += "No potentially underused dependencies found.`n"
}
else {
    $reportContent += "| Dependency | Type | References | Defined In |`n|------------|------|-----------|-----------|`n"
    foreach ($dep in $analysis.PotentiallyUnused) {
        $reportContent += "| $($dep.Name) | $($dep.Type) | $($dep.References.Count) | $($dep.Files -join '<br>') |`n"
    }
}

$reportContent += @"

## Multiple Versions

These dependencies appear to have multiple versions or instances:

"@

if ($analysis.MultipleVersions.Count -eq 0) {
    $reportContent += "No multiple versions of dependencies found.`n"
}
else {
    foreach ($item in $analysis.MultipleVersions) {
        $reportContent += "`n### $($item.BaseName)`n`n"
        $reportContent += "| Version | Type | References |`n|---------|------|-----------|`n"
        foreach ($version in $item.Versions) {
            $reportContent += "| $($version.Name) | $($version.Type) | $($version.References.Count) |`n"
        }
    }
}

$reportContent += @"

## Recommendations

Based on the analysis, consider the following actions:

1. **For unused dependencies**:
   - Remove them from your build system if they're truly unnecessary
   - Update documentation if they were intended for future use

2. **For potentially underused dependencies**:
   - Evaluate if functionality could be implemented without the dependency
   - Consider if the dependency brings enough value for its maintenance cost

3. **For multiple versions**:
   - Standardize on a single version where possible
   - Document why multiple versions are needed if they cannot be consolidated
"@

# Save report
$reportContent | Out-File -FilePath $reportPath -Encoding utf8

Write-Host "`nDependency analysis complete!" -ForegroundColor Green
Write-Host "Report saved to: $reportPath" -ForegroundColor Green

# Ask if user wants to open the report
$openReport = Read-Host "`nWould you like to open the report now? (y/n)"
if ($openReport -eq "y") {
    Invoke-Item $reportPath
} 