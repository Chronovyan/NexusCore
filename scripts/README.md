# Development Scripts

This directory contains various scripts to assist with development, building, and testing of the AI-First Text Editor.

## Available Scripts

### Windows

- `setup_dev.ps1` - Sets up the development environment on Windows
  - Installs required tools (Git, CMake, Visual Studio Build Tools, Python, etc.)
  - Sets up vcpkg and installs dependencies
  - Configures and builds the project
  - Sets up pre-commit hooks

### Build and Test

- `build.ps1` - Builds the project with various configuration options
- `test.ps1` - Runs tests and generates coverage reports
- `format.ps1` - Formats the code using clang-format and other formatters
- `lint.ps1` - Runs static analysis tools

### Code Quality

- `check_format.ps1` - Checks code formatting without making changes
- `analyze.ps1` - Runs static code analysis
- `coverage.ps1` - Generates code coverage reports

### Documentation

- `docs.ps1` - Builds the documentation
- `serve_docs.ps1` - Serves the documentation locally for preview

## Usage

To run any script, open a PowerShell window and navigate to the project root, then run:

```powershell
.\scripts\script_name.ps1 [arguments]
```

## Requirements

- Windows 10/11
- PowerShell 5.1 or later
- Administrator privileges (for setup)

## Adding New Scripts

1. Add your script to the appropriate subdirectory
2. Update this README with a brief description of the script
3. Ensure the script includes proper error handling and help text
4. Test the script on a clean environment

## Best Practices

- Use `$PSScriptRoot` to reference files relative to the script
- Include error handling with `try/catch` blocks
- Use `Write-Verbose` for detailed output
- Add comment-based help to all scripts
- Follow PowerShell coding guidelines

## Examples

### Setting up the development environment

```powershell
# Run as administrator
Set-ExecutionPolicy Bypass -Scope Process -Force
.\scripts\setup_dev.ps1
```

### Building the project

```powershell
.\scripts\build.ps1 -Configuration Release
```

### Running tests

```powershell
.\scripts\test.ps1 -Filter "*unit*"
```

### Formatting code

```powershell
.\scripts\format.ps1 -CheckOnly
```
