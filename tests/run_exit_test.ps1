# Function to run a command script and capture its environment variables
function Invoke-CmdScript {
    param(
        [String] $scriptName
    )
    $cmdLine = """$scriptName"" & set"
    & $env:SystemRoot\system32\cmd.exe /c $cmdLine |
    Select-String '^([^=]*)=(.*)$' | ForEach-Object {
        $varName = $_.Matches[0].Groups[1].Value
        $varValue = $_.Matches[0].Groups[2].Value
        Set-Item Env:$varName $varValue
    }
}

# Find Visual Studio installation and setup environment
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
if (-not $vsPath) {
    Write-Host "Visual Studio not found. Using alternative paths..."
    # Try common paths for VS 2019 or 2022
    $possiblePaths = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path "$path\VC\Auxiliary\Build\vcvars64.bat") {
            $vsPath = $path
            break
        }
    }
    
    if (-not $vsPath) {
        Write-Host "Visual Studio not found in common paths. Please install Visual Studio with C++ development tools."
        exit 1
    }
}

# Setup VS environment variables
$vcvarsPath = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
Write-Host "Setting up VS environment from: $vcvarsPath"
Invoke-CmdScript $vcvarsPath

# Compile the test with Visual Studio compiler
$srcDir = "..\src"
$compiler = "cl.exe"
$sourceFile = ".\ExitTest.cpp"
$outputExe = ".\ExitTest.exe"
$includes = "/I.\ /I$srcDir"
$sourceFiles = "$sourceFile $srcDir\Editor.cpp $srcDir\TextBuffer.cpp"
$compileOptions = "/EHsc /std:c++17 /W4"

# Compile command
$compileCmd = "$compiler $compileOptions $includes $sourceFiles /Fe:$outputExe"
Write-Host "Compiling with command: $compileCmd"
Invoke-Expression $compileCmd

# Check if compilation succeeded
if ($LASTEXITCODE -eq 0) {
    Write-Host "Compilation successful. Running test..."
    # Run the test
    & $outputExe
} else {
    Write-Host "Compilation failed with exit code $LASTEXITCODE"
} 