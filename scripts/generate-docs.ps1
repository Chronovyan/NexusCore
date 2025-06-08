<#
.SYNOPSIS
    Generates documentation for the project.
.DESCRIPTION
    This script generates API documentation using Doxygen and other documentation tools.
    It can generate documentation in various formats including HTML, PDF, and man pages.
.PARAMETER OutputDir
    The output directory for the generated documentation (default: 'docs').
.PARAMETER Format
    The output format(s) to generate (html, latex, xml, man, rtf).
.PARAMETER Open
    Open the generated documentation in the default web browser.
.PARAMETER Clean
    Clean the output directory before generating documentation.
.PARAMETER DoxygenConfig
    Path to a custom Doxygen configuration file.
.PARAMETER Verbose
    Show detailed output.
.EXAMPLE
    .\scripts\generate-docs.ps1
    Generates HTML documentation in the default location.
.EXAMPLE
    .\scripts\generate-docs.ps1 -Format html,pdf -Open
    Generates HTML and PDF documentation and opens the HTML in the default browser.
#>

[CmdletBinding()]
param(
    [string]$OutputDir = "docs",
    [string[]]$Format = @("html"),
    [switch]$Open,
    [switch]$Clean,
    [string]$DoxygenConfig,
    [switch]$Verbose
)

# Error handling
$ErrorActionPreference = "Stop"

# Colors for output
$ErrorColor = "Red"
$WarningColor = "Yellow"
$SuccessColor = "Green"
$InfoColor = "Cyan"

# Get the project root directory
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $projectRoot $OutputDir

# Check for required tools
$missingTools = @()

if (-not (Get-Command "doxygen" -ErrorAction SilentlyContinue)) {
    $missingTools += "Doxygen (install with: choco install doxygen.install)"
}

# Check for optional tools
$hasDoxygen = $true
if (-not (Get-Command "doxygen" -ErrorAction SilentlyContinue)) {
    $hasDoxygen = $false
    Write-Host "Doxygen not found. Some documentation features may be limited." -ForegroundColor $WarningColor
}

$hasGraphviz = $true
if (-not (Get-Command "dot" -ErrorAction SilentlyContinue)) {
    $hasGraphviz = $false
    Write-Host "Graphviz not found. Class diagrams will not be generated." -ForegroundColor $WarningColor
}

if ($missingTools.Count -gt 0) {
    Write-Host "The following tools are required but not found:" -ForegroundColor $ErrorColor
    $missingTools | ForEach-Object { Write-Host "  - $_" -ForegroundColor $ErrorColor }
    Write-Host "`nPlease install the missing tools and try again." -ForegroundColor $WarningColor
    exit 1
}

# Create output directory if it doesn't exist
if (-not (Test-Path $outputPath)) {
    New-Item -ItemType Directory -Path $outputPath | Out-Null
} elseif ($Clean) {
    Write-Host "Cleaning output directory: $outputPath" -ForegroundColor $InfoColor
    Remove-Item -Path "$outputPath\*" -Recurse -Force
}

