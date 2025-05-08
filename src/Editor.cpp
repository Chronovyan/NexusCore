#include "Editor.h"
#include <iostream> // For std::cout, std::cerr (used in printView, and potentially by TextBuffer methods if they still print errors)
#include <algorithm> // For std::min, std::max
#include <cctype>    // For isalnum, isspace

Editor::Editor()
    : buffer_(), cursorLine_(0), cursorCol_(0), 
      hasSelection_(false), selectionStartLine_(0), selectionStartCol_(0),
      selectionEndLine_(0), selectionEndCol_(0), clipboard_("") {
    // Ensure buffer starts non-empty for initial cursor validation, or handle empty buffer case.
    if (buffer_.isEmpty()) {
        buffer_.addLine(""); // Start with one empty line so cursor at (0,0) is valid.
    }
    validateAndClampCursor(); 
}

void Editor::setCursor(size_t line, size_t col) {
    cursorLine_ = line;
    cursorCol_ = col;
    validateAndClampCursor();
}

size_t Editor::getCursorLine() const {
    return cursorLine_;
}

size_t Editor::getCursorCol() const {
    return cursorCol_;
}

void Editor::printView(std::ostream& os) const {
    if (buffer_.isEmpty()) {
        os << "(Buffer is empty)" << '\n';
        os << "Cursor at: [0, 0] (conceptual on empty buffer)" << '\n';
        return;
    }

    for (size_t i = 0; i < buffer_.lineCount(); ++i) {
        if (i == cursorLine_) {
            const std::string& currentLine = buffer_.getLine(i);
            // Simple conceptual cursor: print part before, cursor marker, part after
            // This is very basic and doesn't handle tabs or wide characters well.
            if (cursorCol_ < currentLine.length()) {
                os << currentLine.substr(0, cursorCol_);
                os << "|"; // Cursor marker
                os << currentLine.substr(cursorCol_);
            } else {
                os << currentLine;
                os << "|"; // Cursor at end of line
            }
            os << "  <-- Cursor Line (" << cursorLine_ << ", " << cursorCol_ << ")";
        } else {
            os << buffer_.getLine(i);
        }
        os << '\n';
    }
    // If cursor is beyond last line (e.g. after adding a line), print its position.
    if (cursorLine_ >= buffer_.lineCount() && buffer_.lineCount() > 0 && cursorLine_ != (buffer_.lineCount() -1) ) { 
        // This condition is a bit tricky, it tries to only show if cursor is truly out of intended bounds
        // and not just at the last valid line that was printed above.
        // Let's refine this to be clearer or remove if redundant with clamping display.
        // For now, let's ensure it prints if cursorLine_ suggests it's on a "new" line that doesn't exist yet.
        // This message might be less relevant now that validateAndClampCursor keeps it in bounds.
        // os << "Debug: Cursor after loop at: [" << cursorLine_ << ", " << cursorCol_ << "] (Clamped to end of buffer)" << '\n';
    }
    // An alternative or additional display for explicit cursor position:
    // os << "----" << '\n';
    // os << "Cursor at: [" << cursorLine_ << ", " << cursorCol_ << "]" << '\n';
}

TextBuffer& Editor::getBuffer() {
    return buffer_;
}

const TextBuffer& Editor::getBuffer() const {
    return buffer_;
}

// --- Editor-level operations (pass-through for now, but can add cursor logic) ---
void Editor::addLine(const std::string& text) {
    buffer_.addLine(text);
    // Potentially move cursor to the start of the new line or end of buffer
    setCursor(buffer_.lineCount() > 0 ? buffer_.lineCount() - 1 : 0, 0);
}

void Editor::insertLine(size_t lineIndex, const std::string& text) {
    buffer_.insertLine(lineIndex, text);
    // Adjust cursor if it was on or after the inserted line
    if (cursorLine_ >= lineIndex) {
        // setCursor(cursorLine_ + 1, cursorCol_); // or move to start of inserted line
        setCursor(lineIndex, 0); // Move to start of newly inserted line
    }
    validateAndClampCursor();
}

