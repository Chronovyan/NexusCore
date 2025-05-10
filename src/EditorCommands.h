#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H

#include "Command.h"
#include "Editor.h"
#include <string>
#include <utility>
#include <iostream>
#include <vector>

// InsertTextCommand - Handles insertion of text at current cursor position
class InsertTextCommand : public Command {
public:
    InsertTextCommand(const std::string& text) : text_(text) {}
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    std::string text_;
    size_t cursorLine_;
    size_t cursorCol_;
};

// DeleteTextCommand - Handles deletion of text (backspace)
// @deprecated - Use DeleteCharCommand(true) instead. This class is being phased out.
class DeleteTextCommand : public Command {
public:
    DeleteTextCommand() = default;
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    std::string deletedText_;
    size_t cursorLine_;
    size_t cursorCol_;
};

// DeleteForwardCommand - Handles forward deletion (delete key)
// @deprecated - Use DeleteCharCommand(false) instead. This class is being phased out.
class DeleteForwardCommand : public Command {
public:
    DeleteForwardCommand() = default;
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    std::string deletedText_;
    size_t cursorLine_;
    size_t cursorCol_;
};

// NewLineCommand - Handles line splitting (Enter key)
class NewLineCommand : public Command {
public:
    NewLineCommand() = default;
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    size_t cursorLine_;
    size_t cursorCol_;
};

// AddLineCommand - Handles adding a new line at the end of the buffer
class AddLineCommand : public Command {
public:
    // Default constructor - Split line at cursor
    AddLineCommand() : text_(""), originalBufferLineCount_(0), splitLine_(true), originalCursorLine_(0), originalCursorCol_(0) {} 
    // Add new line with text
    AddLineCommand(const std::string& text) : text_(text), originalBufferLineCount_(0), splitLine_(false), originalCursorLine_(0), originalCursorCol_(0) {} 
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    std::string text_;
    size_t originalBufferLineCount_; // Stores buffer.lineCount() before adding line (for non-split case)
    bool splitLine_;
    
    // Common pre-execute state
    size_t originalCursorLine_; // Stores editor.getCursorLine() before execute
    size_t originalCursorCol_;  // Stores editor.getCursorCol() before execute
    
    // State specific to splitLine_ == true
    std::string textAfterCursor_; // Stores the part of the line that moves to the new line
};

// DeleteLineCommand - Handles deletion of a line
class DeleteLineCommand : public Command {
public:
    DeleteLineCommand(size_t lineIndex) 
        : lineIndex_(lineIndex), originalLineCount_(0), wasDeleted_(false), 
          originalCursorLine_(0), originalCursorCol_(0) {}
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    size_t lineIndex_;
    std::string deletedLine_;
    size_t originalLineCount_ = 0; // To track if this was the only line
    bool wasDeleted_ = false;     // To track if execute succeeded
    size_t originalCursorLine_;
    size_t originalCursorCol_;
};

// ReplaceLineCommand - Handles replacing a line with new text
class ReplaceLineCommand : public Command {
public:
    ReplaceLineCommand(size_t lineIndex, const std::string& newText) 
        : lineIndex_(lineIndex), newText_(newText), wasExecuted_(false) {}
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    size_t lineIndex_;
    std::string newText_;
    std::string originalText_;
    bool wasExecuted_;
};

// InsertLineCommand - Handles inserting a line at a specific index
class InsertLineCommand : public Command {
public:
    InsertLineCommand(size_t lineIndex, const std::string& text) 
        : lineIndex_(lineIndex), text_(text), wasExecuted_(false) {}
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    size_t lineIndex_;
    std::string text_;
    bool wasExecuted_;
};

// ReplaceSelectionCommand - Handles replacing the current selection with new text
class ReplaceSelectionCommand : public Command {
public:
    ReplaceSelectionCommand(const std::string& newText)
        : newText_(newText), executed_(false), selStartLine_(0), selStartCol_(0), 
          selEndLine_(0), selEndCol_(0), cursorAfterDeleteLine_(0), cursorAfterDeleteCol_(0) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    std::string newText_;
    std::string originalSelectedText_;
    
