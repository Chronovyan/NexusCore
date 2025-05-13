#ifndef EDITOR_H
#define EDITOR_H

#include "TextBuffer.h"
#include "CommandManager.h"
#include "SyntaxHighlighter.h"
#include "SyntaxHighlightingManager.h"
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <limits> // For std::numeric_limits
#include <memory> // For std::shared_ptr

// Position struct to represent cursor or selection position
struct Position {
    size_t line;
    size_t column;
    
    bool operator==(const Position& other) const {
        return line == other.line && column == other.column;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

class Editor {
public:
    Editor();
    ~Editor() = default;

    // File Operations
    bool openFile(const std::string& filename);
    bool saveFile(const std::string& filename = ""); // filename optional, uses internal if empty
    bool isModified() const { return modified_; } // Already existed, ensure it's used consistently
    void setModified(bool modified) { modified_ = modified; }

    // Terminal/Display Dimension (stubbed for now, to be implemented properly)
    int getTerminalWidth() const;  // To be implemented
    int getTerminalHeight() const; // To be implemented

    // Cursor management
    virtual void setCursor(size_t line, size_t col);
    size_t getCursorLine() const;
    size_t getCursorCol() const;

    // Cursor Movement
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

    void printView(std::ostream& os) const;

    TextBuffer& getBuffer() { return buffer_; }
    const TextBuffer& getBuffer() const { return buffer_; }

    void addLine(const std::string& text);
    void insertLine(size_t lineIndex, const std::string& text);
    void deleteLine(size_t lineIndex); // This was a direct editor method, distinct from DeleteLineCommand
    void replaceLine(size_t lineIndex, const std::string& text);

    // Text editing operations (higher level, often use commands)
    void typeText(const std::string& textToInsert); // Uses InsertTextCommand
    void typeChar(char charToInsert);  // Uses InsertTextCommand with a single char
    void deleteSelection();  // Uses DeleteSelectionCommand
    void backspace();        // Uses BackspaceCommand
    void deleteForward();    // Uses DeleteCommand
    void newLine();          // Uses NewLineCommand
    void joinWithNextLine(); // Uses JoinLinesCommand
    // Add wordwise variants (deleteWordBackward, deleteWordForward) if needed
    
    // Indentation operations
    void increaseIndent();
    void decreaseIndent();

    // Undo/redo operations
    bool canUndo() const { return commandManager_.canUndo(); }
    bool canRedo() const { return commandManager_.canRedo(); }
    bool undo();
    bool redo();

    // Selection operations
    bool hasSelection() const;
    void clearSelection();
    void setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
    std::string getSelectedText() const;
    void startSelection();
    void updateSelection();
    void replaceSelection(const std::string& text);
    void selectLine(); // Selects the entire current line
    void selectAll(); // Selects the entire buffer content
    
    // Selection coordinate getters
    size_t getSelectionStartLine() const;
    size_t getSelectionStartCol() const;
    size_t getSelectionEndLine() const;
    size_t getSelectionEndCol() const;
    
    // Clipboard operations
    void cutSelection();
    void copySelection();
    void pasteAtCursor();
    std::string getClipboardText() const;
    void setClipboardText(const std::string& text);
    
    // For backwards compatibility
    void setSelectionStart() { startSelection(); }
    void setSelectionEnd() { updateSelection(); }
    void copySelectedText() { copySelection(); }
    void cutSelectedText() { cutSelection(); }
    void pasteText() { pasteAtCursor(); }
    
    // Search operations
    bool search(const std::string& searchTerm, bool caseSensitive, bool forward);
    bool searchPrevious();

    // Word operations (can be grouped with text editing or selection)
    void deleteWord();
    void selectWord();

    // Search and replace operations
    bool searchNext();
    bool replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);
    bool replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);

    // Syntax highlighting methods
    virtual void enableSyntaxHighlighting(bool enable = true);
    bool isSyntaxHighlightingEnabled() const;
    void setFilename(const std::string& filename);
    std::string getFilename() const;

