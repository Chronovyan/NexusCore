#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H

#include "Command.h"
#include "Editor.h"
#include "interfaces/ITextBuffer.hpp"
#include <string>
#include <utility>
#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <fstream>

// InsertTextCommand - Handles insertion of text at current cursor position
class InsertTextCommand : public Command {
public:
    // Default constructor - Uses current cursor position
    InsertTextCommand(const std::string& text) : text_(text), useSpecifiedPosition_(false) {}
    
    // Constructor with explicit line and column position
    InsertTextCommand(const std::string& text, size_t line, size_t col) 
        : text_(text), linePos_(line), colPos_(col), useSpecifiedPosition_(true) {}
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    std::string text_;
    size_t cursorLine_;
    size_t cursorCol_;
    size_t linePos_; // Line position for specified insertion
    size_t colPos_;  // Column position for specified insertion
    bool useSpecifiedPosition_; // Whether to use specified position or cursor position
};

// NewLineCommand - Handles line splitting (Enter key)
// This is the preferred command for adding new lines and splitting lines.
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
// @deprecated - Use NewLineCommand for splitting lines. This class is being phased out.
class AddLineCommand : public Command {
public:
    // Default constructor - Split line at cursor
    AddLineCommand() : text_(""), originalBufferLineCount_(0), splitLine_(true), originalCursorLine_(0), originalCursorCol_(0) {} 
    // Add new line with text
    AddLineCommand(const std::string& text) : text_(text), originalBufferLineCount_(0), splitLine_(false), originalCursorLine_(0), originalCursorCol_(0) {} 
    // New constructor that accepts a text buffer directly
    AddLineCommand(std::shared_ptr<ITextBuffer> textBuffer, const std::string& text) 
        : textBuffer_(textBuffer), text_(text), originalBufferLineCount_(0), splitLine_(false), originalCursorLine_(0), originalCursorCol_(0) {} 
    
    void execute(Editor& editor) override;
    
    void undo(Editor& editor) override;
    
    std::string getDescription() const override;
    
private:
    std::shared_ptr<ITextBuffer> textBuffer_;
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
    // Old constructor for backward compatibility
    DeleteLineCommand(size_t lineIndex) 
        : lineIndex_(lineIndex), wasDeleted_(false) {}
    
    // New constructor that accepts a text buffer directly
    DeleteLineCommand(std::shared_ptr<ITextBuffer> textBuffer, size_t lineIndex) 
        : textBuffer_(textBuffer), lineIndex_(lineIndex), wasDeleted_(false) {}
    
    void execute(Editor& editor) override;
    void execute() {
        if (textBuffer_) {
            // Store the line before deleting
            deletedLine_ = textBuffer_->getLine(lineIndex_);
            
            // Delete the line
            textBuffer_->deleteLine(lineIndex_);
            wasDeleted_ = true;
        }
    }
    
    void undo(Editor& editor) override;
    void undo() {
        if (textBuffer_ && wasDeleted_) {
            // Re-insert the deleted line
            textBuffer_->insertLine(lineIndex_, deletedLine_);
            wasDeleted_ = false;
        }
    }
    
    std::string getDescription() const override {
        return "Delete line " + std::to_string(lineIndex_);
    }
    
private:
    std::shared_ptr<ITextBuffer> textBuffer_;
    size_t lineIndex_;
    std::string deletedLine_;
    bool wasDeleted_ = false;
};

// ReplaceLineCommand - Handles replacing a line with new text
class ReplaceLineCommand : public Command {
public:
    // Old constructor for backward compatibility
    ReplaceLineCommand(size_t lineIndex, const std::string& newText) 
        : lineIndex_(lineIndex), newText_(newText), wasExecuted_(false) {}
    
    // New constructor that accepts a text buffer directly
    ReplaceLineCommand(std::shared_ptr<ITextBuffer> textBuffer, size_t lineIndex, const std::string& newText) 
        : textBuffer_(textBuffer), lineIndex_(lineIndex), newText_(newText), wasExecuted_(false) {}
    
    void execute(Editor& editor) override;
    void execute() {
        if (textBuffer_) {
            // Store the original text before replacing
            originalText_ = textBuffer_->getLine(lineIndex_);
            
            // Replace the line
            textBuffer_->replaceLine(lineIndex_, newText_);
            wasExecuted_ = true;
        }
    }
    
    void undo(Editor& editor) override;
    void undo() {
        if (textBuffer_ && wasExecuted_) {
            // Restore the original text
            textBuffer_->replaceLine(lineIndex_, originalText_);
            wasExecuted_ = false;
        }
    }
    
    std::string getDescription() const override {
        return "Replace line " + std::to_string(lineIndex_);
    }
    
private:
    std::shared_ptr<ITextBuffer> textBuffer_;
    size_t lineIndex_;
    std::string newText_;
    std::string originalText_;
    bool wasExecuted_;
};

