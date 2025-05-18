# TextEditor Project Outline

## 1. Project Overview

TextEditor is a C++17 text editing application designed with a focus on stability, performance, and core editing functionality. It features a modular architecture with clean separation between the text buffer management, command processing, and editor interface layers. The project follows a command pattern design, allowing for robust undo/redo functionality.

## 2. File Structure

### Core Components

```
/src
├── Editor.h/cpp               # Main editor class with core editing operations
├── TextBuffer.h/cpp           # Text content storage and manipulation
├── Command.h                  # Command interface for editor operations
├── CommandManager.h           # Manages command history for undo/redo
├── EditorCommands.h/cpp       # Implementation of specific editor commands
├── EditorError.h/cpp          # Error handling and reporting infrastructure
├── SyntaxHighlighter.h/cpp    # Syntax highlighting implementation
├── SyntaxHighlightingManager.h/cpp # Management of syntax highlighters
├── ThreadSafetyConfig.h       # Thread safety utilities and configuration
└── main.cpp                   # Entry point for the application
```

### Testing Framework

```
/tests
├── RunAllTests.cpp            # Test runner for all unit tests
├── TestEditor.h               # Test implementation of Editor for validation
├── TestUtilities.h            # Test helper functions and utilities
├── TextBufferTest.cpp         # Tests for TextBuffer functionality 
├── editor_commands_test.cpp   # Tests for editor commands
├── editor_facade_test.cpp     # Tests for Editor class API
├── syntax_highlighting_manager_test.cpp # Tests for syntax highlighting
├── command_*_test.cpp         # Various command-specific tests
└── ...                        # Additional test files
```

### Documentation

```
/docs
├── BUILD_AND_TEST.md          # Build and testing instructions
├── STABILITY.md               # Stability principles and guidelines
├── THREAD_SAFETY.md           # Thread safety patterns and practices
├── REFINEMENTS.md             # Code refinement best practices
├── SYNTAX_HIGHLIGHTING_*.md   # Documentation for syntax highlighting
└── ...                        # Additional documentation files
```

## 3. Features

### Core Editing

- **Text Buffer Management**
  - Storage and manipulation of text content
  - Line-based operations (add, insert, delete, replace)
  - Direct access to specific lines or ranges

- **Cursor Management**
  - Cursor positioning and navigation
  - Validation and clamping within buffer boundaries
  - Movement operations (up, down, left, right, line start/end, buffer start/end)
  - Word navigation (next word, previous word)

- **Selection Operations**
  - Text selection with start and end positions
  - Copy, cut, and paste functionality
  - Selection expansion by semantic units (character, word, line, expression, paragraph, block, document)
  - Selection shrinking by semantic units

- **Text Editing**
  - Character and text insertion at cursor position
  - Deletion (backspace, forward delete)
  - Word deletion 
  - Line operations (newline, join lines)
  - Indentation (increase/decrease)

### Command System

- **Command Pattern Implementation**
  - Execute and undo functionality for all operations
  - Command history management
  - Undo/redo stack with proper memory management

- **Compound Commands**
  - Grouping of multiple operations into atomic transactions
  - All-or-nothing execution and rollback

### Search and Replace

- **Text Search**
  - Forward and backward search
  - Case-sensitive and case-insensitive options
  - Search result selection

- **Replace Operations**
  - Single occurrence replacement
  - Replace all occurrences
  - Validation and proper cursor positioning

### File Operations

- **File I/O**
  - Open and load files
  - Save and save-as functionality
  - File modification tracking

### Syntax Highlighting

- **Language Support**
  - C++ syntax highlighting
  - Extensible framework for additional languages

- **Highlighting Features**
  - Tokenization and pattern matching
  - Multi-line token support
  - Caching and performance optimizations
  - Style customization

## 4. API Endpoints

### TextBuffer API

```cpp
// Core buffer operations
void addLine(const std::string& text);
void insertLine(size_t lineIndex, const std::string& text);
void deleteLine(size_t lineIndex);
void setLine(size_t lineIndex, const std::string& text);
std::string getLine(size_t lineIndex) const;

// Buffer information
size_t lineCount() const;
bool isEmpty() const;
void clear(bool keepEmptyLine = true);

// File operations
bool loadFromFile(const std::string& filename);
bool saveToFile(const std::string& filename);
```

### Editor API

