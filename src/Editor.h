#ifndef EDITOR_H
#define EDITOR_H

#include "TextBuffer.h"
#include "CommandManager.h"
#include "SyntaxHighlighter.h"
#include "SyntaxHighlightingManager.h"
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <limits> // For std::numeric_limits

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
    void typeChar(char ch); 
    void backspace(); // Uses DeleteCharCommand
    void deleteForward(); // Uses DeleteCharCommand
    void newLine(); // Uses AddLineCommand
    void joinWithNextLine(); // Uses JoinLinesCommand

    // Selection methods
    void setSelectionStart(); 
    void setSelectionEnd();   
    bool hasSelection() const; // Public method to check selection state
    void clearSelection();
    std::string getSelectedText() const;
    void deleteSelectedText(); // Uses a CompoundCommand with DeleteCharCommands or similar
    void setSelectionRange(size_t sl, size_t sc, size_t el, size_t ec);
    size_t getSelectionStartLine() const { return selectionStartLine_; }
    size_t getSelectionStartCol() const { return selectionStartCol_; }
    size_t getSelectionEndLine() const { return selectionEndLine_; }
    size_t getSelectionEndCol() const { return selectionEndCol_; }

    // Clipboard operations (higher level, use commands)
    void copySelectedText();  // Uses CopyCommand
    void cutSelectedText();   // Uses CutCommand
    void pasteText();         // Uses PasteCommand
    std::string getClipboardText() const { return clipboard_; } // Public getter
    void setClipboardText(const std::string& text) { clipboard_ = text; } // Public setter

    // Word operations
    void deleteWord();
    void selectWord();
    
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Search operations (higher level)
    bool search(const std::string& searchTerm, bool caseSensitive = true); // Uses SearchCommand for first find
    bool searchNext(); // Uses SearchCommand with isNewSearch=false
    bool replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true); // Uses ReplaceCommand
    bool replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true); // Uses ReplaceAllCommand
    
    void enableSyntaxHighlighting(bool enable = true);
    bool isSyntaxHighlightingEnabled() const;
    void setFilename(const std::string& filename); // Sets filename and tries to detect highlighter
    std::string getFilename() const;
    void detectAndSetHighlighter();
    SyntaxHighlighter* getCurrentHighlighter() const;
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const;
    void invalidateHighlightingCache(); // MOVED HERE
    
    // Helpers for direct buffer manipulation (used by performReplaceLogic and ReplaceCommand)
    // These are likely okay to be public if commands need them and are not friends for this.
    void directDeleteTextRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
    void directInsertText(size_t line, size_t col, const std::string& text, size_t& outEndLine, size_t& outEndCol);

    // Internal logic for search and replace, called by commands (public for now as commands are not all friends yet for this)
    bool performSearchLogic(const std::string& searchTerm, bool caseSensitive, bool isNewSearch);
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
    bool syntaxHighlightingEnabled_ = false;
    std::string filename_ = "untitled.txt"; // Initialized
    SyntaxHighlighter* currentHighlighter_ = nullptr; // Initialized
    mutable std::vector<std::vector<SyntaxStyle>> cachedHighlightStyles_;
    mutable bool highlightingStylesCacheValid_ = false; // Renamed from highlightingCacheValid_ for clarity
    void updateHighlightingCache() const; // Kept protected

    // Viewport, Display Dimensions, and modification status
    size_t topVisibleLine_ = 0;
    bool modified_ = false;
    size_t commandLineHeight_ = 1;   // Added
    size_t statusLineHeight_ = 1;    // Added
    size_t displayWidth_ = 80;       // Added (default, will be calculated)
    size_t displayHeight_ = 24;      // Added (default, will be calculated)
    size_t viewableLines_ = 22;      // Added (default, will be calculated: displayHeight - cmd - status)

    // Helper to validate and clamp cursor position
    void validateAndClampCursor();
    
    // Helper for word boundaries
    bool isWordChar(char c) const;
    
    // Helper for search operations
    bool findMatchInLine(const std::string& line, const std::string& term, 
                        size_t startPos, bool caseSensitive, size_t& matchPos, size_t& matchLength);
};

#endif // EDITOR_H 