void Editor::deleteLine(size_t lineIndex) {
    buffer_.deleteLine(lineIndex);
    // Adjust cursor if it was on or after the deleted line
    // or if it was on the line that got deleted.
    if (cursorLine_ >= lineIndex) {
        if (buffer_.isEmpty()) { // If buffer becomes empty
             buffer_.addLine(""); // Add an empty line to keep a valid cursor pos
             setCursor(0,0);
        } else if (cursorLine_ > 0 && cursorLine_ >= buffer_.lineCount()) {
             setCursor(buffer_.lineCount() -1 , 0); // Move to start of previous line if last was deleted
        } else {
             setCursor(cursorLine_ > lineIndex ? cursorLine_ -1 : lineIndex, 0); // Try to stay or move to start of current/next line
        }
    }
    validateAndClampCursor();
}

void Editor::replaceLine(size_t lineIndex, const std::string& text) {
    buffer_.replaceLine(lineIndex, text);
    // Cursor line doesn't change, but column might become invalid
    setCursor(lineIndex, 0); // Move to start of replaced line for simplicity
    validateAndClampCursor(); 
}

void Editor::typeText(const std::string& textToInsert) {
    if (buffer_.isEmpty()) { // Should not happen if constructor ensures a line
        validateAndClampCursor(); // Will add an empty line if needed.
    }
    for (char ch : textToInsert) {
        // For simplicity, newline characters in typed text are not specially handled here.
        // A more advanced editor might split the line or insert multiple lines.
        // For now, they will be inserted as regular characters if present.
        if (ch == '\n') { 
            // Option 1: Insert newline as a character (might look weird)
            // buffer_.insertChar(cursorLine_, cursorCol_, ch);
            // cursorCol_++;

            // Option 2: Split line (more complex, involves new TextBuffer methods)
            // std::string restOfLine = buffer_.getLine(cursorLine_).substr(cursorCol_);
            // buffer_.replaceLine(cursorLine_, buffer_.getLine(cursorLine_).substr(0, cursorCol_));
            // insertLine(cursorLine_ + 1, restOfLine); // insertLine already handles cursor
            // setCursor(cursorLine_ + 1, 0);

            // Option 3: For now, just treat as a normal character or skip. Let's skip for simplicity now.
            // Or, for a basic interpretation, let's just add a new line and move cursor
            std::string currentLineText = buffer_.getLine(cursorLine_);
            std::string textAfterCursor = "";
            if(cursorCol_ < currentLineText.length()){
                textAfterCursor = currentLineText.substr(cursorCol_);
            }
            buffer_.replaceLine(cursorLine_, currentLineText.substr(0, cursorCol_));
            insertLine(cursorLine_ + 1, textAfterCursor); // insertLine moves cursor to start of new line
                                                      // which is cursorLine_+1, col 0
        } else {
            buffer_.insertChar(cursorLine_, cursorCol_, ch);
            cursorCol_++;
        }
    }
    validateAndClampCursor();
}

// --- Cursor Movement ---
void Editor::moveCursorUp() {
    if (cursorLine_ > 0) {
        cursorLine_--;
    }
    // Column position is maintained, validateAndClampCursor will adjust if new line is shorter.
    validateAndClampCursor();
}

void Editor::moveCursorDown() {
    if (!buffer_.isEmpty() && cursorLine_ < buffer_.lineCount() - 1) {
        cursorLine_++;
    }
    // Column position is maintained, validateAndClampCursor will adjust if new line is shorter.
    validateAndClampCursor();
}

void Editor::moveCursorLeft() {
    if (cursorCol_ > 0) {
        cursorCol_--;
    } else if (cursorLine_ > 0) {
        // Move to end of previous line
        cursorLine_--;
        cursorCol_ = buffer_.getLine(cursorLine_).length(); 
    }
    validateAndClampCursor(); // Ensure final position is valid
}

void Editor::moveCursorRight() {
    if (buffer_.isEmpty()) {
        validateAndClampCursor(); // Should already be 0,0 on an empty (but 1 line) buffer
        return;
    }
    const std::string& currentLineContent = buffer_.getLine(cursorLine_);
    if (cursorCol_ < currentLineContent.length()) {
        cursorCol_++;
    } else if (cursorLine_ < buffer_.lineCount() - 1) {
        // Move to start of next line
        cursorLine_++;
        cursorCol_ = 0;
    }
    validateAndClampCursor(); // Ensure final position is valid
}

