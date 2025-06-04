#!/bin/bash
#
# Script to update the CI status badge in the README
# Run this script after setting up the GitHub Actions workflow

# Colors for output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
NC="\033[0m" # No Color

echo "${YELLOW}Updating CI status badge in README.md...${NC}"

# Get the repository name from remote URL
REPO_URL=$(git config --get remote.origin.url)
REPO_NAME=$(echo $REPO_URL | sed -e 's/.*github.com[:\/]\([^ ]*\)\.git/\1/')

if [ -z "$REPO_NAME" ]; then
    echo "${RED}Could not determine repository name from git config.${NC}"
    echo "${YELLOW}Please update the badge URL manually in README.md with your actual repository name.${NC}"
    exit 1
fi

# Update the badge URL in README.md
sed -i "s|github.com/yourusername/AI-First-TextEditor/actions|github.com/${REPO_NAME}/actions|g" README.md

# Check if the replacement was successful
if grep -q "github.com/${REPO_NAME}/actions" README.md; then
    echo "${GREEN}✅ Successfully updated badge URL in README.md${NC}"
    echo "New badge URL points to: ${REPO_NAME}"
else
    echo "${RED}Failed to update badge URL in README.md${NC}"
    echo "${YELLOW}Please update it manually with your actual repository name.${NC}"
    exit 1
fi

echo "${GREEN}Badge URL updated. The badge will be visible once the first workflow run is completed.${NC}" 