# Search and Replace Functionality

The text editor now includes powerful search and replace capabilities, allowing you to find and modify text efficiently.

## Features

### Basic Search

- **Find text**: Search for specific text within the document
- **Case-sensitive search**: Control whether searches match exact case or not
- **Selection highlighting**: Found matches are automatically selected
- **Search navigation**: Easily move between search results

### Replace Capabilities

- **Single replacement**: Replace the current match with new text
- **Replace all**: Replace all occurrences of the search term throughout the document
- **Undo support**: All replacements can be undone

## Commands

| Command | Description |
|---------|-------------|
| `search <text>` or `find <text>` | Search for text in the document |
| `searchnext` or `findnext` | Move to the next occurrence of the search term |
| `replace <search_term> <replacement_text>` | Find and replace the first occurrence |
| `replaceall <search_term> <replacement_text>` | Replace all occurrences in the document |

## Implementation Details

The search functionality is implemented with these key components:

1. **Search state variables** in the Editor class:
   - `currentSearchTerm_` - The current search string
   - `currentSearchCaseSensitive_` - Whether search is case-sensitive
   - `lastSearchLine_` and `lastSearchCol_` - Position of last match
   - `searchWrapped_` - Tracks if search has wrapped around document

2. **String matching algorithm**:
   - For case-sensitive search, direct string comparison
   - For case-insensitive search, conversion to lowercase before comparison

3. **Selection integration**:
   - Found matches are automatically selected
   - Allows for easy visual identification of matches
   - Enables seamless integration with clipboard operations

## Example Usage

```
> add Hello, world!
> add This is a test with multiple words.
> add Another line with the test in it.

> search test
Found match. Cursor at: [2, 10]

> searchnext
Found next match. Cursor at: [3, 22]

> replace test example
Replaced text. Cursor at: [3, 29]

> replaceall test example
Replaced all occurrences. 