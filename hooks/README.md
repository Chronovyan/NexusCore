# Git Hooks

This directory contains Git hooks for the project.

## Pre-commit Hooks

We offer two versions of pre-commit hooks:

1. **Standard Pre-commit Hook** (`pre-commit`): A simple hook that builds the project and runs critical tests.
2. **Advanced Pre-commit Hook**: A more comprehensive hook with additional static checks and validations.

### Installation Instructions

#### Option 1: Standard Pre-commit Hook

```bash
# Copy the hook to the Git hooks directory
cp hooks/pre-commit .git/hooks/pre-commit
# Make it executable
chmod +x .git/hooks/pre-commit
```

Or use the installer script:
```bash
# On Linux/macOS
./hooks/install-hooks.sh

# On Windows (PowerShell)
powershell -File hooks/install-hooks.ps1
```

#### Option 2: Advanced Cross-platform Pre-commit Hook

Use the dedicated installer scripts:

```bash
# On Linux/macOS
./hooks/install-advanced-hooks.sh

# On Windows (PowerShell)
powershell -File hooks/install-advanced-hooks.ps1
```

Alternatively, manual installation:
```bash
# For Windows, Linux, and macOS
# Step 1: Copy the scripts to the Git hooks directory
cp hooks/advanced-pre-commit.ps1 hooks/advanced-pre-commit.sh hooks/advanced-pre-commit-wrapper .git/hooks/
# Step 2: Make them executable (Linux/macOS)
chmod +x .git/hooks/advanced-pre-commit.sh .git/hooks/advanced-pre-commit-wrapper
# Step 3: Set the wrapper as the pre-commit hook
cp .git/hooks/advanced-pre-commit-wrapper .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

### Features of the Advanced Pre-commit Hook

The advanced pre-commit hook performs the following checks:

1. Checks for modifications to critical API implementation files
2. Warns if "not implemented yet" is present in critical files
3. Checks interface compatibility (basic signature check)
4. Warns if test files were modified but no critical tests added
5. Builds the project and runs critical tests

### Cross-Platform Support

The advanced pre-commit hook automatically uses the appropriate script based on your platform:
- PowerShell script on Windows
- Bash script on Linux/macOS
- PowerShell Core (if available) on any platform

This ensures consistent pre-commit checks across all development environments.

### Bypass Pre-commit Hooks

To bypass the pre-commit hook for a specific commit (not recommended for regular use):

```bash
git commit --no-verify -m "Your commit message"
``` 