// InsertLineCommand - Handles inserting a line at a specific index
class InsertLineCommand : public Command {
public:
    // Old constructor for backward compatibility
    InsertLineCommand(size_t lineIndex, const std::string& text) 
        : lineIndex_(lineIndex), text_(text), wasExecuted_(false) {}
    
    // New constructor that accepts a text buffer directly
    InsertLineCommand(std::shared_ptr<ITextBuffer> textBuffer, size_t lineIndex, const std::string& text) 
        : textBuffer_(textBuffer), lineIndex_(lineIndex), text_(text), wasExecuted_(false) {}
    
    void execute(Editor& editor) override;
    void execute() {
        if (textBuffer_) {
            // Insert the line
            textBuffer_->insertLine(lineIndex_, text_);
            wasExecuted_ = true;
        }
    }
    
    void undo(Editor& editor) override;
    void undo() {
        if (textBuffer_ && wasExecuted_) {
            // Delete the inserted line
            textBuffer_->deleteLine(lineIndex_);
            wasExecuted_ = false;
        }
    }
    
    std::string getDescription() const override {
        return "Insert line at " + std::to_string(lineIndex_);
    }
    
private:
    std::shared_ptr<ITextBuffer> textBuffer_;
    size_t lineIndex_;
    std::string text_;
    bool wasExecuted_;
};

// ReplaceSelectionCommand - Handles replacing the current selection with new text
class ReplaceSelectionCommand : public Command {
public:
    ReplaceSelectionCommand(const std::string& newText)
        : newText_(newText), originalSelectedText_(""), selStartLine_(0), selStartCol_(0), 
          selEndLine_(0), selEndCol_(0), cursorAfterDeleteLine_(0), cursorAfterDeleteCol_(0), executed_(false) {}

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
          originalSelectionStartLine_(0), originalSelectionStartCol_(0), originalSelectionEndLine_(0), 
          originalSelectionEndCol_(0), lastMatchEndLine_(0), lastMatchEndCol_(0) {}
    
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
    
    // Position of match end (for next search)
    size_t lastMatchEndLine_;
    size_t lastMatchEndCol_;
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
    bool findAndStageNextReplacement(Editor& editor);

    std::string searchTerm_;
    std::string replacementText_;
    bool caseSensitive_;
    bool replaceSuccessful_;
    std::string replacementCount_; // String representation of replacement count
    
    // For undo
    size_t originalCursorLine_;
    size_t originalCursorCol_;
    size_t newCursorLine_;
    size_t newCursorCol_;
    std::vector<std::string> originalLines_; // Original buffer content for undo
    
    // For staged replacements
    struct StagedMatch {
        std::string originalText;
        size_t startLine;
        size_t startCol;
        size_t endLine;
        size_t endCol;
    };
    StagedMatch stagedMatch_;
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

// DeleteCharCommand - Deletes a single character in either backspace or forward delete mode
// This command handles both backspace (isBackspace=true) and forward delete (isBackspace=false) operations
// when no selection is active. The command properly handles:
// - Character deletion within a line
// - Line joining when deleting at line boundaries
// - Edge cases (buffer start/end)
// 
// NOTE: This command does not handle text selections. When a selection is active, the editor uses
// ReplaceSelectionCommand("") through the Editor::deleteSelection() method instead.
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
                   lastNewlinePos_(std::string::npos), wasSelectionActive_(false), selStartLine_(0), selStartCol_(0) {}

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
    bool wasSelectionActive_;
    size_t selStartLine_;
    size_t selStartCol_;
};

// CutCommand - Cuts selected text to clipboard
class CutCommand : public Command {
public:
    CutCommand() : 
                 originalClipboard_(""),
                 cutText_(""),
                 originalStartLine_(0), 
                 originalStartCol_(0), 
                 originalEndLine_(0), 
                 originalEndCol_(0),
                 executedSuccessfully_(false),
                 textToCut_(""),
                 wasSelection_(false) {}

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
    std::string textToCut_;
    bool wasSelection_;
};

// IncreaseIndentCommand Implementation
class IncreaseIndentCommand : public Command {
public:
    IncreaseIndentCommand(size_t firstLine, size_t lastLine, const std::vector<std::string>& lines, 
                         size_t tabWidth, bool isSelectionActive, 
                         const Position& selectionStartPos, const Position& cursorPos);
    
    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;
    
private:
    size_t mFirstLineIndex;
    size_t mLastLineIndex;
    std::vector<std::string> mOldLines;
    std::vector<std::string> mNewLines;
    size_t mTabWidth;
    bool mWasSelectionActive;
    Position mOldSelectionStartPos;
    Position mOldCursorPos;
    Position mNewSelectionStartPos;
    Position mNewCursorPos;
    bool executed_ = false;
    std::vector<std::string> originalLines_;
};

// DecreaseIndentCommand Implementation
class DecreaseIndentCommand : public Command {
public:
    DecreaseIndentCommand(size_t firstLine, size_t lastLine, const std::vector<std::string>& lines, 
                         size_t tabWidth, bool isSelectionActive, 
                         const Position& selectionStartPos, const Position& cursorPos);
    
