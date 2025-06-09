# Git Hooks for Cross-Platform Development

## Overview

This project includes Git hooks to ensure code quality across all development platforms (Windows, macOS, and Linux). We offer both standard and advanced pre-commit hooks that validate your code before each commit.

## Available Hooks

### 1. Standard Pre-commit Hook
- **Purpose**: Builds the project and runs critical tests
- **When to use**: For quick validation of basic functionality

### 2. Advanced Cross-Platform Pre-commit Hook
- **Purpose**: More comprehensive validation with static analysis and critical tests
- **Features**:
  - Checks for modifications to critical API files
  - Warns if "not implemented yet" is present in critical files
  - Validates interface compatibility
  - Warns if test files lack critical tests
  - Builds the project and runs critical tests
- **Platform support**: Windows, macOS, and Linux

## Quick Installation

### Standard Hook

```bash
# On Linux/macOS
./hooks/install-hooks.sh

# On Windows (PowerShell)
powershell -File hooks/install-hooks.ps1
```

### Advanced Hook

```bash
# On Linux/macOS
./hooks/install-advanced-hooks.sh

# On Windows (PowerShell)
powershell -File hooks/install-advanced-hooks.ps1
```

## Documentation

For more detailed information, see the [hooks README](hooks/README.md).

## Bypassing Hooks

For exceptional cases, you can bypass the pre-commit hook:

```bash
git commit --no-verify -m "Your commit message"
```

**Note**: Bypassing hooks should be done sparingly and only when necessary. 