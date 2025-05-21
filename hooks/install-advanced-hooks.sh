#!/bin/bash
#
# Script to install advanced cross-platform Git hooks
#

# Colors for output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
NC="\033[0m" # No Color

echo "${YELLOW}Installing advanced cross-platform pre-commit hooks...${NC}"

# Get the root directory of the git repository
REPO_ROOT=$(git rev-parse --show-toplevel)

# Make sure the .git/hooks directory exists
HOOKS_DIR="$(git rev-parse --git-dir)/hooks"
mkdir -p "$HOOKS_DIR"

# Copy the hook files
cp "${REPO_ROOT}/hooks/advanced-pre-commit.ps1" "$HOOKS_DIR/"
cp "${REPO_ROOT}/hooks/advanced-pre-commit.sh" "$HOOKS_DIR/"
cp "${REPO_ROOT}/hooks/advanced-pre-commit-wrapper" "$HOOKS_DIR/"

# Make the shell scripts executable on Unix-like systems
chmod +x "$HOOKS_DIR/advanced-pre-commit.sh" "$HOOKS_DIR/advanced-pre-commit-wrapper"

# Set the wrapper as the pre-commit hook
cp "$HOOKS_DIR/advanced-pre-commit-wrapper" "$HOOKS_DIR/pre-commit"
chmod +x "$HOOKS_DIR/pre-commit"

echo "${GREEN}✅ Advanced pre-commit hooks installed successfully.${NC}"
echo "${YELLOW}The hooks will automatically use:${NC}"
echo "  - PowerShell script on Windows"
echo "  - Bash script on Linux/macOS"
echo "  - PowerShell Core (if available) on any platform"
echo ""
echo "${YELLOW}To bypass these hooks for a single commit (not recommended):${NC}"
echo "  git commit --no-verify -m \"Your commit message\"" 