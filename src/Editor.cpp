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
      syntaxHighlightingEnabled_(true), filename_("untitled.txt"), currentHighlighter_(nullptr),
      highlightingStylesCacheValid_(false), commandLineHeight_(1), statusLineHeight_(1),
      displayWidth_(80), displayHeight_(24), viewableLines_(22) {
    // Ensure buffer starts non-empty for initial cursor validation, or handle empty buffer case.
    if (buffer_.isEmpty()) {
        buffer_.addLine(""); // Start with one empty line so cursor at (0,0) is valid.
    }
    validateAndClampCursor(); 

    // Initialize syntax highlighting manager with the buffer
    syntaxHighlightingManager_.setBuffer(&buffer_);
    syntaxHighlightingManager_.setEnabled(syntaxHighlightingEnabled_);

    // Constants for minimum dimensions
    const int MIN_DISPLAY_WIDTH = 10;
    const int MIN_DISPLAY_HEIGHT = 5; 
    const int MIN_VIEWABLE_LINES = 1;

    // Get actual terminal dimensions with safe defaults
    int termWidth = std::max(MIN_DISPLAY_WIDTH, getTerminalWidth());
    int termHeight = std::max(MIN_DISPLAY_HEIGHT, getTerminalHeight());

    // Calculate actual displayWidth based on terminal width
    displayWidth_ = termWidth;

    // Calculate actual displayHeight based on terminal height, considering command/status lines
    displayHeight_ = termHeight;

    // Calculate viewableLines_ (lines for text content) with bounds checks
    if (displayHeight_ > (commandLineHeight_ + statusLineHeight_)) {
        viewableLines_ = displayHeight_ - commandLineHeight_ - statusLineHeight_;
    } else {
        viewableLines_ = MIN_VIEWABLE_LINES; 
    }
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

    auto command = std::make_unique<DeleteCharCommand>(true); // isBackspace = true
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

    auto command = std::make_unique<DeleteCharCommand>(false); // isBackspace = false
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
    
    // Create and execute a JoinLinesCommand
    auto command = std::make_unique<JoinLinesCommand>(cursorLine_); // Pass line to join
    commandManager_.executeCommand(std::move(command), *this);
    // The command will handle cursor update and cache invalidation.
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

void Editor::setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    // Set selection start and end coordinates
    selectionStartLine_ = startLine;
    selectionStartCol_ = startCol;
    selectionEndLine_ = endLine;
    selectionEndCol_ = endCol;
    
    // Ensure start is before end in document order
    if (selectionStartLine_ > selectionEndLine_ || 
        (selectionStartLine_ == selectionEndLine_ && selectionStartCol_ > selectionEndCol_)) {
        // Swap start and end if needed
        std::swap(selectionStartLine_, selectionEndLine_);
        std::swap(selectionStartCol_, selectionEndCol_);
    }
    
    // Set hasSelection to true (since coordinates are explicitly being set)
    hasSelection_ = true;
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
        // Iterate downwards to avoid index invalidation issues with DeleteLineCommand
        for (size_t i = selectionEndLine_; i > selectionStartLine_; --i) {
            // Note: DeleteLineCommand needs to be robust and its execute/undo must invalidate cache.
            auto deleteLineCmd = std::make_unique<DeleteLineCommand>(i);
            // These are executed directly as part of building the compound command.
            // The CompoundCommand's undo will call undo on these sub-commands.
            deleteLineCmd->execute(*this); 
            compoundCommand->addCommand(std::move(deleteLineCmd));
        }
        
        // Replace first line with combination of firstLineStart + lastLineEnd
        // Note: ReplaceLineCommand needs to be robust and its execute/undo must invalidate cache.
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
        
        // Note: ReplaceLineCommand needs to be robust and its execute/undo must invalidate cache.
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
    // This assumes CompoundCommand itself doesn't call invalidateHighlightingCache, 
    // but its constituent commands do.
    commandManager_.addCommand(std::move(compoundCommand));
    
    // Invalidate the highlighting cache AFTER the compound command is fully built and its parts executed.
    // However, if sub-commands already invalidate, this might be redundant or cause multiple invalidations.
    // It's safer if individual commands (DeleteLine, ReplaceLine) called by execute() above handle it.
    // The current refactor of those commands in EditorCommands.h ensures they do.
    // So, no explicit invalidateHighlightingCache() here is needed if sub-commands are correct.
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
    if (term.empty()) {
        return false;
    }

    try {
        if (!caseSensitive) {
            std::string lowerLine = line;
            std::string lowerTerm = term;
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            matchPos = lowerLine.find(lowerTerm, startPos);

            if (matchPos == std::string::npos) {
                return false;
            }
            matchLength = term.length(); // Use original term's length
            return true;
        } else {
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

bool Editor::performSearchLogic(const std::string& searchTerm, bool caseSensitive, bool isNewSearch) {
    if (searchTerm.empty() || buffer_.isEmpty()) {
        return false;
    }

    // Define search start position
    size_t startLine, startCol;
    
    if (isNewSearch) {
        // New search: start from current cursor position
        currentSearchTerm_ = searchTerm;
        currentSearchCaseSensitive_ = caseSensitive;
        startLine = cursorLine_;
        startCol = cursorCol_;
        searchWrapped_ = false;
    } else {
        // Search next: start from one position after current cursor
        if (currentSearchTerm_.empty()) return false;
        
        startLine = cursorLine_;
        startCol = cursorCol_;
        
        // Advance position by one to avoid finding the same match
        if (hasSelection_ && 
            cursorLine_ == selectionEndLine_ && 
            cursorCol_ == selectionEndCol_) {
            // Always advance at least one position from the current cursor
            if (startCol < buffer_.lineLength(startLine)) {
                startCol++;
            } else if (startLine < buffer_.lineCount() - 1) {
                startLine++;
                startCol = 0;
            } else {
                // At the very end of the buffer, wrap around to the beginning
                searchWrapped_ = true;
                startLine = 0;
                startCol = 0;
            }
        }
    }
    
    // Store these positions for potential wrap-around
    lastSearchLine_ = startLine;
    lastSearchCol_ = startCol;
    
    // Initialize match variables
    size_t matchPos = 0;
    size_t matchLength = 0;
    size_t matchStartLine = 0;
    size_t matchStartCol = 0;
    size_t matchEndLine = 0;
    size_t matchEndCol = 0;
    bool found = false;
    
    // First search pass: from start position to end of buffer
    size_t line = startLine;
    size_t col = startCol;
    
    while (line < buffer_.lineCount() && !found) {
        const std::string& lineText = buffer_.getLine(line);
        
        if (findMatchInLine(lineText, currentSearchTerm_, col, currentSearchCaseSensitive_, matchPos, matchLength)) {
            matchStartLine = line;
            matchStartCol = matchPos;
            matchEndLine = line;
            matchEndCol = matchPos + matchLength;
            found = true;
            break;
        }
        
        // Move to next line
        line++;
        col = 0; // Start from beginning of next line
    }
    
    // If not found, and we haven't wrapped already, try from beginning
    if (!found && !searchWrapped_) {
        searchWrapped_ = true;
        
        // Second pass: from beginning to original start position
        for (line = 0; line <= lastSearchLine_ && !found; line++) {
            const std::string& lineText = buffer_.getLine(line);
            
            // Determine how far to search in this line
            // size_t endCol = (line == lastSearchLine_) ? lastSearchCol_ : lineText.length();
            
            if (findMatchInLine(lineText, currentSearchTerm_, 0, currentSearchCaseSensitive_, matchPos, matchLength)) {
                // If this line is the original start line, make sure we're not finding beyond our start col
                if (line == lastSearchLine_ && matchPos >= lastSearchCol_) {
                    continue; // Skip this match as it's beyond our original start position
                }
                
                matchStartLine = line;
                matchStartCol = matchPos;
                matchEndLine = line;
                matchEndCol = matchPos + matchLength;
                found = true;
                break;
            }
        }
    }
    
    if (found) {
        // Set selection to the match
        hasSelection_ = true;
        selectionStartLine_ = matchStartLine;
        selectionStartCol_ = matchStartCol;
        selectionEndLine_ = matchEndLine;
        selectionEndCol_ = matchEndCol;
        
        // Set cursor to end of match (important for test expectations)
        setCursor(matchEndLine, matchEndCol);
        
        return true;
    } else {
        // No match found
        if (isNewSearch) {
            // For new search, leave cursor where it was and clear selection
            clearSelection();
        }
        return false;
    }
}

bool Editor::search(const std::string& searchTerm, bool caseSensitive) {
    // Directly use performSearchLogic instead of creating a SearchCommand to avoid recursion
    return performSearchLogic(searchTerm, caseSensitive, true);
}

bool Editor::searchNext() {
    if (currentSearchTerm_.empty()) {
        return false;
    }
    
    // Start search from cursor position
    size_t startLine = cursorLine_;
    size_t startCol = cursorCol_;
    
    // If we have a selection and cursor is at the end of selection,
    // we need to advance to avoid finding the same match again
    if (hasSelection_ && 
        cursorLine_ == selectionEndLine_ && 
        cursorCol_ == selectionEndCol_) {
        // Advance position by one character
        if (cursorCol_ < buffer_.lineLength(cursorLine_)) {
            startCol++;
        } else if (cursorLine_ < buffer_.lineCount() - 1) {
            startLine++;
            startCol = 0;
        }
    }
    
    // Update last search position
    lastSearchLine_ = startLine;
    lastSearchCol_ = startCol;
    
    // For "searchNext", we call performSearchLogic with isNewSearch = false.
    // This reuses the currentSearchTerm_ and other search state.
    return performSearchLogic(currentSearchTerm_, currentSearchCaseSensitive_, false /*isNewSearch*/);
}

// Helper to delete a range of text directly from the buffer.
// This needs to be robust for multi-line ranges.
void Editor::directDeleteTextRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    if (startLine == endLine) {
        // Single line deletion
        if (startCol < endCol && startLine < buffer_.lineCount()) {
            std::string& line = buffer_.getLine(startLine); // Need non-const access
            line.erase(startCol, endCol - startCol);
        }
    } else {
        // Multi-line deletion
        if (startLine >= buffer_.lineCount() || endLine >= buffer_.lineCount()) return; // Invalid range

        std::string firstLinePrefix = buffer_.getLine(startLine).substr(0, startCol);
        std::string lastLineSuffix = buffer_.getLine(endLine).substr(endCol);

        // Delete intermediate lines (from endLine - 1 down to startLine + 1)
        for (size_t i = endLine -1; i > startLine; --i) {
            buffer_.deleteLine(i);
        }

        // Replace startLine with combined first and last part, then delete the (original) endLine 
        // which is now adjacent if not the same.
        buffer_.replaceLine(startLine, firstLinePrefix + lastLineSuffix);
        if (startLine < endLine) { // If they were different lines originally
             // The original endLine content is now merged into startLine.
             // The line that *was* endLine needs to be deleted if it wasn't startLine.
             // After deleting intermediate lines, the original endLine is now at startLine + 1.
            if (startLine + 1 < buffer_.lineCount()) {
                 buffer_.deleteLine(startLine + 1);
            }
        }
    }
    // Note: Cursor update and cache invalidation should be handled by the caller (performReplaceLogic)
}

// Helper to insert text, potentially multi-line, directly into the buffer.
void Editor::directInsertText(size_t line, size_t col, const std::string& text, size_t& outEndLine, size_t& outEndCol) {
    std::string currentSegment;
    outEndLine = line;
    outEndCol = col;

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') {
            if (!currentSegment.empty()) {
                buffer_.insertString(outEndLine, outEndCol, currentSegment);
                outEndCol += currentSegment.length();
                currentSegment.clear();
            }
            std::string textAfterCursor = buffer_.getLine(outEndLine).substr(outEndCol);
            buffer_.replaceLine(outEndLine, buffer_.getLine(outEndLine).substr(0, outEndCol));
            buffer_.insertLine(outEndLine + 1, textAfterCursor);
            outEndLine++;
            outEndCol = 0;
        } else {
            currentSegment += text[i];
        }
    }
    if (!currentSegment.empty()) {
        buffer_.insertString(outEndLine, outEndCol, currentSegment);
        outEndCol += currentSegment.length();
    }
    // Note: Cursor update and cache invalidation should be handled by the caller (performReplaceLogic)
}

