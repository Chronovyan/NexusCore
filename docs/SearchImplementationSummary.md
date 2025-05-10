# Search & Replace: Technical Summary

Brief overview of the search and replace functionality's core technical components.

## Core Implementation Components

### 1. `Editor` Class Search API:
   - `search(const std::string& searchTerm, bool caseSensitive = true)`: Initiates search.
   - `searchNext()`: Finds subsequent match.
   - `replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true)`: Replaces current/next match.
   - `replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true)`: Replaces all matches.

### 2. Key Helper Logic:
   - `findMatchInLine(...)`: Low-level function to find a term within a single line, handling case sensitivity and returning match position/length.

### 3. State Management (within `Editor` or a dedicated Search module):
   - `currentSearchTerm_`: Active search query.
   - `currentSearchCaseSensitive_`: Boolean flag for case sensitivity.
   - `lastSearchPosition_` (e.g., line/col): Tracks end of the last match for `searchNext()` resumption.
   - `searchWrapped_`: Flag indicating if search has wrapped around the document.

### 4. Command Pattern Integration:
   - `ReplaceSelectionCommand`: Encapsulates a single replacement operation for undo/redo.
   - `CompoundCommand`: Groups multiple `ReplaceSelectionCommand` instances for `replaceAll` to ensure atomicity in undo/redo.
   - Command Manager updated to handle these commands.

### 5. Selection Integration:
   - Search results are visually highlighted by leveraging the existing text selection mechanism.

*(For user-facing commands, see `docs/SearchFeature.md` or the main `README.md`.)* 