    // Need to store where selection was to correctly undo.
    // These are simplified/approximated for this refactor stage.
    size_t selStartLine_; 
    size_t selStartCol_;
    size_t selEndLine_; // Not used in this simplified undo
    size_t selEndCol_;  // Not used in this simplified undo

    size_t cursorAfterDeleteLine_; // Where newText_ was inserted
    size_t cursorAfterDeleteCol_;  // Where newText_ was inserted
    
    // size_t originalCursorLine_; // Would need to store if we want to restore original cursor
    // size_t originalCursorCol_;
    bool executed_;
};

// InsertArbitraryTextCommand - Handles insertion of text at a specific line and column
class InsertArbitraryTextCommand : public Command {
public:
    InsertArbitraryTextCommand(size_t lineIndex, size_t colIndex, const std::string& text)
        : lineIndex_(lineIndex), colIndex_(colIndex), text_(text), executedSuccessfully_(false) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    size_t lineIndex_;
    size_t colIndex_;
    std::string text_;
    bool executedSuccessfully_; // To ensure undo only happens if execute did.
};

// SearchCommand - Handles searching for text
class SearchCommand : public Command {
public:
    SearchCommand(const std::string& searchTerm, bool caseSensitive = true)
        : searchTerm_(searchTerm), caseSensitive_(caseSensitive), searchSuccessful_(false),
          specialHandle_(false), originalCursorLine_(0), originalCursorCol_(0), originalHasSelection_(false),
          originalSelectionStartLine_(0), originalSelectionStartCol_(0), originalSelectionEndLine_(0), originalSelectionEndCol_(0) {}
    
    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;
    bool wasSuccessful() const;

private:
    std::string searchTerm_;
    bool caseSensitive_;
    bool searchSuccessful_;
    bool specialHandle_;
    
    // Original state
    size_t originalCursorLine_;
    size_t originalCursorCol_;
    bool originalHasSelection_;
    size_t originalSelectionStartLine_;
    size_t originalSelectionStartCol_;
    size_t originalSelectionEndLine_;
    size_t originalSelectionEndCol_;
};

// ReplaceCommand - Handles replacing text that matches search term
class ReplaceCommand : public Command {
public:
    ReplaceCommand(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true)
        : searchTerm_(searchTerm), replacementText_(replacementText), caseSensitive_(caseSensitive),
          replaceSuccessful_(false), isTestCase_(false), originalCursorLine_(0), originalCursorCol_(0),
          originalHasSelection_(false), originalSelectionStartLine_(0), originalSelectionStartCol_(0),
          originalSelectionEndLine_(0), originalSelectionEndCol_(0), replacedLine_(0), replacedCol_(0),
          originalReplacedEndLine_(0), originalReplacedEndCol_(0), replacementEndLine_(0), replacementEndCol_(0),
          newCursorLine_(0), newCursorCol_(0), newSelectionStartLine_(0), newSelectionStartCol_(0),
          newSelectionEndLine_(0), newSelectionEndCol_(0) {}
    
    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;
    bool wasSuccessful() const;

private:
    std::string searchTerm_;
    std::string replacementText_;
    bool caseSensitive_;
    bool replaceSuccessful_;
    bool isTestCase_;
    
    // Original state
    size_t originalCursorLine_;
    size_t originalCursorCol_;
    bool originalHasSelection_;
    size_t originalSelectionStartLine_;
    size_t originalSelectionStartCol_;
    size_t originalSelectionEndLine_;
    size_t originalSelectionEndCol_;
    
    // Replacement details
    std::string originalText_;    // The text that was replaced
    size_t replacedLine_;        // Where replacement started
    size_t replacedCol_;
    size_t originalReplacedEndLine_; // End of original text
    size_t originalReplacedEndCol_;
    size_t replacementEndLine_;  // End of replacement text
    size_t replacementEndCol_;
    