void Editor::moveCursorToLineStart() {
    cursorCol_ = 0;
    validateAndClampCursor();
}

void Editor::moveCursorToLineEnd() {
    if (!buffer_.isEmpty()) {
        cursorCol_ = buffer_.getLine(cursorLine_).length();
    }
    validateAndClampCursor();
}

void Editor::moveCursorToBufferStart() {
    cursorLine_ = 0;
    cursorCol_ = 0;
    validateAndClampCursor();
}

void Editor::moveCursorToBufferEnd() {
    if (!buffer_.isEmpty()) {
        cursorLine_ = buffer_.lineCount() - 1;
        cursorCol_ = buffer_.getLine(cursorLine_).length();
    }
    validateAndClampCursor();
}

void Editor::moveCursorToNextWord() {
    if (buffer_.isEmpty()) return;
    
    const std::string& line = buffer_.getLine(cursorLine_);
    
    // Start from current position
    size_t pos = cursorCol_;
    
    // If we're at the end of the line, move to next line
    if (pos >= line.length() && cursorLine_ < buffer_.lineCount() - 1) {
        cursorLine_++;
        cursorCol_ = 0;
        validateAndClampCursor();
        return;
    }
    
    // Skip current word if we're in one
    while (pos < line.length() && isWordChar(line[pos])) {
        pos++;
    }
    
    // Skip spaces after word
    while (pos < line.length() && !isWordChar(line[pos])) {
        pos++;
    }
    
    // Update cursor position
    cursorCol_ = pos;
    validateAndClampCursor();
}

void Editor::moveCursorToPrevWord() {
    if (buffer_.isEmpty()) return;
    
    // If at beginning of line, move to previous line end
    if (cursorCol_ == 0) {
        if (cursorLine_ > 0) {
            cursorLine_--;
            cursorCol_ = buffer_.getLine(cursorLine_).length();
        }
        validateAndClampCursor();
        return;
    }
    
    const std::string& line = buffer_.getLine(cursorLine_);
    
    // Start from position before current
    size_t pos = cursorCol_ > 0 ? cursorCol_ - 1 : 0;
    
    // Skip spaces before word
    while (pos > 0 && !isWordChar(line[pos])) {
        pos--;
    }
    
    // Find beginning of word
    while (pos > 0 && isWordChar(line[pos-1])) {
        pos--;
    }
    
    // Update cursor position
    cursorCol_ = pos;
    validateAndClampCursor();
}

// Text editing operations
void Editor::typeChar(char ch) {
    if (hasSelection_) {
        deleteSelectedText();
    }
    
    if (buffer_.isEmpty()) {
        buffer_.addLine("");
    }
    
    buffer_.insertChar(cursorLine_, cursorCol_, ch);
    cursorCol_++;
    validateAndClampCursor();
}

void Editor::backspace() {
    if (hasSelection_) {
        deleteSelectedText();
        return;
    }
    
    if (buffer_.isEmpty()) return;
    
    // If at beginning of line, join with previous line
    if (cursorCol_ == 0) {
        if (cursorLine_ > 0) {
            size_t prevLineLength = buffer_.getLine(cursorLine_ - 1).length();
            buffer_.deleteChar(cursorLine_, cursorCol_); // This joins with previous line
            cursorLine_--;
            cursorCol_ = prevLineLength;
        }
    } else {
        buffer_.deleteChar(cursorLine_, cursorCol_);
        cursorCol_--;
    }
    
    validateAndClampCursor();
}

void Editor::deleteForward() {
    if (hasSelection_) {
        deleteSelectedText();
        return;
    }
    
    if (buffer_.isEmpty()) return;
    
    const std::string& line = buffer_.getLine(cursorLine_);
    
    // If at end of line, join with next line
    if (cursorCol_ >= line.length()) {
        if (cursorLine_ < buffer_.lineCount() - 1) {
            buffer_.deleteCharForward(cursorLine_, cursorCol_); // This joins with next line
        }
    } else {
        buffer_.deleteCharForward(cursorLine_, cursorCol_);
    }
    
    validateAndClampCursor();
}