```cpp
// Cursor management
void setCursor(size_t line, size_t col);
size_t getCursorLine() const;
size_t getCursorCol() const;

// Cursor movement
void moveCursorUp();
void moveCursorDown();
void moveCursorLeft();
void moveCursorRight();
void moveCursorToLineStart();
void moveCursorToLineEnd();
void moveCursorToBufferStart();
void moveCursorToBufferEnd();
void moveCursorToNextWord();
void moveCursorToPrevWord();

// Text editing
void typeText(const std::string& textToInsert);
void typeChar(char charToInsert);
void backspace();
void deleteForward();
void newLine();
void joinWithNextLine();
void deleteWord();

// Selection operations
bool hasSelection() const;
void clearSelection();
void setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
std::string getSelectedText() const;
void startSelection();
void updateSelection();
void replaceSelection(const std::string& text);
void selectLine();
void selectAll();
void selectToLineStart();
void selectToLineEnd();
void expandSelection(SelectionUnit targetUnit = SelectionUnit::Word);
void shrinkSelection(SelectionUnit targetUnit = SelectionUnit::Character);
SelectionUnit getCurrentSelectionUnit() const;

// Clipboard operations
void cutSelection();
void copySelection();
void pasteAtCursor();
std::string getClipboardText() const;
void setClipboardText(const std::string& text);

// Indentation operations
void increaseIndent();
void decreaseIndent();

// Search and replace
bool search(const std::string& searchTerm, bool caseSensitive = true, bool forward = true);
bool searchNext();
bool searchPrevious();
bool replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);
bool replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);

// File operations
bool openFile(const std::string& filename);
bool saveFile();
bool saveFile(const std::string& filename);
std::string getFilename() const;
void setFilename(const std::string& filename);
bool isModified() const;

// Syntax highlighting
void enableSyntaxHighlighting(bool enable = true);
bool isSyntaxHighlightingEnabled() const;
std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const;
void detectAndSetHighlighter();
std::shared_ptr<SyntaxHighlighter> getCurrentHighlighter() const;

// Undo/redo operations
bool canUndo() const;
bool canRedo() const;
bool undo();
bool redo();
```

### CommandManager API

```cpp
// Command execution and history management
template <typename CmdT, typename... Args>
void executeCommand(Args&&... args, Editor& editor);
void executeCommand(std::unique_ptr<Command> command, Editor& editor);
bool canUndo() const;
bool canRedo() const;
void undo(Editor& editor);
void redo(Editor& editor);
void clear();
```

### SyntaxHighlightingManager API

```cpp
// Syntax highlighting management
void setBuffer(TextBuffer* buffer);
void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter);
void setEnabled(bool enabled);
bool isEnabled() const;
void setVisibleRange(size_t startLine, size_t endLine);
void invalidateCache();
void invalidateLine(size_t lineIndex);
std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const;
```

## 5. Selection Units

The editor supports various selection units for expanding and shrinking selections:

- `Character`: Individual character selection
- `Word`: Word-level selection (alphanumeric + underscore)
- `Expression`: Expression-level selection (e.g., function call, parenthesized expression)
- `Line`: Line-level selection (entire line)
- `Paragraph`: Paragraph-level selection (consecutive non-empty lines)
- `Block`: Block-level selection (code blocks in curly braces)
- `Document`: Entire document selection

## 6. Command-Line Interface

The application provides a command-line interface with the following commands:

```
load <filename>                           Load file into buffer
save <filename>                           Save buffer to file
add <text>                                Add text as a new line at the end
insert <line_idx> <text>                  Insert text at specified line index
delete <line_idx>                         Delete line at specified index
view                                      Print buffer with cursor
setcursor <line> <col>                    Set cursor position
type <text>                               Insert text at cursor
search/find <term>                        Find text
replace <search_term> <replace_text>      Find and replace first occurrence
replaceall <search_term> <replace_text>   Replace all occurrences
help                                      Show all commands
quit/exit                                 Exit the editor
```

## 7. Future Development

According to the project roadmap, future development will focus on:

1. **Finalizing Core Backend**: Completing error handling refinements and command system optimizations
2. **Testing & Validation**: Comprehensive testing for stability and performance
3. **GUI Integration**: Developing a responsive and intuitive graphical interface
4. **Final Polishing**: Documentation, optimization, and release preparation 