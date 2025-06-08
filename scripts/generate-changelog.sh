#!/bin/bash

# Generate a changelog from git history
# This script generates a changelog in Markdown format based on git commits
# It groups commits by type (feat, fix, etc.) and includes the commit messages

set -e

# Configuration
CHANGELOG_FILE="CHANGELOG.md"
TEMP_FILE=$(mktemp)
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")

# If no tags exist, get all commits
if [ -z "$LATEST_TAG" ]; then
    COMMIT_RANGE=""
else
    COMMIT_RANGE="$LATEST_TAG..HEAD"
fi

# Get all commits in the specified range
COMMITS=$(git log $COMMIT_RANGE --pretty=format:"%s" --no-merges)

# Initialize changelog
{
    echo "# Changelog"
    echo ""
    echo "## [Unreleased]"
    echo ""
} > "$TEMP_FILE"

# Function to categorize commits
categorize_commits() {
    local type=$1
    local emoji=""
    local title=""
    
    case $type in
        feat)       emoji="✨" title="New Features" ;;
        fix)        emoji="🐛" title="Bug Fixes" ;;
        docs)       emoji="📚" title="Documentation" ;;
        style)      emoji="💅" title="Code Style" ;;
        refactor)   emoji="♻️"  title="Code Refactoring" ;;
        perf)       emoji="⚡" title="Performance Improvements" ;;
        test)       emoji="✅" title="Tests" ;;
        chore)      emoji="🔧" title="Chores" ;;
        ci)         emoji="👷" title="CI/CD" ;;
        revert)     emoji="⏪" title="Reverts" ;;
        *)          return ;;
    esac
    
    local commits=$(echo "$COMMITS" | grep -i "^$type(\|^$type: " || true)
    
    if [ -n "$commits" ]; then
        echo "### $emoji $title"
        echo ""
        
        while IFS= read -r commit; do
            # Extract the message part (after type: )
            local message=$(echo "$commit" | sed -E "s/^$type(:\s*|\(.*\):\s*)(.+)/\2/")
            # Capitalize first letter
            message=$(echo "$message" | sed 's/^./\u&/')
            echo "- $message"
        done <<< "$commits"
        
        echo ""
    fi
}

# Generate changelog sections
categorize_commits "feat"
categorize_commits "fix"
categorize_commits "perf"
categorize_commits "refactor"
categorize_commits "style"
categorize_commits "test"
categorize_commits "docs"
categorize_commits "chore"
categorize_commits "ci"
categorize_commits "revert"

# If there are no changes, add a note
if [ $(wc -l < "$TEMP_FILE") -le 4 ]; then
    echo "- No significant changes" >> "$TEMP_FILE"
    echo "" >> "$TEMP_FILE"
fi

# If we have a previous changelog, append it
if [ -f "$CHANGELOG_FILE" ] && [ -n "$LATEST_TAG" ]; then
    # Find the line after the first heading
    local line_num=$(grep -n "^## " "$CHANGELOG_FILE" | head -n 2 | tail -n 1 | cut -d: -f1)
    if [ -n "$line_num" ]; then
        tail -n +"$line_num" "$CHANGELOG_FILE" >> "$TEMP_FILE"
    fi
fi

# Update the changelog file
mv "$TEMP_FILE" "$CHANGELOG_FILE"

echo "Changelog generated successfully!"
echo "View it at: $CHANGELOG_FILE"
