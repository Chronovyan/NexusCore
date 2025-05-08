#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H

#include "Command.h"
#include "Editor.h"
#include <string>
#include <utility>

// InsertTextCommand - Handles insertion of text at current cursor position
class InsertTextCommand : public Command {
public:
    InsertTextCommand(const std::string& text) : text_(text) {}
    
    void execute(Editor& editor) override {
        // Store cursor position for undo
        cursorLine_ = editor.getCursorLine();
        cursorCol_ = editor.getCursorCol();
        
        // Get direct access to the buffer and insert text
        TextBuffer& buffer = editor.getBuffer();
        buffer.insertString(cursorLine_, cursorCol_, text_);
        
        // Update cursor position after insertion
        editor.setCursor(cursorLine_, cursorCol_ + text_.length());
    }
    
    void undo(Editor& editor) override {
        // Restore cursor to original position
        editor.setCursor(cursorLine_, cursorCol_);
        
        // Delete the inserted text
        for (size_t i = 0; i < text_.length(); ++i) {
            editor.getBuffer().deleteCharForward(cursorLine_, cursorCol_);
        }
        
        // Ensure cursor is at the original position
        editor.setCursor(cursorLine_, cursorCol_);
    }
    
    std::string getDescription() const override {
        return "Insert text: " + text_;
    }
    
private:
    std::string text_;
    size_t cursorLine_;
    size_t cursorCol_;
};

// DeleteTextCommand - Handles deletion of text (backspace)
class DeleteTextCommand : public Command {
public:
    DeleteTextCommand() = default;
    
    void execute(Editor& editor) override {
        // Store cursor position for undo
        cursorLine_ = editor.getCursorLine();
        cursorCol_ = editor.getCursorCol();
        
        // If cursor is at start of buffer, nothing to delete
        if (cursorLine_ == 0 && cursorCol_ == 0) {
            deletedText_ = "";
            return;
        }
        
        TextBuffer& buffer = editor.getBuffer();
        
        // If cursor is at start of line and not first line, store the newline character
        if (cursorCol_ == 0 && cursorLine_ > 0) {
            deletedText_ = "\n";
            
            // Need to remember the previous line's length for cursor positioning
            size_t prevLineLength = buffer.lineLength(cursorLine_ - 1);
            
            // Join with previous line
            buffer.deleteChar(cursorLine_, cursorCol_);
            
            // Update cursor position
            editor.setCursor(cursorLine_ - 1, prevLineLength);
            return;
        }
        
        // For regular backspace, store the character that will be deleted
        size_t lineIdx = cursorLine_;
        size_t colIdx = cursorCol_ - 1; // Character to be deleted is before cursor
        const std::string& line = buffer.getLine(lineIdx);
        deletedText_ = std::string(1, line[colIdx]);
        
        // Execute the deletion
        buffer.deleteChar(cursorLine_, cursorCol_);
        
        // Update cursor position
        editor.setCursor(cursorLine_, cursorCol_ - 1);
    }
    
    void undo(Editor& editor) override {
        // Restore cursor to original position
        editor.setCursor(cursorLine_, cursorCol_);
        
        // If deletedText is empty, nothing was deleted
        if (deletedText_.empty()) {
            return;
        }
        
        TextBuffer& buffer = editor.getBuffer();
        
        // If deletedText is a newline, we need to split the line
        if (deletedText_ == "\n") {
            buffer.splitLine(cursorLine_, cursorCol_);
            editor.setCursor(cursorLine_ + 1, 0);
            return;
        }
        
        // Otherwise, insert the deleted character
        buffer.insertString(cursorLine_, cursorCol_ - 1, deletedText_);
        
        // Keep cursor at the original position
        editor.setCursor(cursorLine_, cursorCol_);
    }
    
    std::string getDescription() const override {
        if (deletedText_ == "\n") {
            return "Delete newline";
        }
        return "Delete character: " + deletedText_;
    }
    
private:
    std::string deletedText_;
    size_t cursorLine_;
    size_t cursorCol_;
};

// DeleteForwardCommand - Handles forward deletion (delete key)
class DeleteForwardCommand : public Command {
public:
    DeleteForwardCommand() = default;
    
