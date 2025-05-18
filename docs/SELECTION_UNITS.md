# Selection Units Guide

## Overview

The TextEditor implements a semantic selection model that allows users to work with text at various levels of abstraction beyond simple character ranges. This system enables intelligent text selection based on the logical structure of text and code.

## Selection Unit Types

The editor supports the following selection units, arranged in a hierarchy from smallest to largest:

1. **Character** (`SelectionUnit::Character`)
   - The most basic unit, representing individual characters
   - Used for precise, character-by-character selections
   - Default unit when no selection exists or after clearing a selection

2. **Word** (`SelectionUnit::Word`)
   - Selects whole words at once
   - Words are defined as contiguous alphanumeric characters plus underscores
   - Special characters and whitespace are typically treated as single-character "words"

3. **Expression** (`SelectionUnit::Expression`)
   - Captures code expressions and quoted text
   - Handles parenthesized expressions: `(...)`, `[...]`, `{...}`
   - Handles quoted strings: `"..."`, `'...'`
   - Recognizes nested expressions and finds matching delimiters

4. **Line** (`SelectionUnit::Line`)
   - Selects entire lines of text
   - Selection spans from column 0 to the end of the line

5. **Paragraph** (`SelectionUnit::Paragraph`)
   - Selects a group of related lines (a paragraph)
   - A paragraph is defined as consecutive non-empty lines
   - Empty lines or lines with only whitespace act as paragraph separators

6. **Block** (`SelectionUnit::Block`)
   - Selects code blocks, typically enclosed in curly braces `{...}`
   - Useful for selecting functions, classes, or control structures in code

7. **Document** (`SelectionUnit::Document`)
   - Selects the entire text buffer
   - Equivalent to a "Select All" operation

## Selection Operations

### Expanding Selections

The `expandSelection(SelectionUnit targetUnit)` method allows expanding a selection to a larger semantic unit. The expansion logic follows these rules:

- If no selection exists, a selection is created at the current cursor position
- The selection expands outward to encompass the complete semantic unit containing the cursor
- When expanding from a smaller unit to a larger one, the selection grows to include the entire larger unit
- Multiple calls to expand will move through progressively larger units in the hierarchy

Example sequence:
1. Start with no selection → Select the word under the cursor
2. Expand again → Select the expression containing the word
3. Expand again → Select the entire line containing the expression
4. Expand again → Select the entire paragraph containing the line
5. Expand again → Select the entire document

### Shrinking Selections

The `shrinkSelection(SelectionUnit targetUnit)` method allows reducing a selection to a smaller semantic unit. The shrinking logic follows these rules:

- A selection can only be shrunk if it already exists
- Shrinking moves down the hierarchy to the next smaller unit
- The smaller selection is chosen intelligently from within the current selection
- When no further shrinking is possible, the selection is cleared (returning to character unit)

Example sequence:
1. Start with a document selection → Shrink to a paragraph
2. Shrink again → Select a single line within the paragraph
3. Shrink again → Select a word within the line
4. Shrink again → Clear selection (character level)

## Implementation Details

### Selection Unit Detection

The editor tracks the current selection unit in the `currentSelectionUnit_` member variable. This value is updated whenever a selection is modified through expansion, shrinking, or direct selection methods.

### Selection Coordinates

Selections are tracked using:
- `selectionStartLine_` and `selectionStartCol_`: Starting position
- `selectionEndLine_` and `selectionEndCol_`: Ending position
- `hasSelection_`: Flag indicating if a selection is active

### Helper Methods

The implementation includes several helper methods for working with selections:

- `findWordBoundaries(line, col)`: Determines word boundaries at a position
- `findEnclosingExpression(startPos, endPos)`: Finds expressions that contain the given range
- `findMatchingBracketPair(pos, openBracket, closeBracket)`: Locates matching brackets
- `findEnclosingQuotes(pos, quoteChar)`: Finds quoted strings
- `findEnclosingBracePair(startPos, endPos)`: Locates code blocks in curly braces

## API Reference

### Key Methods

```cpp
// Expand selection to larger semantic unit
void expandSelection(SelectionUnit targetUnit = SelectionUnit::Word);

// Shrink selection to smaller semantic unit
void shrinkSelection(SelectionUnit targetUnit = SelectionUnit::Character);

// Get current selection unit
SelectionUnit getCurrentSelectionUnit() const;

// Specialized expansion methods
bool expandToWord();
bool expandToLine();
bool expandToExpression();
bool expandToBlock();
bool expandToParagraph();
bool expandToDocument();

// Specialized shrinking methods
bool shrinkToCharacter();
bool shrinkToWord();
bool shrinkFromLineToWord();
bool shrinkFromExpressionToWord();
bool shrinkFromParagraphToLine();
bool shrinkFromBlockToLine();
bool shrinkFromDocumentToParagraph();
```

## Usage Examples

### Selecting a Function Call

To select a function call like `calculate(arg1, arg2)`:

1. Place cursor within the function call
2. Call `expandToWord()` or `expandSelection(SelectionUnit::Word)` to select a word (e.g., "arg1")
3. Call `expandToExpression()` or `expandSelection(SelectionUnit::Expression)` to select the entire function call

### Selecting a Code Block

To select a code block like a function body:

1. Place cursor within the code block (between the curly braces)
2. Call `expandToBlock()` or `expandSelection(SelectionUnit::Block)` to select the entire block including braces

### Selecting and Editing Text in Stages

To progressively refine a selection:

1. Select an entire paragraph with `expandToParagraph()`
2. Shrink to a specific line with `shrinkSelection()`
3. Shrink again to a specific word with another `shrinkSelection()` call
4. Edit or replace the selected word 