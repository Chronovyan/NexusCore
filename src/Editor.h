#ifndef EDITOR_H
#define EDITOR_H

#include "TextBuffer.h"
#include "CommandManager.h"
#include "SyntaxHighlighter.h"
#include "SyntaxHighlightingManager.h"
#include "interfaces/IEditor.hpp"
#include "interfaces/ITextBuffer.hpp"
#include "interfaces/ICommandManager.hpp"
#include "interfaces/ISyntaxHighlightingManager.hpp"
#include "interfaces/IDiffEngine.hpp"
#include "interfaces/IMergeEngine.hpp"
#include "interfaces/IMultiCursor.hpp"
#include "MultiCursor.h"
#include "AIAgentOrchestrator.h"
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <limits> // For std::numeric_limits
#include <memory> // For std::shared_ptr
#include <vector>
#include <optional>

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

class Editor : public IEditor {
public:
    // Constructor without dependencies - for backward compatibility
    Editor();
    
    // Constructor with dependencies injected
    Editor(
        std::shared_ptr<ITextBuffer> textBuffer,
        std::shared_ptr<ICommandManager> commandManager,
        std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager,
        std::shared_ptr<IDiffEngine> diffEngine = nullptr,
        std::shared_ptr<IMergeEngine> mergeEngine = nullptr
    );
    
    ~Editor() = default;

    // File Operations
    bool openFile(const std::string& filename) override;
    bool saveFile() override; // Uses current filename
    bool saveFileAs(const std::string& filename) override; // Uses specified filename
    bool isModified() const override { return modified_; } // Already existed, ensure it's used consistently
    void setModified(bool modified) override { modified_ = modified; }
    bool loadFile(const std::string& filename);

    // Terminal/Display Dimension (stubbed for now, to be implemented properly)
    int getTerminalWidth() const;  // To be implemented
    int getTerminalHeight() const; // To be implemented

    // Cursor management
    void setCursor(size_t line, size_t col) override;
    size_t getCursorLine() const override;
    size_t getCursorCol() const override;

    // Cursor Movement
    void moveCursorUp() override;
    void moveCursorDown() override;
    void moveCursorLeft() override;
    void moveCursorRight() override;
    void moveCursorToLineStart() override;
    void moveCursorToLineEnd() override;
    void moveCursorToBufferStart() override;
    void moveCursorToBufferEnd() override;
    void moveCursorToNextWord();
    void moveCursorToPrevWord();

    // Editor API Extensions
    std::string getFileExtension() const; // Returns the extension of the current file
    bool isNewFile() const; // Returns true if the file is new/untitled and unmodified
    std::string getCurrentLineText() const; // Returns the text of the current line
    bool isCursorAtLineStart() const; // Returns true if cursor is at the start of the line
    bool isCursorAtLineEnd() const; // Returns true if cursor is at the end of the line
    bool isCursorAtBufferStart() const; // Returns true if cursor is at the start of the buffer
    bool isCursorAtBufferEnd() const; // Returns true if cursor is at the end of the buffer
    size_t getViewportStartLine() const; // Returns the first visible line in the viewport
    size_t getViewportHeight() const; // Returns the height of the viewport in lines
    std::string getWordUnderCursor() const; // Returns the word at the current cursor position

    void printView(std::ostream& os) const;
    void printLineWithHighlighting(std::ostream& os, const std::string& line, const std::vector<SyntaxStyle>& styles) const;
    void applyColorForSyntaxColor(std::ostream& os, SyntaxColor color) const;
    void printStatusBar(std::ostream& os) const;
    void positionCursor();

    // Buffer access methods
    ITextBuffer& getBuffer() override { return *textBuffer_; }
    const ITextBuffer& getBuffer() const override { return *textBuffer_; }
    
    // Access to dependencies
    std::shared_ptr<ITextBuffer> getTextBuffer() const { return textBuffer_; }
    std::shared_ptr<ICommandManager> getCommandManager() const { return commandManager_; }
    std::shared_ptr<ISyntaxHighlightingManager> getSyntaxHighlightingManager() const { return syntaxHighlightingManager_; }

