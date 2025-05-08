#include "Editor.h"
#include "EditorCommands.h"
#include <iostream> // For std::cout, std::cerr (used in printView, and potentially by TextBuffer methods if they still print errors)
#include <algorithm> // For std::min, std::max
#include <cctype>    // For isalnum, isspace

Editor::Editor()
    : buffer_(), cursorLine_(0), cursorCol_(0), 
      hasSelection_(false), selectionStartLine_(0), selectionStartCol_(0),
      selectionEndLine_(0), selectionEndCol_(0), clipboard_(""),
      currentSearchTerm_(""), currentSearchCaseSensitive_(true),
      lastSearchLine_(0), lastSearchCol_(0), searchWrapped_(false),
      syntaxHighlightingEnabled_(true), filename_(""), currentHighlighter_(nullptr),
      highlightingStylesCacheValid_(false) {
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

    // Get highlighting styles if enabled
    std::vector<std::vector<SyntaxStyle>> styles;
    bool useHighlighting = syntaxHighlightingEnabled_ && currentHighlighter_;
    
    if (useHighlighting) {
        styles = getHighlightingStyles();
    }

    // ANSI color codes for syntax highlighting
    const std::string resetColor = "\033[0m";
    std::map<SyntaxColor, std::string> colorCodes = {
        {SyntaxColor::Default, "\033[0m"},
        {SyntaxColor::Keyword, "\033[1;34m"},     // Bold Blue
        {SyntaxColor::Type, "\033[1;32m"},        // Bold Green
        {SyntaxColor::String, "\033[0;31m"},      // Red
        {SyntaxColor::Comment, "\033[0;32m"},     // Green
        {SyntaxColor::Number, "\033[0;35m"},      // Magenta
        {SyntaxColor::Identifier, "\033[0m"},     // Default
        {SyntaxColor::Preprocessor, "\033[0;33m"},// Yellow
        {SyntaxColor::Operator, "\033[1;37m"},    // Bold White
        {SyntaxColor::Function, "\033[0;36m"}     // Cyan
    };

    for (size_t i = 0; i < buffer_.lineCount(); ++i) {
        if (i == cursorLine_) {
            const std::string& currentLine = buffer_.getLine(i);
            
            // Handle line with cursor differently
            if (useHighlighting && i < styles.size()) {
                // Print line with syntax highlighting and cursor
                const auto& lineStyles = styles[i];
                size_t currentPos = 0;
                
                // Keep track of currently active color
                std::string currentColor = resetColor;
                
                for (size_t col = 0; col <= currentLine.length(); ++col) {
                    // Check if we need to change color at this position
                    for (const auto& style : lineStyles) {
                        if (col == style.startCol) {
                            os << resetColor << colorCodes[style.color];
                            currentColor = colorCodes[style.color];
                        }
                        else if (col == style.endCol) {
                            os << resetColor;
                            currentColor = resetColor;
                            
                            // Check if there's another style starting at the same position
                            for (const auto& nextStyle : lineStyles) {
                                if (nextStyle.startCol == col) {
                                    os << colorCodes[nextStyle.color];
                                    currentColor = colorCodes[nextStyle.color];
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Print cursor if we're at cursor position
                    if (col == cursorCol_) {
                        os << resetColor << "|" << currentColor;
                    }
                    
                    // Print character if not at end of line
                    if (col < currentLine.length()) {
                        os << currentLine[col];
                    }
                }
                
                // Reset color at end of line
                os << resetColor;
            } 
            else {
                // Simple conceptual cursor: print part before, cursor marker, part after
                if (cursorCol_ < currentLine.length()) {
                    os << currentLine.substr(0, cursorCol_);
                    os << "|"; // Cursor marker
                    os << currentLine.substr(cursorCol_);
                } else {
                    os << currentLine;
                    os << "|"; // Cursor at end of line
                }
            }
            os << "  <-- Cursor Line (" << cursorLine_ << ", " << cursorCol_ << ")";
        } 
        else {
            // Regular line without cursor
            const std::string& line = buffer_.getLine(i);
            
            if (useHighlighting && i < styles.size()) {
                // Print line with syntax highlighting
                const auto& lineStyles = styles[i];
                size_t currentPos = 0;
                
                // Keep track of currently active color
                std::string currentColor = resetColor;
                
                for (size_t col = 0; col <= line.length(); ++col) {
                    // Check if we need to change color at this position
                    for (const auto& style : lineStyles) {
                        if (col == style.startCol) {
                            os << resetColor << colorCodes[style.color];
                            currentColor = colorCodes[style.color];
                        }
                        else if (col == style.endCol) {
                            os << resetColor;
                            currentColor = resetColor;
                            
                            // Check if there's another style starting at the same position
                            for (const auto& nextStyle : lineStyles) {
                                if (nextStyle.startCol == col) {
                                    os << colorCodes[nextStyle.color];
                                    currentColor = colorCodes[nextStyle.color];
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Print character if not at end of line
                    if (col < line.length()) {
                        os << line[col];
                    }
                }
                
                // Reset color at end of line
                os << resetColor;
            } 
            else {
                os << line;
            }
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
    auto command = std::make_unique<AddLineCommand>(text);
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::insertLine(size_t lineIndex, const std::string& text) {
    auto command = std::make_unique<InsertLineCommand>(lineIndex, text);
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::deleteLine(size_t lineIndex) {
    auto command = std::make_unique<DeleteLineCommand>(lineIndex);
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::replaceLine(size_t lineIndex, const std::string& text) {
    auto command = std::make_unique<ReplaceLineCommand>(lineIndex, text);
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::typeText(const std::string& textToInsert) {
    if (textToInsert.empty()) {
        return;
    }

    if (buffer_.isEmpty()) {
        validateAndClampCursor(); // Will add an empty line if needed.
    }

    // Check if there's a selection that needs to be deleted first
    if (hasSelection_) {
        deleteSelectedText();
    }

    // For single character or simple text, create and execute a command
    if (textToInsert.find('\n') == std::string::npos) {
        // Create and execute an InsertTextCommand
        auto command = std::make_unique<InsertTextCommand>(textToInsert);
        commandManager_.executeCommand(std::move(command), *this);
    } else {
        // For text with newlines, handle each segment separately
        std::string segment;
        for (size_t i = 0; i < textToInsert.length(); ++i) {
            if (textToInsert[i] == '\n') {
                if (!segment.empty()) {
                    // Insert text before newline
                    auto textCommand = std::make_unique<InsertTextCommand>(segment);
                    commandManager_.executeCommand(std::move(textCommand), *this);
                    segment.clear();
                }
                // Handle newline as a separate command
                auto newLineCommand = std::make_unique<NewLineCommand>();
                commandManager_.executeCommand(std::move(newLineCommand), *this);
            } else {
                segment += textToInsert[i];
            }
        }
        
        // Insert any remaining text after the last newline
        if (!segment.empty()) {
            auto textCommand = std::make_unique<InsertTextCommand>(segment);
            commandManager_.executeCommand(std::move(textCommand), *this);
        }
    }
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
    
    if (ch == '\n') {
        // For newline, use NewLineCommand
        auto command = std::make_unique<NewLineCommand>();
        commandManager_.executeCommand(std::move(command), *this);
    } else {
        // For regular character, use InsertTextCommand with single char
        std::string charStr(1, ch);
        auto command = std::make_unique<InsertTextCommand>(charStr);
        commandManager_.executeCommand(std::move(command), *this);
    }
}

void Editor::backspace() {
    if (hasSelection_) {
        deleteSelectedText();
        return;
    }
    
    if (cursorLine_ == 0 && cursorCol_ == 0) {
        return; // Already at the start of the buffer
    }

    auto command = std::make_unique<DeleteTextCommand>();
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::deleteForward() {
    if (hasSelection_) {
        deleteSelectedText();
        return;
    }
    
    if (buffer_.isEmpty()) {
        return;
    }

    const auto& line = buffer_.getLine(cursorLine_);
    if (cursorCol_ >= line.length() && cursorLine_ >= buffer_.lineCount() - 1) {
        return; // Already at the end of the buffer
    }

    auto command = std::make_unique<DeleteForwardCommand>();
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::newLine() {
    if (hasSelection_) {
        deleteSelectedText();
    }
    
    auto command = std::make_unique<NewLineCommand>();
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::joinWithNextLine() {
    if (cursorLine_ >= buffer_.lineCount() - 1) return; // Can't join if last line
    
    // Store the line length for cursor positioning
    size_t firstLineLength = buffer_.lineLength(cursorLine_);
    
    buffer_.joinLines(cursorLine_);
    
    // Set cursor at the join point
    cursorCol_ = firstLineLength;
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
    if (!hasSelection_) {
        return;
    }
    
    // Create a compound command for undo/redo
    auto compoundCommand = std::make_unique<CompoundCommand>();
    
    // Function to order selection points
    auto orderSelection = [&]() {
        if (selectionStartLine_ > selectionEndLine_ || 
            (selectionStartLine_ == selectionEndLine_ && selectionStartCol_ > selectionEndCol_)) {
            // Swap start and end if needed
            std::swap(selectionStartLine_, selectionEndLine_);
            std::swap(selectionStartCol_, selectionEndCol_);
        }
    };
    
    // Ensure selection points are in order
    orderSelection();
    
    // If selection spans multiple lines
    if (selectionStartLine_ != selectionEndLine_) {
        // Handle first line partially
        std::string firstLine = buffer_.getLine(selectionStartLine_);
        std::string firstLineStart = firstLine.substr(0, selectionStartCol_);
        
        // Handle last line partially
        std::string lastLine = buffer_.getLine(selectionEndLine_);
        std::string lastLineEnd = lastLine.substr(selectionEndCol_);
        
        // Delete all fully selected lines between first and last
        for (size_t i = selectionEndLine_; i > selectionStartLine_; --i) {
            auto deleteLineCmd = std::make_unique<DeleteLineCommand>(i);
            deleteLineCmd->execute(*this);
            compoundCommand->addCommand(std::move(deleteLineCmd));
        }
        
        // Replace first line with combination of firstLineStart + lastLineEnd
        auto replaceLineCmd = std::make_unique<ReplaceLineCommand>(
            selectionStartLine_, firstLineStart + lastLineEnd);
        replaceLineCmd->execute(*this);
        compoundCommand->addCommand(std::move(replaceLineCmd));
        
        // Set cursor at join point
        setCursor(selectionStartLine_, selectionStartCol_);
    } 
    // Selection within a single line
    else {
        std::string line = buffer_.getLine(selectionStartLine_);
        std::string newLine = line.substr(0, selectionStartCol_) + 
                            line.substr(selectionEndCol_);
        
        auto replaceLineCmd = std::make_unique<ReplaceLineCommand>(
            selectionStartLine_, newLine);
        replaceLineCmd->execute(*this);
        compoundCommand->addCommand(std::move(replaceLineCmd));
        
        // Set cursor at deletion point
        setCursor(selectionStartLine_, selectionStartCol_);
    }
    
    // Clear selection state
    clearSelection();
    
    // Add the compound command to the command manager
    commandManager_.addCommand(std::move(compoundCommand));
    
    // Invalidate the highlighting cache
    invalidateHighlightingCache();
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
    if (clipboard_.empty()) {
        return;
    }
    
    // Delete any selected text first
    if (hasSelection_) {
        deleteSelectedText();
    }
    
    // Use typeText to handle multiline paste
    typeText(clipboard_);
    
    // No need to invalidate the cache here because typeText will do it
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

// Add implementations for the undo/redo methods
bool Editor::undo() {
    return commandManager_.undo(*this);
}

bool Editor::redo() {
    return commandManager_.redo(*this);
}

bool Editor::canUndo() const {
    return commandManager_.canUndo();
}

bool Editor::canRedo() const {
    return commandManager_.canRedo();
}

// Search operations implementation
bool Editor::findMatchInLine(const std::string& line, const std::string& term, 
                           size_t startPos, bool caseSensitive, size_t& matchPos, size_t& matchLength) {
    // Initialize output parameters to safe defaults
    matchPos = std::string::npos;
    matchLength = 0;
    
    // Validate inputs
    if (term.empty()) {
        return false;
    }
    
    if (line.empty()) {
        return false;
    }
    
    // Bounds check
    if (startPos >= line.length()) {
        return false;
    }

    try {
        // For case-insensitive search, convert both strings to lowercase for comparison
        if (!caseSensitive) {
            std::string lowerLine = line;
            std::string lowerTerm = term;
            
            // Use safe transformation with bounds checking
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), 
                          [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(), 
                          [](unsigned char c) { return std::tolower(c); });
            
            matchPos = lowerLine.find(lowerTerm, startPos);
            if (matchPos == std::string::npos) {
                return false;
            }
            matchLength = term.length();
            return true;
        } else {
            // Case-sensitive search
            matchPos = line.find(term, startPos);
            if (matchPos == std::string::npos) {
                return false;
            }
            matchLength = term.length();
            return true;
        }
    } catch (const std::exception& e) {
        // Log error and return failure
        std::cerr << "Error in findMatchInLine: " << e.what() << std::endl;
        return false;
    } catch (...) {
        // Handle unknown exceptions
        std::cerr << "Unknown error in findMatchInLine" << std::endl;
        return false;
    }
}

bool Editor::search(const std::string& searchTerm, bool caseSensitive) {
    if (searchTerm.empty()) {
        return false;
    }
    
    if (buffer_.isEmpty()) {
        return false;
    }
    
    try {
        // Create and execute a search command
        auto searchCommand = std::make_unique<SearchCommand>(searchTerm, caseSensitive);
        commandManager_.executeCommand(std::move(searchCommand), *this);
        
        // Save search parameters for searchNext
        currentSearchTerm_ = searchTerm;
        currentSearchCaseSensitive_ = caseSensitive;
        
        // Start from current cursor position for next search
        lastSearchLine_ = cursorLine_;
        lastSearchCol_ = cursorCol_;
        searchWrapped_ = false;
        
        return hasSelection_; // If we have a selection, search was successful
    } catch (const std::exception& e) {
        std::cerr << "Error in search: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown error in search" << std::endl;
        return false;
    }
}

bool Editor::searchNext() {
    if (currentSearchTerm_.empty()) {
        return false;
    }
    
    if (buffer_.isEmpty()) {
        return false;
    }
    
    try {
        // Start from current cursor position
        size_t line = lastSearchLine_;
        size_t col = lastSearchCol_ + 1; // Start after the last match
        bool found = false;
        searchWrapped_ = false;
        
        // First pass: search from current position to end of buffer
        while (line < buffer_.lineCount() && !found) {
            // Safely get the line text
            if (line >= buffer_.lineCount()) {
                break;
            }
            
            const std::string& lineText = buffer_.getLine(line);
            
            // If this is the line we're starting on, use the specified column
            // Otherwise start from beginning of line
            size_t startCol = (line == lastSearchLine_) ? col : 0;
            
            size_t matchPos = 0;
            size_t matchLength = 0;
            
            if (findMatchInLine(lineText, currentSearchTerm_, startCol, 
                               currentSearchCaseSensitive_, matchPos, matchLength)) {
                // Match found, move cursor to start of match
                cursorLine_ = line;
                cursorCol_ = matchPos;
                
                // Set selection to highlight the match
                hasSelection_ = true;
                selectionStartLine_ = line;
                selectionStartCol_ = matchPos;
                selectionEndLine_ = line;
                selectionEndCol_ = matchPos + matchLength;
                
                // Store last search position
                lastSearchLine_ = line;
                lastSearchCol_ = matchPos;
                
                found = true;
                break;
            }
            
            // Move to next line
            line++;
        }
        
        // If not found, wrap around to start of buffer (if not already wrapped)
        if (!found && !searchWrapped_) {
            searchWrapped_ = true;
            line = 0;
            col = 0;
            
            while (line <= lastSearchLine_ && !found) {
                // Safety check
                if (line >= buffer_.lineCount()) {
                    break;
                }
                
                const std::string& lineText = buffer_.getLine(line);
                
                // For the last line, only search up to the original column
                size_t endCol = (line == lastSearchLine_) ? lastSearchCol_ : std::string::npos;
                
                size_t matchPos = 0;
                size_t matchLength = 0;
                
                if (findMatchInLine(lineText, currentSearchTerm_, 0, 
                                   currentSearchCaseSensitive_, matchPos, matchLength) && 
                    (endCol == std::string::npos || matchPos < endCol)) {
                    // Match found, move cursor to start of match
                    cursorLine_ = line;
                    cursorCol_ = matchPos;
                    
                    // Set selection to highlight the match
                    hasSelection_ = true;
                    selectionStartLine_ = line;
                    selectionStartCol_ = matchPos;
                    selectionEndLine_ = line;
                    selectionEndCol_ = matchPos + matchLength;
                    
                    // Store last search position
                    lastSearchLine_ = line;
                    lastSearchCol_ = matchPos;
                    
                    found = true;
                    break;
                }
                
                // Move to next line
                line++;
            }
        }
        
        // Always validate cursor position
        validateAndClampCursor();
        return found;
    } catch (const std::exception& e) {
        std::cerr << "Error in searchNext: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown error in searchNext" << std::endl;
        return false;
    }
}

bool Editor::replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive) {
    if (searchTerm.empty()) {
        return false;
    }
    
    if (buffer_.isEmpty()) {
        return false;
    }
    
    try {
        // Create and execute a replace command
        auto replaceCommand = std::make_unique<ReplaceCommand>(searchTerm, replacementText, caseSensitive);
        commandManager_.executeCommand(std::move(replaceCommand), *this);
        
        // If successful, the command will have performed the search and replacement
        return hasSelection_; // Selection indicates a successful replacement
    } catch (const std::exception& e) {
        std::cerr << "Error in replace: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown error in replace" << std::endl;
        return false;
    }
}

bool Editor::replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive) {
    if (searchTerm.empty()) {
        return false;
    }
    
    if (buffer_.isEmpty()) {
        return false;
    }
    
    try {
        // Create and execute a replace all command
        auto replaceAllCommand = std::make_unique<ReplaceAllCommand>(searchTerm, replacementText, caseSensitive);
        commandManager_.executeCommand(std::move(replaceAllCommand), *this);
        
        // The command handles everything including cursor position restoration
        return true; // Return true since the command was executed
    } catch (const std::exception& e) {
        std::cerr << "Error in replaceAll: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown error in replaceAll" << std::endl;
        return false;
    }
}

// Syntax highlighting methods
void Editor::enableSyntaxHighlighting(bool enable) {
    syntaxHighlightingEnabled_ = enable;
    invalidateHighlightingCache();
}

bool Editor::isSyntaxHighlightingEnabled() const {
    return syntaxHighlightingEnabled_;
}

void Editor::setFilename(const std::string& filename) {
    filename_ = filename;
    detectAndSetHighlighter();
}

std::string Editor::getFilename() const {
    return filename_;
}

void Editor::detectAndSetHighlighter() {
    // Reset current highlighter
    currentHighlighter_ = nullptr;
    invalidateHighlightingCache();
    
    // If no filename or highlighting disabled, exit early
    if (filename_.empty() || !syntaxHighlightingEnabled_) {
        return;
    }
    
    // Get a highlighter for the file's extension
    currentHighlighter_ = SyntaxHighlighterRegistry::getInstance().getHighlighterForExtension(filename_);
}

SyntaxHighlighter* Editor::getCurrentHighlighter() const {
    return currentHighlighter_;
}

std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() const {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        return std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
    }
    
    // If cache is invalid, update it
    if (!highlightingStylesCacheValid_) {
        updateHighlightingCache();
    }
    
    return cachedHighlightStyles_;
}

void Editor::invalidateHighlightingCache() {
    highlightingStylesCacheValid_ = false;
}

void Editor::updateHighlightingCache() const {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        cachedHighlightStyles_ = std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
    } else {
        cachedHighlightStyles_ = currentHighlighter_->highlightBuffer(buffer_);
    }
    
    highlightingStylesCacheValid_ = true;
} 