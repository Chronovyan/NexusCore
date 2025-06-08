#!/bin/bash

# Create a new release of the project
# This script will:
# 1. Check for uncommitted changes
# 2. Bump the version number
# 3. Update the changelog
# 4. Create a git tag
# 5. Push everything to the remote repository
#
# Usage: ./scripts/create-release.sh [major|minor|patch]

set -e

# Check if we're in a git repository
if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    echo "Error: Not a git repository"
    exit 1
fi

# Check for uncommitted changes
if ! git diff-index --quiet HEAD --; then
    echo "Error: You have uncommitted changes. Please commit or stash them before creating a release."
    exit 1
fi

# Ensure we're on the main branch
CURRENT_BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null)
if [ "$CURRENT_BRANCH" != "main" ] && [ "$CURRENT_BRANCH" != "master" ]; then
    echo "Error: You must be on the main or master branch to create a release."
    exit 1
fi

# Get the current version from CMakeLists.txt
CURRENT_VERSION=$(grep -m 1 "set\(VERSION " CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')
if [ -z "$CURRENT_VERSION" ]; then
    echo "Error: Could not determine current version from CMakeLists.txt"
    exit 1
fi

echo "Current version: $CURRENT_VERSION"

# Determine version bump type
BUMP_TYPE=$1
if [ -z "$BUMP_TYPE" ]; then
    echo "Usage: $0 [major|minor|patch]"
    exit 1
fi

# Bump version
IFS='.' read -r -a VERSION_PARTS <<< "$CURRENT_VERSION"
MAJOR=${VERSION_PARTS[0]}
MINOR=${VERSION_PARTS[1]}
PATCH=${VERSION_PARTS[2]}

case $BUMP_TYPE in
    major)
        NEW_MAJOR=$((MAJOR + 1))
        NEW_VERSION="$NEW_MAJOR.0.0"
        ;;
    minor)
        NEW_MINOR=$((MINOR + 1))
        NEW_VERSION="$MAJOR.$NEW_MINOR.0"
        ;;
    patch)
        NEW_PATCH=$((PATCH + 1))
        NEW_VERSION="$MAJOR.$MINOR.$NEW_PATCH"
        ;;
    *)
        echo "Error: Invalid version bump type. Use 'major', 'minor', or 'patch'."
        exit 1
        ;;
esac

echo "New version: $NEW_VERSION"

# Update version in CMakeLists.txt
sed -i.bak "s/set(VERSION $CURRENT_VERSION)/set(VERSION $NEW_VERSION)/" CMakeLists.txt
rm -f CMakeLists.txt.bak

# Update changelog
TODAY=$(date +'%Y-%m-%d')
CHANGELOG_ENTRY="## [$NEW_VERSION] - $TODAY"

# Get the latest commits for the changelog
LATEST_COMMITS=$(git log --pretty=format:"- %s" v$CURRENT_VERSION..HEAD | grep -v "^$" | grep -v "^Merge" | sort | uniq)

# Create a temporary changelog
TEMP_CHANGELOG=$(mktemp)
echo "# Changelog" > "$TEMP_CHANGELOG"
echo "" >> "$TEMP_CHANGELOG"
echo "$CHANGELOG_ENTRY" >> "$TEMP_CHANGELOG"
echo "" >> "$TEMP_CHANGELOG"
if [ -n "$LATEST_COMMITS" ]; then
    echo "$LATEST_COMMITS" >> "$TEMP_CHANGELOG"
    echo "" >> "$TEMP_CHANGELOG"
fi

tail -n +3 CHANGELOG.md >> "$TEMP_CHANGELOG"
mv "$TEMP_CHANGELOG" CHANGELOG.md

# Commit changes
git add CMakeLists.txt CHANGELOG.md
git commit -m "chore: bump version to $NEW_VERSION"

# Create and push tag
git tag -a "v$NEW_VERSION" -m "Version $NEW_VERSION"

echo ""
echo "✅ Release $NEW_VERSION is ready!"
echo ""
echo "To publish the release, run:"
echo "  git push origin $CURRENT_BRANCH --tags"