    void addLine(const std::string& text) override;
    void insertLine(size_t lineIndex, const std::string& text) override;
    virtual void deleteLine(size_t lineIndex) override; // Made virtual to allow overriding in TestEditor
    void replaceLine(size_t lineIndex, const std::string& text) override;

    // Text editing operations (higher level, often use commands)
    void typeText(const std::string& textToInsert) override; // Uses InsertTextCommand
    void typeChar(char charToInsert) override;  // Uses InsertTextCommand with a single char
    void processCharacterInput(char ch) override; // Processes single character input using TypeTextCommand
    void deleteSelection() override;  // Uses DeleteSelectionCommand
    void deleteCharacter();           // Implements delete key functionality
    void backspace() override;        // Uses BackspaceCommand
    void deleteForward() override;    // Uses DeleteCommand
    virtual void newLine() override;          // Uses NewLineCommand, made virtual to allow overriding in TestEditor
    virtual void joinWithNextLine() override; // Uses JoinLinesCommand, made virtual to allow overriding in TestEditor
    // Add wordwise variants (deleteWordBackward, deleteWordForward) if needed
    
    // Indentation operations
    void increaseIndent() override;
    void decreaseIndent() override;

    // Undo/redo operations
    bool canUndo() const override { return commandManager_->canUndo(); }
    bool canRedo() const override { return commandManager_->canRedo(); }
    bool undo() override;
    bool redo() override;