    void execute(Editor& editor) override;
    void undo(Editor& editor) override;
    std::string getDescription() const override;
    
private:
    size_t mFirstLineIndex;
    size_t mLastLineIndex;
    std::vector<std::string> mOldLines;
    std::vector<std::string> mNewLines;
    size_t mTabWidth;
    bool mWasSelectionActive;
    Position mOldSelectionStartPos;
    Position mOldCursorPos;
    Position mNewSelectionStartPos;
    Position mNewCursorPos;
    bool executed_ = false;
};

// LoadFileCommand - Handles loading a file into the text buffer
class LoadFileCommand : public Command {
public:
    LoadFileCommand(std::shared_ptr<ITextBuffer> textBuffer, const std::string& filePath)
        : textBuffer_(textBuffer), filePath_(filePath), wasExecuted_(false) {}
    
    void execute(Editor& editor) override;
    void execute() {
        if (textBuffer_) {
            // Store the original buffer state
            originalBufferContent_ = saveBufferState();
            
            // Load the file
            bool success = loadFile();
            wasExecuted_ = success;
        }
    }
    
    void undo(Editor& editor) override;
    void undo() {
        if (textBuffer_ && wasExecuted_) {
            // Restore the original buffer state
            restoreBufferState(originalBufferContent_);
            wasExecuted_ = false;
        }
    }
    
    std::string getDescription() const override {
        return "Load file " + filePath_;
    }
    
private:
    bool loadFile() {
        try {
            std::ifstream file(filePath_);
            if (!file.is_open()) {
                return false;
            }
            
            // Clear existing buffer
            while (textBuffer_->lineCount() > 0) {
                textBuffer_->deleteLine(0);
            }
            
            // Read file line by line
            std::string line;
            while (std::getline(file, line)) {
                // Handle different line endings
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                textBuffer_->addLine(line);
            }
            
            // Ensure buffer has at least one line
            if (textBuffer_->lineCount() == 0) {
                textBuffer_->addLine("");
            }
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    std::vector<std::string> saveBufferState() {
        std::vector<std::string> bufferContent;
        for (size_t i = 0; i < textBuffer_->lineCount(); ++i) {
            bufferContent.push_back(textBuffer_->getLine(i));
        }
        return bufferContent;
    }
    
    void restoreBufferState(const std::vector<std::string>& bufferContent) {
        // Clear existing buffer
        while (textBuffer_->lineCount() > 0) {
            textBuffer_->deleteLine(0);
        }
        
        // Restore saved content
        for (const auto& line : bufferContent) {
            textBuffer_->addLine(line);
        }
    }
    
    std::shared_ptr<ITextBuffer> textBuffer_;
    std::string filePath_;
    std::vector<std::string> originalBufferContent_;
    bool wasExecuted_;
};

// SaveFileCommand - Handles saving the text buffer to a file
class SaveFileCommand : public Command {
public:
    SaveFileCommand(std::shared_ptr<ITextBuffer> textBuffer, const std::string& filePath)
        : textBuffer_(textBuffer), filePath_(filePath), wasExecuted_(false) {}
    
    void execute(Editor& editor) override;
    void execute() {
        if (textBuffer_) {
            // Save the file
            bool success = saveFile();
            wasExecuted_ = success;
        }
    }
    
    void undo(Editor& editor) override;
    void undo() {
        // Saving a file doesn't change the buffer state, so undo is a no-op
    }
    
    std::string getDescription() const override {
        return "Save file " + filePath_;
    }
    
private:
    bool saveFile() {
        try {
            std::ofstream file(filePath_);
            if (!file.is_open()) {
                return false;
            }
            
            // Write buffer content to file
            for (size_t i = 0; i < textBuffer_->lineCount(); ++i) {
                file << textBuffer_->getLine(i);
                // Add newline after each line except the last
                if (i < textBuffer_->lineCount() - 1) {
                    file << "\n";
                }
            }
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    std::shared_ptr<ITextBuffer> textBuffer_;
    std::string filePath_;
    bool wasExecuted_;
};

// BatchCommand - Handles executing multiple commands as a single operation
class BatchCommand : public Command {
public:
    BatchCommand() : wasExecuted_(false) {}
    
    void addCommand(std::shared_ptr<Command> command) {
        commands_.push_back(command);
    }
    
    void execute(Editor& editor) override {
        // Execute each command in sequence
        for (auto& command : commands_) {
            command->execute(editor);
        }
        wasExecuted_ = true;
    }
    
    void undo(Editor& editor) override {
        if (wasExecuted_) {
            // Undo commands in reverse order
            for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
                (*it)->undo(editor);
            }
            wasExecuted_ = false;
        }
    }
    
    std::string getDescription() const override {
        return "Batch command with " + std::to_string(commands_.size()) + " operations";
    }
    
private:
    std::vector<std::shared_ptr<Command>> commands_;
    bool wasExecuted_;
};

// CompoundCommand class

#endif // EDITOR_COMMANDS_H 