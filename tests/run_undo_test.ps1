# This script runs the ManualUndoRedoTest.cpp with Visual Studio environment
# Based on the approach from PowerShell Cookbook

param(
    # Visual Studio installation path (adjust if needed)
    [string]$VsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
)

# Function to run a batch file and capture its environment variables
function Invoke-CmdScript {
    param(
        [string]$scriptPath,
        [string]$arguments = ""
    )
    
    Write-Host "Setting up environment using $scriptPath"
    
    $tempFile = [IO.Path]::GetTempFileName()
    
    # Run the batch file and capture the environment variables
    cmd /c " `"$scriptPath`" $arguments && set > `"$tempFile`" "
    
    # Import the environment variables into the current PowerShell session
    Get-Content $tempFile | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") {
            Set-Content "env:\$($matches[1])" $matches[2]
        }
    }
    
    Remove-Item $tempFile
    Write-Host "Environment setup complete."
}

# Check if Visual Studio is installed
$vcvarsPath = Join-Path -Path $VsPath -ChildPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvarsPath)) {
    Write-Error "Visual Studio environment setup file not found at: $vcvarsPath"
    exit 1
}

# Set up VS environment
Invoke-CmdScript $vcvarsPath

# Make sure we're in the tests directory
Set-Location -Path $PSScriptRoot

# Create bin directory if it doesn't exist
if (-not (Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

# Compile the test
Write-Host "Compiling ManualUndoRedoTest.cpp..."
$compileResult = cmd /c "cl.exe /EHsc /std:c++17 /W4 /I..\src ManualUndoRedoTest.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp /Febin\UndoRedoTest.exe 2>&1"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed:"
    Write-Host $compileResult
    exit 1
}

# Run the test
Write-Host "Running UndoRedoTest.exe..."
.\bin\UndoRedoTest.exe 