    // Selection operations
    bool hasSelection() const override;
    void clearSelection() override;
    virtual void setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) override;
    virtual std::string getSelectedText() const override;
    virtual void startSelection() override;
    virtual void updateSelection() override;
    virtual void endSelection();
    virtual void replaceSelection(const std::string& text) override;
    void selectLine() override; // Selects the entire current line
    void selectAll() override; // Selects the entire buffer content
    void selectToLineStart(); // Extends selection from cursor to start of line
    void selectToLineEnd(); // Extends selection from cursor to end of line
    void expandSelection(SelectionUnit targetUnit = SelectionUnit::Word);
    virtual void shrinkSelection(SelectionUnit targetUnit = SelectionUnit::Word) override;
    SelectionUnit getCurrentSelectionUnit() const;
    
    // Selection coordinate getters
    size_t getSelectionStartLine() const;
    size_t getSelectionStartCol() const;
    size_t getSelectionEndLine() const;
    size_t getSelectionEndCol() const;
    
    // Clipboard operations
    virtual void cutSelection() override;
    virtual void copySelection() override;
    virtual void pasteAtCursor() override;
    virtual std::string getClipboardText() const override;
    virtual void setClipboardText(const std::string& text) override;
    
    // For backwards compatibility
    virtual void setSelectionStart() { startSelection(); }
    virtual void setSelectionEnd() { updateSelection(); }
    virtual void copySelectedText() { copySelection(); }
    virtual void cutSelectedText() { cutSelection(); }
    virtual void pasteText() { pasteAtCursor(); }
    
    // Search operations
    bool search(const std::string& searchTerm, bool caseSensitive = true, bool forward = true) override;
    bool searchPrevious() override;

    // Word operations (can be grouped with text editing or selection)
    virtual void deleteWord();
    void selectWord();

    // Search and replace operations
    bool searchNext() override;
    bool findNext(const std::string& pattern);
    bool replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true) override;
    bool replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true) override;

    // Syntax highlighting methods
    virtual void enableSyntaxHighlighting(bool enable = true) override;
    bool isSyntaxHighlightingEnabled() const override;
    void setFilename(const std::string& filename) override;
    std::string getFilename() const override;
    void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter);
    void detectAndSetHighlighter();
    void invalidateHighlightingCache();
    void applyColorForSyntaxColor(SyntaxColor color);

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

    std::shared_ptr<SyntaxHighlighter> getCurrentHighlighter() const override;
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles();
    virtual std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const override;
    
    // Multiple cursor operations
    bool isMultiCursorEnabled() const override;
    void setMultiCursorEnabled(bool enable) override;
    size_t getCursorCount() const override;
    bool addCursor(size_t line, size_t col) override;
    bool removeCursor(size_t line, size_t col) override;
    void removeAllSecondaryCursors() override;
    size_t addCursorsAtAllOccurrences(const std::string& text, bool caseSensitive = true) override;
    size_t addCursorsAtColumn(size_t startLine, size_t endLine, size_t column) override;
    IMultiCursor& getMultiCursor() override;
    const IMultiCursor& getMultiCursor() const override;
    
    // Helper methods for indentation commands
    void setLine(size_t lineIndex, const std::string& text);
    void setCursorPosition(const Position& pos);
    
    // Helper method for selection handling
    std::string getSingleSelectionText() const;

    // Text Selection Helper methods
    bool isWhitespace(char c) const;
    bool isWordChar(char c) const;
    bool isSpecialChar(char c) const;
    
    // Position from line and column
    Position getCursorPosition() const;
    
    // Moves cursor to a new position
    void validateAndClampCursor();
    
    // Page up/down
    void pageUp();
    void pageDown();
    
    // Find word boundaries (for word navigation/selection)
    std::pair<size_t, size_t> findWordBoundaries(size_t line, size_t col) const;
    std::pair<size_t, size_t> findWordStart(size_t line, size_t col) const;
    std::pair<size_t, size_t> findWordEnd(size_t line, size_t col) const;
    
    // Expression handling (needed for code-aware selection expansion)
    bool handleSelectionExpansionToExpression(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    bool expandSelectionToExpression(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    bool expandSelectionToParagraph(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    bool expandSelectionToBlock(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    bool expandSelectionToDocument(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    
    // Helper for scroll position management
    void ensureCursorVisible();
    
    void updateVisibleRange();
    
    // Selection Expansion Helpers
    bool expandSelectionToWord(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    bool expandSelectionToLine(size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol);
    
    // Syntax highlighting cache utilities
    void updateHighlightingCache();
    bool isHighlightingCacheValid() const;

    // Method to extract pure text content from a line (no formatting)
    std::string getLineContentForHighlighting(size_t lineIndex) const;

    // For debugging and testing
    void printBuffer(std::ostream& os) const; // Print buffer content to stream

    // Method to handle different line ending types in the buffer
    void normalizeLineEndings();

    // Structure to represent semantic boundaries for expressions
    struct ExpressionBoundary {
        Position start;
        Position end;
        bool found;
        
        ExpressionBoundary() : found(false) {}
        ExpressionBoundary(const Position& s, const Position& e) : start(s), end(e), found(true) {}
    };
    
    ExpressionBoundary findEnclosingExpression(const Position& startPos, const Position& endPos) const;
    ExpressionBoundary findMatchingBracketPair(const Position& pos, char openBracket, char closeBracket) const;
    ExpressionBoundary findEnclosingQuotes(const Position& pos, char quoteChar) const;
    char getMatchingBracket(char bracket) const;
    bool isOpeningBracket(char c) const;
    bool isClosingBracket(char c) const;
    bool isQuoteChar(char c) const;
    
    // Additional helpers for larger syntax structures
    Position findPreviousOpeningBrace(const Position& pos) const;
    ExpressionBoundary scanForEnclosingBraces(const Position& startPos, const Position& endPos) const;
    ExpressionBoundary findEnclosingBracePair(const Position& startPos, const Position& endPos) const;
    
    // Helper for position comparisons
    int comparePositions(const Position& a, const Position& b) const;
    
    friend class Command; 
    friend class CompoundCommand;
    
    friend class TestEditor; 

    // Diff and Merge Operations
    bool showDiff(const std::vector<std::string>& text1, const std::vector<std::string>& text2) override;
    bool diffWithCurrent(const std::vector<std::string>& otherText) override;
    bool diffWithFile(const std::string& filename) override;
    bool mergeTexts(
        const std::vector<std::string>& base,
        const std::vector<std::string>& ours,
        const std::vector<std::string>& theirs) override;
    bool mergeWithFile(const std::string& theirFile, const std::string& baseFile) override;
    bool applyDiffChanges(
        const std::vector<DiffChange>& changes,
        const std::vector<std::string>& sourceText) override;
    bool resolveConflict(
        size_t conflictIndex,
        MergeConflictResolution resolution,
        const std::vector<std::string>& customResolution = {}) override;

    // AI Agent Orchestrator integration
    /**
     * @brief Set the AI Agent Orchestrator
     * 
     * @param orchestrator The orchestrator to use
     */
    void setAIAgentOrchestrator(std::shared_ptr<ai_editor::AIAgentOrchestrator> orchestrator);

protected:
    // The injected dependencies
    std::shared_ptr<ITextBuffer> textBuffer_;
    std::shared_ptr<ICommandManager> commandManager_;
    std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager_;
    
    size_t cursorLine_ = 0;
    size_t cursorCol_ = 0;
    
    // Selection state
    bool hasSelection_ = false; // Actual state variable
    size_t selectionStartLine_ = 0;
    size_t selectionStartCol_ = 0;
    size_t selectionEndLine_ = 0;
    size_t selectionEndCol_ = 0;
    SelectionUnit currentSelectionUnit_ = SelectionUnit::Character;
    
    std::string clipboard_;
    
    std::string filename_;
    
    std::string currentSearchTerm_;
    bool currentSearchCaseSensitive_ = true;
    
    size_t lastSearchLine_ = 0;
    size_t lastSearchCol_ = 0;
    bool searchWrapped_ = false;
    bool highlightingStylesCacheValid_ = false;
    bool modified_ = false;
    
    bool syntaxHighlightingEnabled_ = false;
    std::shared_ptr<SyntaxHighlighter> currentHighlighter_ = nullptr;
    
    // Multiple cursor support
    std::unique_ptr<MultiCursor> multiCursor_;
    bool multiCursorEnabled_ = false;
    
    // Viewing configuration and screen metrics
    size_t topVisibleLine_ = 0;     // First visible line in the editor window
    size_t viewableLines_ = 20;     // Number of lines that can be displayed at once
    int commandLineHeight_ = 1;     // Height of command line in rows
    int statusLineHeight_ = 1;      // Height of status line in rows
    int displayWidth_ = 80;         // Width of display in columns
    int displayHeight_ = 24;        // Height of display in rows

    // Viewport management
    void scrollDown();
    void scrollUp();
    void scrollToLine(size_t line);
    size_t getTopVisibleLine() const { return topVisibleLine_; }
    size_t getBottomVisibleLine() const;
    size_t getViewableLines() const { return viewableLines_; }
    
    // Helper methods for expanding selection
    void updateSelectionUnit(SelectionUnit unit);
    
    // Other helper methods
    char getCharAtCursor() const;
    
    // Command history
    std::vector<std::string> commandHistory_;
    size_t commandHistoryIndex_ = 0;
    
    // Helper methods for initialization
    void initialize();

    // Helper accessor for backwards compatibility
    TextBuffer& getTextBuffer() { 
        // Try to cast the textBuffer_ to a TextBuffer
        TextBuffer* concreteBuffer = dynamic_cast<TextBuffer*>(textBuffer_.get());
        if (!concreteBuffer) {
            throw std::runtime_error("TextBuffer cast failed - interface implementation is not TextBuffer");
        }
        return *concreteBuffer;
    }
    
    // Diff and Merge Components
    std::shared_ptr<IDiffEngine> diffEngine_;
    std::shared_ptr<IMergeEngine> mergeEngine_;
    MergeResult currentMergeResult_; // Stores the current merge result
    
    // Helper methods for diff and merge operations
    std::vector<std::string> getCurrentTextAsLines() const;
    bool loadTextFromFile(const std::string& filename, std::vector<std::string>& lines);
    
    // Update code context in the AI Agent Orchestrator
    void updateCodeContext();
    
    // AI Agent Orchestrator for context-aware assistance
    std::shared_ptr<ai_editor::AIAgentOrchestrator> aiAgentOrchestrator_;

private:
    // Add any private members or methods here
};

#endif // EDITOR_H 