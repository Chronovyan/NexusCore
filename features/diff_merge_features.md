# Text Diff and Merge Capabilities

This document provides an overview of the text diff and merge capabilities implemented in the AI-First TextEditor.

## Overview

The diff and merge capabilities allow users to:

1. Compare the current document with another file
2. View differences highlighted with color coding
3. Merge changes from different versions of a document
4. Resolve merge conflicts interactively

## Core Components

### Diff Engine

The diff engine (`IDiffEngine` interface, `MyersDiff` implementation) is responsible for:

- Computing the differences between two texts
- Identifying added, removed, and modified lines
- Generating a list of changes that can be applied to transform one text into another

### Merge Engine

The merge engine (`IMergeEngine` interface, `MergeEngine` implementation) handles:

- Three-way merging (base, ours, theirs)
- Conflict detection and resolution
- Generating merged output with conflict markers

### Editor Integration

The diff and merge capabilities are integrated into the editor through:

- Extensions to the `IEditor` interface
- New commands for diff and merge operations
- DI framework integration for dependency management

## Usage

### Comparing Documents

To compare the current document with another file:

```cpp
// Using the editor directly
editor->diffWithFile("path/to/other/file.txt");

// Using commands
auto diffCmd = EditorDiffMergeCommands::createDiffWithFileCommand(editor, "path/to/other/file.txt");
diffCmd->execute();
```

### Merging Documents

To merge the current document with another version:

```cpp
// Using the editor directly
editor->mergeWithFile("path/to/base.txt", "path/to/theirs.txt");

// Using commands
auto mergeCmd = EditorDiffMergeCommands::createMergeWithFilesCommand(
    editor, "path/to/base.txt", "path/to/theirs.txt");
mergeCmd->execute();
```

### Resolving Conflicts

When conflicts are detected during a merge, they can be resolved using:

```cpp
// Example of resolving a conflict by choosing "ours"
std::vector<MergeConflictResolution> resolutions = {
    {0, ConflictResolutionType::CHOOSE_OURS}
};
editor->resolveConflict(resolutions);
```

## Implementation Details

### Diff Algorithm

The implementation uses the Myers diff algorithm, which:

- Finds the shortest edit script between two sequences
- Is optimal for typical text editing scenarios
- Handles line-based diffing effectively

### Merge Strategy

The merge strategy follows these principles:

1. Non-conflicting changes from both sources are included automatically
2. Conflicting changes are marked and presented to the user
3. The user can choose which version to keep or manually edit the conflict

## Future Enhancements

Planned enhancements include:

- Word-level diff highlighting
- Interactive conflict resolution UI
- Patch file generation and application
- Integration with version control systems 