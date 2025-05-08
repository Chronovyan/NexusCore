#ifndef EDITOR_H
#define EDITOR_H

#include "TextBuffer.h"
#include "CommandManager.h"
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <limits> // For std::numeric_limits

class Editor {
public:
    Editor();

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

    // Display with cursor (conceptual)
    // For now, this might just print the buffer and then the cursor position separately,
    // or try a very simple inline indication.
    void printView(std::ostream& os) const;

    // TextBuffer access (for now, direct access for commands in main)
    // We might later wrap these with Editor methods that also handle cursor updates.
    TextBuffer& getBuffer(); 
    const TextBuffer& getBuffer() const;

    // Editor-level operations that might involve cursor logic later
    // For now, many will pass through to TextBuffer but might be expanded.
    void addLine(const std::string& text);
    void insertLine(size_t lineIndex, const std::string& text);
    void deleteLine(size_t lineIndex);
    void replaceLine(size_t lineIndex, const std::string& text);

    // Text editing operations
    void typeText(const std::string& textToInsert);
    void typeChar(char ch); 
    void backspace();
    void deleteForward();
    void newLine(); // Enter key
    void joinWithNextLine();

    // Selection and clipboard operations
    void setSelectionStart(); // Mark current cursor position as selection start
    void setSelectionEnd();   // Mark current cursor position as selection end
    bool hasSelection() const;
    void clearSelection();
    std::string getSelectedText() const;
    void deleteSelectedText();
    void copySelectedText();
    void cutSelectedText();
    void pasteText();

    // Word operations
    void deleteWord();
    void selectWord();
    
    // Undo/Redo operations
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Search operations
    bool search(const std::string& searchTerm, bool caseSensitive = true);
    bool searchNext();
    bool replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);
    bool replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);

protected:
    TextBuffer buffer_;
    size_t cursorLine_;
    size_t cursorCol_;
    
    // Selection state
    bool hasSelection_;
    size_t selectionStartLine_;
    size_t selectionStartCol_;
    size_t selectionEndLine_;
    size_t selectionEndCol_;
    
    // Clipboard
    std::string clipboard_;
    
    // Command manager for undo/redo
    CommandManager commandManager_;
    
    // Search state
    std::string currentSearchTerm_;
    bool currentSearchCaseSensitive_ = true;
    size_t lastSearchLine_ = 0;
    size_t lastSearchCol_ = 0;
    bool searchWrapped_ = false;

    // Helper to validate and clamp cursor position
    void validateAndClampCursor();
    
    // Helper for word boundaries
    bool isWordChar(char c) const;
    
    // Helper for search operations
    bool findMatchInLine(const std::string& line, const std::string& term, 
                        size_t startPos, bool caseSensitive, size_t& matchPos, size_t& matchLength);
};

#endif // EDITOR_H 