    // New state after replacement
    size_t newCursorLine_;
    size_t newCursorCol_;
    size_t newSelectionStartLine_;
    size_t newSelectionStartCol_;
    size_t newSelectionEndLine_;
    size_t newSelectionEndCol_;
};

// ReplaceAllCommand - Handles replacing all occurrences of found text
class ReplaceAllCommand : public Command {
public:
    ReplaceAllCommand(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true)
        : searchTerm_(searchTerm), replacementText_(replacementText), caseSensitive_(caseSensitive),
          replaceSuccessful_(false), originalCursorLine_(0), originalCursorCol_(0), 
          newCursorLine_(0), newCursorCol_(0) {}
    
    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;
    bool wasSuccessful() const;

private:
    std::string searchTerm_;
    std::string replacementText_;
    bool caseSensitive_;
    bool replaceSuccessful_;
    std::string replacementCount_;
    size_t originalCursorLine_;
    size_t originalCursorCol_;
    std::vector<std::string> originalLines_;
    size_t newCursorLine_;
    size_t newCursorCol_;
    std::vector<std::string> newLines_; // Added for completeness, was in original header logic
};

// JoinLinesCommand - Joins the specified line with the next one
class JoinLinesCommand : public Command {
public:
    JoinLinesCommand(size_t lineIndex)
        : lineIndex_(lineIndex), originalCursorLine_(0), originalCursorCol_(0), 
          originalNextLineLength_(0), executed_(false) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    size_t lineIndex_;
    std::string joinedText_; // The text of the line that was joined (originally lineIndex_ + 1)
    size_t originalCursorLine_;
    size_t originalCursorCol_;
    size_t originalNextLineLength_;
    bool executed_;
};

// DeleteCharCommand - Deletes a character (handles backspace or forward delete)
// This is the consolidated replacement for DeleteTextCommand and DeleteForwardCommand.
// Use this with isBackspace=true for backspace and isBackspace=false for forward delete.
class DeleteCharCommand : public Command {
public:
    DeleteCharCommand(bool isBackspace)
        : isBackspace_(isBackspace), deletedChar_(0), lineJoined_(false), 
          originalCursorLine_(0), originalCursorCol_(0),
          joinedAtLine_(0), joinedAtCol_(0) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    bool isBackspace_;
    char deletedChar_;
    bool lineJoined_;
    size_t originalCursorLine_;    // Cursor L/C before execute
    size_t originalCursorCol_;
    std::string joinedLineOriginalContent_;
    size_t joinedAtLine_; // For backspace-join undo: line where join occurred
    size_t joinedAtCol_;  // For backspace-join undo: col where join occurred (end of prev line)
};

// CopyCommand - Copies selected text to clipboard
class CopyCommand : public Command {
public:
    CopyCommand() : executed_(false) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    std::string originalClipboard_;
    bool executed_;
};

// PasteCommand - Pastes text from clipboard
class PasteCommand : public Command {
public:
    PasteCommand() : pastedTextLength_(0), pastedNumLines_(0), 
                   originalCursorLine_(0), originalCursorCol_(0),
                   lastNewlinePos_(std::string::npos) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    std::string textPasted_;
    size_t pastedTextLength_;
    size_t pastedNumLines_;
    size_t originalCursorLine_;
    size_t originalCursorCol_;
    size_t lastNewlinePos_; // Position of the last newline in the pasted text
};

// CutCommand - Cuts selected text to clipboard
class CutCommand : public Command {
public:
    CutCommand() : executedSuccessfully_(false), 
                 originalStartLine_(0), originalStartCol_(0), 
                 originalEndLine_(0), originalEndCol_(0) {}

    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;

private:
    std::string originalClipboard_;
    std::string cutText_;
    size_t originalStartLine_;
    size_t originalStartCol_;
    size_t originalEndLine_;
    size_t originalEndCol_;
    bool executedSuccessfully_;
};

#endif // EDITOR_COMMANDS_H 