bool Editor::performReplaceLogic(
    const std::string& searchTerm, 
    const std::string& replacementText, 
    bool caseSensitive, 
    std::string& outOriginalText, 
    size_t& outReplacedAtLine, 
    size_t& outReplacedAtCol,
    size_t& outOriginalEndLine, // To store end of replaced text for multi-line
    size_t& outOriginalEndCol   // To store end of replaced text for multi-line
) {
    if (searchTerm.empty() || buffer_.isEmpty()) {
        return false;
    }

    size_t matchStartLine = 0, matchStartCol = 0, matchEndLine = 0, matchEndCol = 0;
    std::string matchedTextContent;
    bool matchFound = false;

    // 1. Find Phase
    if (hasSelection_) {
        std::string currentSelectedText = getSelectedText();
        std::string tempSearchTerm = searchTerm;
        std::string tempSelectedText = currentSelectedText;

        if (!caseSensitive) {
            std::transform(tempSearchTerm.begin(), tempSearchTerm.end(), tempSearchTerm.begin(), 
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            std::transform(tempSelectedText.begin(), tempSelectedText.end(), tempSelectedText.begin(), 
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        }

        if (tempSelectedText == tempSearchTerm) {
            matchStartLine = selectionStartLine_;
            matchStartCol = selectionStartCol_;
            matchEndLine = selectionEndLine_;
            matchEndCol = selectionEndCol_;
            matchedTextContent = currentSelectedText; 
            matchFound = true;
        }
    }

    if (!matchFound) {
        // No matching selection, or no selection at all. Try to find next occurrence.
        // Need a temporary search without altering main editor search state or selection.
        // This is a simplified find mechanism for the replace logic.
        size_t currentL = cursorLine_;
        size_t currentC = cursorCol_;
        bool wrappedForThisSearch = false;
        
        // Search from currentL, currentC to end of buffer
        for (size_t l = currentL; l < buffer_.lineCount(); ++l) {
            const std::string& lineText = buffer_.getLine(l);
            size_t searchStartC = (l == currentL) ? currentC : 0;
            size_t tempMatchPos, tempMatchLen;
            if (findMatchInLine(lineText, searchTerm, searchStartC, caseSensitive, tempMatchPos, tempMatchLen)) {
                matchStartLine = l; matchStartCol = tempMatchPos;
                matchEndLine = l; matchEndCol = tempMatchPos + tempMatchLen;
                matchedTextContent = lineText.substr(tempMatchPos, tempMatchLen);
                matchFound = true; break;
            }
        }
        // Search from start of buffer to currentL, currentC (wrap around)
        if (!matchFound) {
            wrappedForThisSearch = true;
            for (size_t l = 0; l < currentL || (l == currentL && 0 < currentC); ++l) {
                if (l >= buffer_.lineCount()) break; 
                const std::string& lineText = buffer_.getLine(l);
                size_t searchEndC = (l == currentL) ? currentC : lineText.length();
                size_t tempMatchPos, tempMatchLen;
                if (findMatchInLine(lineText, searchTerm, 0, caseSensitive, tempMatchPos, tempMatchLen)) {
                    if (tempMatchPos + tempMatchLen <= searchEndC) { // Ensure match is within allowed range
                        matchStartLine = l; matchStartCol = tempMatchPos;
                        matchEndLine = l; matchEndCol = tempMatchPos + tempMatchLen;
                        matchedTextContent = lineText.substr(tempMatchPos, tempMatchLen);
                        matchFound = true; break;
                    }
                }
            }
        }
    }

    if (!matchFound) {
        clearSelection(); // Clear any selection if no match found or selection didn't match
        return false;
    }

    // 2. Replace Phase - direct buffer manipulation
    outOriginalText = matchedTextContent;
    outReplacedAtLine = matchStartLine;
    outReplacedAtCol = matchStartCol;
    outOriginalEndLine = matchEndLine;
    outOriginalEndCol = matchEndCol;

    // A. Delete the originalTextRange
    directDeleteTextRange(matchStartLine, matchStartCol, matchEndLine, matchEndCol);
    // Cursor is now effectively at matchStartLine, matchStartCol after deletion.
    setCursor(matchStartLine, matchStartCol);
    clearSelection(); // Selection is now invalid after deletion.

    // B. Insert the replacementText
    size_t replacementEndLine, replacementEndCol;
    directInsertText(matchStartLine, matchStartCol, replacementText, replacementEndLine, replacementEndCol);
    
    // C. Update cursor and selection to the new text
    setCursor(replacementEndLine, replacementEndCol);
    hasSelection_ = true;
    selectionStartLine_ = matchStartLine;
    selectionStartCol_ = matchStartCol;
    selectionEndLine_ = replacementEndLine;
    selectionEndCol_ = replacementEndCol;
    
    // Cache invalidation is done by ReplaceCommand after this method returns true.
    return true;
}

bool Editor::replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive) {
    // This method now creates and executes the ReplaceCommand.
    // The ReplaceCommand's execute will call performReplaceLogic.
    auto command = std::make_unique<ReplaceCommand>(searchTerm, replacementText, caseSensitive);
    commandManager_.executeCommand(std::move(command), *this);
    // The command manager executes it, performReplaceLogic is called, which might change selection.
    // ReplaceCommand needs to return success based on performReplaceLogic.
    // For now, assume command will set some state if needed, or we check selection.
    // The command's success is tracked internally by the command object itself.
    // We can get the last executed command from manager if needed, or assume this function's return is informative.
    // For simplicity, let's assume if a selection exists and matches replacement, it was successful.
    // However, the ReplaceCommand itself should store success from performReplaceLogic.
    // The command will determine if it was successful.
    // We could check commandManager_.wasLastCommandSuccessful() if such a method existed.
    // For now, editor.replace can return true if *any* action was taken by the command.
    // A more robust way: the command itself returns success from its execute.
    // Since commandManager_.executeCommand is void, we rely on editor state.
    return commandManager_.canUndo(); // A simple proxy: if something was done, it can be undone.
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
    syntaxHighlightingManager_.setEnabled(enable);
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
    
    // If no filename or highlighting disabled, exit early
    if (filename_.empty() || !syntaxHighlightingEnabled_) {
        syntaxHighlightingManager_.setHighlighter(nullptr);
        invalidateHighlightingCache();
        return;
    }
    
    // Get a shared_ptr to the highlighter for the file's extension
    std::shared_ptr<SyntaxHighlighter> highlighter = 
        SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension(filename_);
    
    // Set the highlighter in the manager and keep raw pointer for compatibility
    syntaxHighlightingManager_.setHighlighter(highlighter);
    currentHighlighter_ = highlighter.get();
    
    invalidateHighlightingCache();
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
    
    // Calculate visible range (simplification - can be implemented based on topVisibleLine_ and viewableLines_)
    size_t startLine = topVisibleLine_;
    size_t endLine = std::min(buffer_.lineCount(), topVisibleLine_ + viewableLines_) - 1;
    
    // Invalidate the visible lines in the manager
    syntaxHighlightingManager_.invalidateLines(startLine, endLine);
}

void Editor::updateHighlightingCache() const {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        cachedHighlightStyles_ = std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
    } else {
        // Calculate visible range (simplification)
        size_t startLine = topVisibleLine_;
        size_t endLine = std::min(buffer_.lineCount(), topVisibleLine_ + viewableLines_) - 1;
        
        // Set visible range in manager and get styles from it
        syntaxHighlightingManager_.setVisibleRange(startLine, endLine);
        cachedHighlightStyles_ = syntaxHighlightingManager_.getHighlightingStyles(startLine, endLine);
    }
    
    highlightingStylesCacheValid_ = true;
}