void Editor::newLine() {
    if (hasSelection_) {
        deleteSelectedText();
    }
    
    if (buffer_.isEmpty()) {
        buffer_.addLine("");
        buffer_.addLine("");
        cursorLine_ = 1;
        cursorCol_ = 0;
    } else {
        buffer_.splitLine(cursorLine_, cursorCol_);
        cursorLine_++;
        cursorCol_ = 0;
    }
    
    validateAndClampCursor();
}

void Editor::joinWithNextLine() {
    if (cursorLine_ >= buffer_.lineCount() - 1) return; // Can't join if last line
    
    buffer_.joinLines(cursorLine_);
    validateAndClampCursor();
}

// Selection operations
void Editor::setSelectionStart() {
    selectionStartLine_ = cursorLine_;
    selectionStartCol_ = cursorCol_;
    hasSelection_ = true;
    // At this point selectionEnd = selectionStart (no visible selection yet)
    selectionEndLine_ = selectionStartLine_;
    selectionEndCol_ = selectionStartCol_;
}

void Editor::setSelectionEnd() {
    selectionEndLine_ = cursorLine_;
    selectionEndCol_ = cursorCol_;
    
    // Ensure we have valid selection (start should be before end)
    if (selectionStartLine_ > selectionEndLine_ || 
        (selectionStartLine_ == selectionEndLine_ && selectionStartCol_ > selectionEndCol_)) {
        // Swap start and end if needed
        std::swap(selectionStartLine_, selectionEndLine_);
        std::swap(selectionStartCol_, selectionEndCol_);
    }
}

bool Editor::hasSelection() const {
    return hasSelection_ && 
           (selectionStartLine_ != selectionEndLine_ || 
            selectionStartCol_ != selectionEndCol_);
}

void Editor::clearSelection() {
    hasSelection_ = false;
}

std::string Editor::getSelectedText() const {
    if (!hasSelection()) return "";
    
    std::string result;
    
    if (selectionStartLine_ == selectionEndLine_) {
        // Selection on a single line
        result = buffer_.getLineSegment(selectionStartLine_, 
                                        selectionStartCol_, 
                                        selectionEndCol_);
    } else {
        // Multi-line selection
        // First line (from start to end of line)
        result = buffer_.getLineSegment(selectionStartLine_, 
                                        selectionStartCol_, 
                                        buffer_.lineLength(selectionStartLine_));
        result += '\n';
        
        // Middle lines (full lines)
        for (size_t i = selectionStartLine_ + 1; i < selectionEndLine_; i++) {
            result += buffer_.getLine(i) + '\n';
        }
        
        // Last line (from start to selection end)
        result += buffer_.getLineSegment(selectionEndLine_, 0, selectionEndCol_);
    }
    
    return result;
}

void Editor::deleteSelectedText() {
    if (!hasSelection()) return;
    
    // Store cursor at selection start for repositioning after delete
    cursorLine_ = selectionStartLine_;
    cursorCol_ = selectionStartCol_;
    
    if (selectionStartLine_ == selectionEndLine_) {
        // Single line delete
        std::string line = buffer_.getLine(selectionStartLine_);
        std::string newLine = line.substr(0, selectionStartCol_) + 
                              line.substr(selectionEndCol_);
        buffer_.replaceLine(selectionStartLine_, newLine);
    } else {
        // Multi-line delete
        // Get partial first and last lines
        std::string firstLine = buffer_.getLineSegment(selectionStartLine_, 0, selectionStartCol_);
        std::string lastLine = buffer_.getLineSegment(selectionEndLine_, selectionEndCol_,
                                                     buffer_.lineLength(selectionEndLine_));
        
        // Replace first line with firstLine + lastLine
        buffer_.replaceLine(selectionStartLine_, firstLine + lastLine);
        
        // Delete the remaining lines (from end to start to avoid index shifting)
        for (size_t i = selectionEndLine_; i > selectionStartLine_; i--) {
            buffer_.deleteLine(i);
        }
    }
    
    clearSelection();
    validateAndClampCursor();
}

void Editor::copySelectedText() {
    clipboard_ = getSelectedText();
}

void Editor::cutSelectedText() {
    if (!hasSelection()) return;
    
    clipboard_ = getSelectedText();
    deleteSelectedText();
}