    void execute(Editor& editor) override {
        // Store cursor position for undo
        cursorLine_ = editor.getCursorLine();
        cursorCol_ = editor.getCursorCol();
        
        TextBuffer& buffer = editor.getBuffer();
        
        // Check if we're at the end of the buffer
        if (cursorLine_ >= buffer.lineCount() - 1 && 
            cursorCol_ >= buffer.lineLength(cursorLine_)) {
            deletedText_ = "";
            return;
        }
        
        // If at end of line but not last line, we'll delete a newline
        if (cursorCol_ >= buffer.lineLength(cursorLine_) && 
            cursorLine_ < buffer.lineCount() - 1) {
            deletedText_ = "\n";
            buffer.deleteCharForward(cursorLine_, cursorCol_); // This will join with next line
            return;
        }
        
        // For regular delete, store the character that will be deleted
        const std::string& line = buffer.getLine(cursorLine_);
        deletedText_ = std::string(1, line[cursorCol_]);
        
        // Execute the deletion
        buffer.deleteCharForward(cursorLine_, cursorCol_);
    }
    
    void undo(Editor& editor) override {
        // Restore cursor to original position
        editor.setCursor(cursorLine_, cursorCol_);
        
        // If deletedText is empty, nothing was deleted
        if (deletedText_.empty()) {
            return;
        }
        
        TextBuffer& buffer = editor.getBuffer();
        
        // If deletedText is a newline, we need to split the line
        if (deletedText_ == "\n") {
            buffer.splitLine(cursorLine_, cursorCol_);
            return;
        }
        
        // Otherwise, insert the deleted character
        buffer.insertString(cursorLine_, cursorCol_, deletedText_);
    }
    
    std::string getDescription() const override {
        if (deletedText_ == "\n") {
            return "Delete forward newline";
        }
        return "Delete forward character: " + deletedText_;
    }
    
private:
    std::string deletedText_;
    size_t cursorLine_;
    size_t cursorCol_;
};

// NewLineCommand - Handles line splitting (Enter key)
class NewLineCommand : public Command {
public:
    NewLineCommand() = default;
    
    void execute(Editor& editor) override {
        // Store cursor position for undo
        cursorLine_ = editor.getCursorLine();
        cursorCol_ = editor.getCursorCol();
        
        TextBuffer& buffer = editor.getBuffer();
        
        // Check if buffer is empty
        if (buffer.isEmpty()) {
            buffer.addLine("");
            buffer.addLine("");
            editor.setCursor(1, 0);
        } else {
            // Split the line at the cursor position
            buffer.splitLine(cursorLine_, cursorCol_);
            editor.setCursor(cursorLine_ + 1, 0);
        }
    }
    
    void undo(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Join the lines back together
        buffer.joinLines(cursorLine_);
        
        // Restore original cursor position
        editor.setCursor(cursorLine_, cursorCol_);
    }
    
    std::string getDescription() const override {
        return "New line";
    }
    
private:
    size_t cursorLine_;
    size_t cursorCol_;
};

// AddLineCommand - Handles adding a new line at the end of the buffer
class AddLineCommand : public Command {
public:
    AddLineCommand(const std::string& text) : text_(text) {}
    
    void execute(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Store line count for undo
        lineCount_ = buffer.lineCount();
        
        // Add the line
        buffer.addLine(text_);
        
        // Move cursor to the new line
        editor.setCursor(buffer.lineCount() - 1, 0);
    }
    
    void undo(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Delete the line if it still exists
        if (buffer.lineCount() > lineCount_) {
            buffer.deleteLine(buffer.lineCount() - 1);
        }
        
        // Put cursor at a reasonable position
        if (lineCount_ > 0) {
            editor.setCursor(lineCount_ - 1, 0);
        } else {
            editor.setCursor(0, 0);
        }
    }
    
    std::string getDescription() const override {
        return "Add line: " + text_;
    }
    
private:
    std::string text_;
    size_t lineCount_;
};

// DeleteLineCommand - Handles deletion of a line
class DeleteLineCommand : public Command {
public:
    DeleteLineCommand(size_t lineIndex) : lineIndex_(lineIndex) {}
    
    void execute(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Store the line content for undo
        if (lineIndex_ < buffer.lineCount()) {
            deletedLine_ = buffer.getLine(lineIndex_);
            
            // Delete the line
            buffer.deleteLine(lineIndex_);
            
            // Adjust cursor position
            if (buffer.isEmpty()) {
                buffer.addLine("");
                editor.setCursor(0, 0);
            } else if (lineIndex_ >= buffer.lineCount()) {
                editor.setCursor(buffer.lineCount() - 1, 0);
            } else {
                editor.setCursor(lineIndex_, 0);
            }
        }
    }
    
    void undo(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Restore the deleted line
        if (lineIndex_ <= buffer.lineCount()) {
            buffer.insertLine(lineIndex_, deletedLine_);
            editor.setCursor(lineIndex_, 0);
        }
    }
    
