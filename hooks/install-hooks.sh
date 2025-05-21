#!/bin/bash
#
# Script to install Git hooks
#

# Colors for output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
NC="\033[0m" # No Color

echo "${YELLOW}Installing Git hooks...${NC}"

# Get the directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HOOKS_DIR="$(git rev-parse --git-dir)/hooks"

# Copy the pre-commit hook
cp "$SCRIPT_DIR/pre-commit" "$HOOKS_DIR/pre-commit"
chmod +x "$HOOKS_DIR/pre-commit"

echo "${GREEN}✅ Successfully installed pre-commit hook to $HOOKS_DIR${NC}"

# Detect if we're on Windows
if [ "$(uname)" = "Windows_NT" ] || [ -n "$WINDIR" ]; then
    echo "${YELLOW}Windows detected. Installing PowerShell pre-commit hook...${NC}"
    cp "$SCRIPT_DIR/pre-commit.ps1" "$HOOKS_DIR/pre-commit.ps1"
    
    # Create shell wrapper for PowerShell script
    if [ ! -f "$HOOKS_DIR/pre-commit" ]; then
        echo '#!/bin/sh' > "$HOOKS_DIR/pre-commit"
        echo 'powershell.exe -ExecutionPolicy Bypass -File .git/hooks/pre-commit.ps1' >> "$HOOKS_DIR/pre-commit"
        echo 'exit $?' >> "$HOOKS_DIR/pre-commit"
        chmod +x "$HOOKS_DIR/pre-commit"
    fi
    
    echo "${GREEN}✅ Successfully installed PowerShell pre-commit hook${NC}"
fi

echo "${GREEN}Git hooks installed successfully. Your commits will now be tested automatically.${NC}" 