    // --- Potentially public helpers for Commands (revisit access level later) ---
    void directDeleteTextRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
    void directInsertText(size_t line, size_t col, const std::string& text, size_t& outEndLine, size_t& outEndCol);
    bool performReplaceLogic(
        const std::string& searchTerm, 
        const std::string& replacementText, 
        bool caseSensitive, 
        std::string& outOriginalText, 
        size_t& outReplacedAtLine, 
        size_t& outReplacedAtCol,
        size_t& outOriginalEndLine,
        size_t& outOriginalEndCol
    );

    // Internal search logic - moved to public for Command access
    bool performSearchLogic(
        const std::string& searchTerm, 
        bool caseSensitive, 
        bool forward,
        size_t& outFoundLine, 
        size_t& outFoundCol
    );
    // --- End Potentially public helpers ---

    virtual void detectAndSetHighlighter();
    std::shared_ptr<SyntaxHighlighter> getCurrentHighlighter() const;
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles();
    virtual std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const;
    
    // Invalidate syntax highlighting cache after buffer modifications
    void invalidateHighlightingCache();

    // Helper methods for indentation commands
    void setLine(size_t lineIndex, const std::string& text);
    std::string getLine(size_t lineIndex) const;
    bool isSelectionActive() const { return hasSelection_; }
    void setSelection(const Position& start, const Position& end);
    void setCursorPosition(const Position& pos);

protected:
    // Helper methods
    bool isWordChar(char c) const;
    void validateAndClampCursor(); // Makes sure cursor is within valid bounds
    std::string constructLine(const std::string&) const; // For view (stub for now)
    
    // Helper for search functionality
    bool findMatchInLine(const std::string& line, const std::string& term,
                         size_t startPos, bool caseSensitive, 
                         size_t& outMatchPos, size_t& outMatchLength);

    void updateHighlightingCache();

    // Modify buffer and cursor, and invalidate highlighting for range
    void updateBufferWith(const std::string& text, size_t startLine, size_t startCol, 
                         size_t endLine, size_t endCol, 
                         int cursorLineDelta, int cursorColDelta);

    // Viewport/Display related (placeholders)
    size_t getTopVisibleLine() const { return topVisibleLine_; }
    void setTopVisibleLine(size_t line) { topVisibleLine_ = line; }
    size_t getVisibleLineCount() const { return viewableLines_; }

    friend class Command; 
    friend class CompoundCommand;
    // Consider making TestEditor a friend if it needs to poke at internals not exposed by public API.
    friend class TestEditor; 

protected:
    TextBuffer buffer_;
    CommandManager commandManager_;
    SyntaxHighlightingManager syntaxHighlightingManager_;
    
    size_t cursorLine_ = 0;
    size_t cursorCol_ = 0;
    
    // Selection state
    bool hasSelection_ = false; // Actual state variable
    size_t selectionStartLine_ = 0;
    size_t selectionStartCol_ = 0;
    size_t selectionEndLine_ = 0;
    size_t selectionEndCol_ = 0;
    
    std::string clipboard_;
    
    // Search state
    std::string currentSearchTerm_;
    bool currentSearchCaseSensitive_ = true;
    // bool currentSearchForward_ = true; // This was in .bat, but not in .h. Let's assume SearchCommand carries this.
    size_t lastSearchLine_ = 0;
    size_t lastSearchCol_ = 0;
    bool searchWrapped_ = false;
    
    // Syntax highlighting state
    bool syntaxHighlightingEnabled_ = true;
    std::string filename_ = "untitled.txt"; // Initialized
    std::shared_ptr<SyntaxHighlighter> currentHighlighter_ = nullptr; // Changed to shared_ptr 
    mutable std::vector<std::vector<SyntaxStyle>> cachedHighlightStyles_;
    mutable bool highlightingStylesCacheValid_ = false; // Renamed from highlightingCacheValid_ for clarity
    
    // Modified flag
    bool modified_ = false;
    
    // Display dimensions (for view calculations)
    int displayWidth_ = 80;
    int displayHeight_ = 24;
    size_t topVisibleLine_ = 0;
    size_t viewableLines_ = 22; // Default, calculated in constructor based on display height
    size_t commandLineHeight_ = 1;
    size_t statusLineHeight_ = 1;
};

#endif // EDITOR_H 