    std::string getDescription() const override {
        return "Delete line at index " + std::to_string(lineIndex_);
    }
    
private:
    size_t lineIndex_;
    std::string deletedLine_;
};

// ReplaceLineCommand - Handles replacing a line with new text
class ReplaceLineCommand : public Command {
public:
    ReplaceLineCommand(size_t lineIndex, const std::string& newText) 
        : lineIndex_(lineIndex), newText_(newText) {}
    
    void execute(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Store the original line for undo
        if (lineIndex_ < buffer.lineCount()) {
            originalText_ = buffer.getLine(lineIndex_);
            
            // Replace the line
            buffer.replaceLine(lineIndex_, newText_);
            
            // Set cursor to start of replaced line
            editor.setCursor(lineIndex_, 0);
        }
    }
    
    void undo(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Restore the original line
        if (lineIndex_ < buffer.lineCount()) {
            buffer.replaceLine(lineIndex_, originalText_);
            editor.setCursor(lineIndex_, 0);
        }
    }
    
    std::string getDescription() const override {
        return "Replace line at index " + std::to_string(lineIndex_);
    }
    
private:
    size_t lineIndex_;
    std::string newText_;
    std::string originalText_;
};

// InsertLineCommand - Handles inserting a line at a specific index
class InsertLineCommand : public Command {
public:
    InsertLineCommand(size_t lineIndex, const std::string& text) 
        : lineIndex_(lineIndex), text_(text) {}
    
    void execute(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Insert the line
        if (lineIndex_ <= buffer.lineCount()) {
            buffer.insertLine(lineIndex_, text_);
            editor.setCursor(lineIndex_, 0);
        }
    }
    
    void undo(Editor& editor) override {
        TextBuffer& buffer = editor.getBuffer();
        
        // Delete the inserted line
        if (lineIndex_ < buffer.lineCount()) {
            buffer.deleteLine(lineIndex_);
            
            // Adjust cursor position
            if (lineIndex_ > 0) {
                editor.setCursor(lineIndex_ - 1, 0);
            } else if (buffer.lineCount() > 0) {
                editor.setCursor(0, 0);
            }
        }
    }
    
    std::string getDescription() const override {
        return "Insert line at index " + std::to_string(lineIndex_);
    }
    
private:
    size_t lineIndex_;
    std::string text_;
};

// ReplaceSelectionCommand - Handles replacing selected text with new text
class ReplaceSelectionCommand : public Command {
public:
    ReplaceSelectionCommand(const std::string& newText) : newText_(newText) {}
    
    void execute(Editor& editor) override {
        if (!editor.hasSelection()) {
            return;
        }
        
        // Get the cursor and selection information before replacing
        // We will use the TestEditor class in tests which exposes these via accessor methods
        originalCursorLine_ = editor.getCursorLine();
        originalCursorCol_ = editor.getCursorCol();
        
        // Store the original text
        originalText_ = editor.getSelectedText();
        
        // Delete the selected text
        editor.deleteSelectedText();
        
        // Get the new cursor position after deletion
        size_t insertLine = editor.getCursorLine();
        size_t insertCol = editor.getCursorCol();
        
        // Insert the new text at the current cursor position
        TextBuffer& buffer = editor.getBuffer();
        buffer.insertString(insertLine, insertCol, newText_);
        
        // Update cursor position after insertion
        editor.setCursor(insertLine, insertCol + newText_.length());
    }
    
    void undo(Editor& editor) override {
        // Position cursor where the replaced text begins
        editor.setCursor(originalCursorLine_, originalCursorCol_);
        
        // Find the end position based on the length of the replacement text
        size_t endCol = originalCursorCol_ + newText_.length();
        
        // Manually select the text
        editor.setSelectionStart();
        editor.setCursor(originalCursorLine_, endCol);
        editor.setSelectionEnd();
        
        // Delete the replacement text
        editor.deleteSelectedText();
        
        // Insert the original text
        TextBuffer& buffer = editor.getBuffer();
        buffer.insertString(originalCursorLine_, originalCursorCol_, originalText_);
        
        // Restore cursor to original position
        editor.setCursor(originalCursorLine_, originalCursorCol_);
    }
    
    std::string getDescription() const override {
        return "Replace selection with: " + newText_;
    }
    
private:
    std::string newText_;
    std::string originalText_;
    size_t originalCursorLine_;
    size_t originalCursorCol_;
};

#endif // EDITOR_COMMANDS_H 