// --- Stubbed Terminal Dimension Getters ---
int Editor::getTerminalWidth() const {
    // STUB: Replace with actual terminal width detection logic
    // For example, using ncurses getmaxx(stdscr) or platform-specific APIs
    return 80; 
}

int Editor::getTerminalHeight() const {
    // STUB: Replace with actual terminal height detection logic
    // For example, using ncurses getmaxy(stdscr) or platform-specific APIs
    return 24;
}

// --- File Operations ---
bool Editor::openFile(const std::string& filename) {
    if (buffer_.loadFromFile(filename)) {
        filename_ = filename; // Store the new filename
        setCursor(0, 0);
        clearSelection();
        commandManager_.clear(); // Corrected: Use existing clear() method
        setModified(false);
        detectAndSetHighlighter(); // Detect highlighter for new file
        invalidateHighlightingCache();
        return true;
    }
    std::cerr << "Error: Could not open file \"" << filename << "\"" << std::endl;
    return false;
}

bool Editor::saveFile(const std::string& newFilename /* = "" */) {
    std::string fileToSave = newFilename;
    if (fileToSave.empty()) {
        fileToSave = filename_; // Use current filename if none provided
    }

    if (fileToSave.empty() || fileToSave == "untitled.txt") {
        std::cerr << "Error: Filename not specified. Please use 'saveas <filename>' or open/save a file first to set a filename." << std::endl;
        return false;
    }

    if (buffer_.saveToFile(fileToSave)) {
        filename_ = fileToSave; // Update internal filename if saved to a new one (e.g. saveas)
        setModified(false);
        detectAndSetHighlighter(); // Re-detect in case extension changed due to saveas
        return true;
    }
    std::cerr << "Error: Could not save file to \"" << fileToSave << "\"" << std::endl;
    return false;
} 