void Editor::pasteText() {
    if (clipboard_.empty()) return;
    
    if (hasSelection()) {
        deleteSelectedText();
    }
    
    // Check for newlines in clipboard
    size_t newlinePos = clipboard_.find('\n');
    if (newlinePos == std::string::npos) {
        // No newlines, simple insertion
        buffer_.insertString(cursorLine_, cursorCol_, clipboard_);
        cursorCol_ += clipboard_.length();
    } else {
        // Contains newlines, need to split into lines
        std::string currentLine = buffer_.getLine(cursorLine_);
        std::string beforeCursor = currentLine.substr(0, cursorCol_);
        std::string afterCursor = currentLine.substr(cursorCol_);
        
        // Split clipboard into lines
        std::vector<std::string> clipLines;
        size_t startPos = 0;
        while (newlinePos != std::string::npos) {
            clipLines.push_back(clipboard_.substr(startPos, newlinePos - startPos));
            startPos = newlinePos + 1;
            newlinePos = clipboard_.find('\n', startPos);
        }
        // Add the last part
        clipLines.push_back(clipboard_.substr(startPos));
        
        // Replace current line with first part
        buffer_.replaceLine(cursorLine_, beforeCursor + clipLines[0]);
        
        // Insert middle lines
        for (size_t i = 1; i < clipLines.size() - 1; i++) {
            buffer_.insertLine(cursorLine_ + i, clipLines[i]);
        }
        
        // Insert last line + afterCursor
        buffer_.insertLine(cursorLine_ + clipLines.size() - 1, 
                           clipLines.back() + afterCursor);
        
        // Set cursor to end of inserted text
        cursorLine_ += clipLines.size() - 1;
        cursorCol_ = clipLines.back().length();
    }
    
    validateAndClampCursor();
}

void Editor::deleteWord() {
    if (buffer_.isEmpty()) return;
    
    if (hasSelection()) {
        deleteSelectedText();
        return;
    }
    
    const std::string& line = buffer_.getLine(cursorLine_);
    
    // If at end of line, delete until next word on next line
    if (cursorCol_ >= line.length()) {
        if (cursorLine_ < buffer_.lineCount() - 1) {
            joinWithNextLine();
        }
        return;
    }
    
    // Set selection from current position to next word
    setSelectionStart();
    moveCursorToNextWord();
    setSelectionEnd();
    
    // Delete the selection
    deleteSelectedText();
}

void Editor::selectWord() {
    if (buffer_.isEmpty()) return;
    
    const std::string& line = buffer_.getLine(cursorLine_);
    
    // If at end of line or on whitespace, nothing to select
    if (cursorCol_ >= line.length() || !isWordChar(line[cursorCol_])) {
        return;
    }
    
    // Find word boundaries
    size_t wordStart = cursorCol_;
    while (wordStart > 0 && isWordChar(line[wordStart - 1])) {
        wordStart--;
    }
    
    size_t wordEnd = cursorCol_;
    while (wordEnd < line.length() && isWordChar(line[wordEnd])) {
        wordEnd++;
    }
    
    // Set selection
    cursorCol_ = wordStart;
    setSelectionStart();
    cursorCol_ = wordEnd;
    setSelectionEnd();
}

bool Editor::isWordChar(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// --- Private Helper Methods ---
void Editor::validateAndClampCursor() {
    if (buffer_.isEmpty()) {
        // This case should ideally be handled by ensuring buffer_ is never truly empty 
        // if we want a valid cursor (e.g., always has at least one "" line).
        // For robustness, if it does become empty and then non-empty (e.g. via loadFromFile):
        cursorLine_ = 0;
        cursorCol_ = 0;
        if (buffer_.isEmpty()) buffer_.addLine(""); // Ensure one line exists for valid cursor
        return;
    }

    // Clamp line
    if (cursorLine_ >= buffer_.lineCount()) {
        cursorLine_ = buffer_.lineCount() > 0 ? buffer_.lineCount() - 1 : 0;
    }

    // Clamp column
    const std::string& currentLineContent = buffer_.getLine(cursorLine_);
    if (cursorCol_ > currentLineContent.length()) {
        cursorCol_ = currentLineContent.length(); // Allow cursor at one position past the end of the line
    }
} 