# Generate Doxygen documentation if available
if ($hasDoxygen) {
    Write-Host "Generating API documentation with Doxygen..." -ForegroundColor $InfoColor
    
    # Create a temporary Doxyfile if none specified
    $doxyfile = $DoxygenConfig
    $tempDoxyfile = $false
    
    if ([string]::IsNullOrEmpty($doxyfile)) {
        $doxyfile = Join-Path $env:TEMP "Doxyfile"
        $tempDoxyfile = $true
        
        # Generate a basic Doxyfile
        $doxygenArgs = @(
            "-s", "-g", "$doxyfile"
        )
        
        & doxygen $doxygenArgs
        
        # Update the Doxyfile with our settings
        $doxyContent = Get-Content -Path $doxyfile -Raw
        
        # Set common options
        $replacements = @{
            'PROJECT_NAME\s*=.*' = "PROJECT_NAME           = \"AI-First Text Editor\""
            'PROJECT_NUMBER\s*=.*' = "PROJECT_NUMBER         = 1.0"
            'OUTPUT_DIRECTORY\s*=.*' = "OUTPUT_DIRECTORY       = $outputPath"
            'RECURSIVE\s*=.*' = 'RECURSIVE              = YES'
            'EXTRACT_ALL\s*=.*' = 'EXTRACT_ALL            = YES'
            'EXTRACT_PRIVATE\s*=.*' = 'EXTRACT_PRIVATE        = YES'
            'EXTRACT_STATIC\s*=.*' = 'EXTRACT_STATIC         = YES'
            'QUIET\s*=.*' = 'QUIET                   = NO'
            'WARN_IF_UNDOCUMENTED\s*=.*' = 'WARN_IF_UNDOCUMENTED   = YES'
            'WARN_IF_DOC_ERROR\s*=.*' = 'WARN_IF_DOC_ERROR      = YES'
            'GENERATE_HTML\s*=.*' = 'GENERATE_HTML          = ' + ($Format -contains 'html').ToString().ToUpper()
            'GENERATE_LATEX\s*=.*' = 'GENERATE_LATEX         = ' + ($Format -contains 'latex').ToString().ToUpper()
            'GENERATE_XML\s*=.*' = 'GENERATE_XML           = ' + ($Format -contains 'xml').ToString().ToUpper()
            'GENERATE_MAN\s*=.*' = 'GENERATE_MAN           = ' + ($Format -contains 'man').ToString().ToUpper()
            'GENERATE_RTF\s*=.*' = 'GENERATE_RTF           = ' + ($Format -contains 'rtf').ToString().ToUpper()
            'HAVE_DOT\s*=.*' = 'HAVE_DOT               = ' + $hasGraphviz.ToString().ToUpper()
            'DOT_IMAGE_FORMAT\s*=.*' = 'DOT_IMAGE_FORMAT       = svg'
            'INTERACTIVE_SVG\s*=.*' = 'INTERACTIVE_SVG         = YES'
            'DOT_TRANSPARENT\s*=.*' = 'DOT_TRANSPARENT        = YES'
            'DOT_MULTI_TARGETS\s*=.*' = 'DOT_MULTI_TARGETS      = YES'
            'GENERATE_TREEVIEW\s*=.*' = 'GENERATE_TREEVIEW      = YES'
            'FULL_PATH_NAMES\s*=.*' = 'FULL_PATH_NAMES        = NO'
            'JAVADOC_AUTOBRIEF\s*=.*' = 'JAVADOC_AUTOBRIEF      = YES'
            'QT_AUTOBRIEF\s*=.*' = 'QT_AUTOBRIEF           = YES'
            'EXTRACT_PACKAGE\s*=.*' = 'EXTRACT_PACKAGE        = YES'
            'HIDE_UNDOC_MEMBERS\s*=.*' = 'HIDE_UNDOC_MEMBERS     = NO'
            'HIDE_UNDOC_CLASSES\s*=.*' = 'HIDE_UNDOC_CLASSES     = NO'
            'HIDE_FRIEND_COMPOUNDS\s*=.*' = 'HIDE_FRIEND_COMPOUNDS   = NO'
            'HIDE_IN_BODY_DOCS\s*=.*' = 'HIDE_IN_BODY_DOCS      = NO'
            'INTERNAL_DOCS\s*=.*' = 'INTERNAL_DOCS          = YES'
            'CASE_SENSE_NAMES\s*=.*' = 'CASE_SENSE_NAMES       = YES'
            'HIDE_SCOPE_NAMES\s*=.*' = 'HIDE_SCOPE_NAMES       = NO'
            'SHOW_INCLUDE_FILES\s*=.*' = 'SHOW_INCLUDE_FILES     = YES'
            'FORCE_LOCAL_INCLUDES\s*=.*' = 'FORCE_LOCAL_INCLUDES   = YES'
            'INLINE_INFO\s*=.*' = 'INLINE_INFO            = YES'
            'SORT_MEMBER_DOCS\s*=.*' = 'SORT_MEMBER_DOCS       = NO'
            'SORT_BRIEF_DOCS\s*=.*' = 'SORT_BRIEF_DOCS        = NO'
            'SORT_MEMBERS_CTORS_1ST\s*=.*' = 'SORT_MEMBERS_CTORS_1ST = YES'
            'SORT_GROUP_NAMES\s*=.*' = 'SORT_GROUP_NAMES       = YES'
            'SORT_BY_SCOPE_NAME\s*=.*' = 'SORT_BY_SCOPE_NAME     = YES'
            'GENERATE_HTMLHELP\s*=.*' = 'GENERATE_HTMLHELP      = YES'
            'GENERATE_CHI\s*=.*' = 'GENERATE_CHI           = YES'
            'TOC_INCLUDE_HEADINGS\s*=.*' = 'TOC_INCLUDE_HEADINGS   = 4'
            'TOC_EXPAND\s*=.*' = 'TOC_EXPAND             = YES'
            'TOC_NUM_ENTRIES\s*=.*' = 'TOC_NUM_ENTRIES        = 20'
            'GENERATE_QHP\s*=.*' = 'GENERATE_QHP           = YES'
            'QHP_NAMESPACE\s*=.*' = 'QHP_NAMESPACE          = "org.doxygen.Project"'
            'QHP_VIRTUAL_FOLDER\s*=.*' = 'QHP_VIRTUAL_FOLDER     = doc'
            'QHG_LOCATION\s*=.*' = 'QHG_LOCATION           = '
            'GENERATE_ECLIPSEHELP\s*=.*' = 'GENERATE_ECLIPSEHELP   = YES'
            'ECLIPSE_DOC_ID\s*=.*' = 'ECLIPSE_DOC_ID         = org.doxygen.Project'
            'DISABLE_INDEX\s*=.*' = 'DISABLE_INDEX          = NO'
            'GENERATE_TREEVIEW\s*=.*' = 'GENERATE_TREEVIEW      = YES'
            'ENUM_VALUES_PER_LINE\s*=.*' = 'ENUM_VALUES_PER_LINE   = 4'
            'TREEVIEW_WIDTH\s*=.*' = 'TREEVIEW_WIDTH         = 250'
            'EXTENSION_MAPPING\s*=.*' = 'EXTENSION_MAPPING      = hpp=C++ h=C++ cpp=C++ c=C++ hxx=C++ hpp=C++ h++=C++ cxx=C++'
            'MARKDOWN_SUPPORT\s*=.*' = 'MARKDOWN_SUPPORT       = YES'
            'AUTOLINK_SUPPORT\s*=.*' = 'AUTOLINK_SUPPORT       = YES'
            'BUILTIN_STL_SUPPORT\s*=.*' = 'BUILTIN_STL_SUPPORT    = YES'
            'CPP_CLI_SUPPORT\s*=.*' = 'CPP_CLI_SUPPORT        = NO'
            'SIP_SUPPORT\s*=.*' = 'SIP_SUPPORT            = NO'
            'IDL_PROPERTY_SUPPORT\s*=.*' = 'IDL_PROPERTY_SUPPORT   = YES'
            'DISTRIBUTE_GROUP_DOC\s*=.*' = 'DISTRIBUTE_GROUP_DOC   = YES'
            'GROUP_NESTED_COMPOUNDS\s*=.*' = 'GROUP_NESTED_COMPOUNDS = NO'
            'SUBGROUPING\s*=.*' = 'SUBGROUPING            = YES'
            'INLINE_GROUPED_CLASSES\s*=.*' = 'INLINE_GROUPED_CLASSES = NO'
            'INLINE_SIMPLE_STRUCTS\s*=.*' = 'INLINE_SIMPLE_STRUCTS  = NO'
            'TYPEDEF_HIDES_STRUCT\s*=.*' = 'TYPEDEF_HIDES_STRUCT   = NO'
            'LOOKUP_CACHE_SIZE\s*=.*' = 'LOOKUP_CACHE_SIZE      = 0'
            'INPUT\s*=.*' = "INPUT                  = $projectRoot\\include $projectRoot\\src"
            'FILE_PATTERNS\s*=.*' = 'FILE_PATTERNS          = *.h *.hpp *.hxx *.c *.cpp *.cxx *.cc *.dox'
            'RECURSIVE\s*=.*' = 'RECURSIVE              = YES'
            'EXCLUDE\s*=.*' = 'EXCLUDE                = */build/* */third_party/* */external/* */docs/* */tests/*'
            'EXCLUDE_SYMLINKS\s*=.*' = 'EXCLUDE_SYMLINKS       = YES'
            'EXCLUDE_PATTERNS\s*=.*' = 'EXCLUDE_PATTERNS       = */build/* */third_party/* */external/* */docs/* */tests/* */CMakeFiles/* */_deps/*'
            'EXCLUDE_SYMBOLS\s*=.*' = 'EXCLUDE_SYMBOLS        = *::*detail* *::*impl* *::*private* *::*internal* *detail* *impl* *private* *internal*'
            'EXAMPLE_PATH\s*=.*' = 'EXAMPLE_PATH           = '
            'EXAMPLE_PATTERNS\s*=.*' = 'EXAMPLE_PATTERNS       = *'
            'EXAMPLE_RECURSIVE\s*=.*' = 'EXAMPLE_RECURSIVE      = NO'
            'IMAGE_PATH\s*=.*' = 'IMAGE_PATH             = '
            'INPUT_FILTER\s*=.*' = 'INPUT_FILTER           = '
            'FILTER_PATTERNS\s*=.*' = 'FILTER_PATTERNS        = '
            'FILTER_SOURCE_FILES\s*=.*' = 'FILTER_SOURCE_FILES    = NO'
            'FILTER_SOURCE_PATTERNS\s*=.*' = 'FILTER_SOURCE_PATTERNS = '
            'USE_MDFILE_AS_MAINPAGE\s*=.*' = 'USE_MDFILE_AS_MAINPAGE = '
        }
        
        foreach ($key in $replacements.Keys) {
            $doxyContent = $doxyContent -replace $key, $replacements[$key]
        }
        
        # Save the modified Doxyfile
        $doxyContent | Set-Content -Path $doxyfile -Encoding UTF8
    }
    
    # Run Doxygen
    $doxygenArgs = @("$doxyfile")
    
    if ($Verbose) {
        $doxygenArgs += "-d"
        $doxygenArgs += "extcmd"
    }
    
    $process = Start-Process -FilePath "doxygen" -ArgumentList $doxygenArgs -NoNewWindow -PassThru -WorkingDirectory $projectRoot
    $process.WaitForExit()
    
    if ($process.ExitCode -ne 0) {
        Write-Host "Doxygen failed with exit code $($process.ExitCode)" -ForegroundColor $ErrorColor
        exit $process.ExitCode
    }
    
    # Clean up temporary Doxyfile
    if ($tempDoxyfile) {
        Remove-Item -Path $doxyfile -Force
    }
    
    # Generate PDF if requested
    if ($Format -contains "latex" -or $Format -contains "pdf") {
        $latexDir = Join-Path $outputPath "latex"
        
        if (Test-Path $latexDir) {
            Write-Host "Generating PDF documentation..." -ForegroundColor $InfoColor
            
            $makefile = Join-Path $latexDir "Makefile"
            
            if (Test-Path $makefile) {
                Push-Location $latexDir
                
                try {
                    # Run make to generate the PDF
                    $makeProcess = Start-Process -FilePath "make" -ArgumentList @("pdf") -NoNewWindow -PassThru -WorkingDirectory $latexDir
                    $makeProcess.WaitForExit()
                    
                    if ($makeProcess.ExitCode -eq 0) {
                        $pdfFile = Join-Path $latexDir "refman.pdf"
                        $pdfDest = Join-Path $outputPath "documentation.pdf"
                        
                        if (Test-Path $pdfFile) {
                            Copy-Item -Path $pdfFile -Destination $pdfDest -Force
                            Write-Host "PDF documentation generated: $pdfDest" -ForegroundColor $SuccessColor
                        }
                    } else {
                        Write-Host "Failed to generate PDF documentation" -ForegroundColor $ErrorColor
                    }
                } finally {
                    Pop-Location
                }
            }
        }
    }
    
    # Open the documentation if requested
    if ($Open) {
        $indexHtml = Join-Path $outputPath "html\index.html"
        
        if (Test-Path $indexHtml) {
            Start-Process $indexHtml
        } else {
            Write-Host "Could not find documentation index: $indexHtml" -ForegroundColor $WarningColor
        }
    }
    
    Write-Host "Documentation generated in: $outputPath" -ForegroundColor $SuccessColor
} else {
    Write-Host "Doxygen is not available. Cannot generate API documentation." -ForegroundColor $ErrorColor
    exit 1
}
