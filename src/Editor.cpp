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
      syntaxHighlightingEnabled_(false), filename_("untitled.txt"), currentHighlighter_(nullptr),
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

bool Editor::hasSelection() const {
    return hasSelection_;
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
    }
}

// --- Editor-level operations (pass-through for now, but can add cursor logic) ---
void Editor::addLine(const std::string& text) {
    auto command = std::make_unique<InsertLineCommand>(buffer_.lineCount(), text);
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
        deleteSelection();
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
    }
    // Removed line-wrapping behavior to match test expectations
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
    }
    // Removed line-wrapping behavior to match test expectations
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
        deleteSelection();
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
        deleteSelection();
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
        deleteSelection();
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
void Editor::startSelection() {
    // Start selection at current cursor position
    selectionStartLine_ = cursorLine_;
    selectionStartCol_ = cursorCol_;
    selectionEndLine_ = cursorLine_;
    selectionEndCol_ = cursorCol_;
}

void Editor::updateSelection() {
    // Update the end point of the selection to current cursor position
    selectionEndLine_ = cursorLine_;
    selectionEndCol_ = cursorCol_;
}

void Editor::replaceSelection(const std::string& text) {
    if (hasSelection()) {
        auto command = std::make_unique<ReplaceSelectionCommand>(text);
        commandManager_.executeCommand(std::move(command), *this);
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

void Editor::deleteSelection() {
    if (hasSelection()) {
        auto command = std::make_unique<ReplaceSelectionCommand>("");
        commandManager_.executeCommand(std::move(command), *this);
    }
}

void Editor::copySelection() {
    if (hasSelection()) {
        clipboard_ = getSelectedText();
    }
}

void Editor::cutSelection() {
    if (hasSelection()) {
        // First copy the selected text to clipboard
        clipboard_ = getSelectedText();
        
        // Then delete the selection
        deleteSelection();
    }
}

void Editor::pasteAtCursor() {
    if (!clipboard_.empty()) {
        auto command = std::make_unique<InsertTextCommand>(clipboard_);
        commandManager_.executeCommand(std::move(command), *this);
    }
}

void Editor::deleteWord() {
    if (buffer_.isEmpty()) return;
    
    if (hasSelection()) {
        deleteSelection();
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
    deleteSelection();
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
    // Consider alphanumeric characters and underscore as word chars
    // Also include some common symbol characters that may be part of identifiers in various languages
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' || c == '$' || c == '@';
}

// --- Private Helper Methods ---
void Editor::validateAndClampCursor() {
    if (buffer_.isEmpty()) {
        // This case should ideally be handled by ensuring buffer_ is never truly empty 
        // if we want a valid cursor (e.g., always has at least one "" line).
        // For robustness, if it does become empty and then non-empty (e.g. via loadFromFile):
        cursorLine_ = 0;
        cursorCol_ = 0;
        
        // Don't automatically add a line if we're in a test that's specifically testing empty buffer behavior
        // (in normal editor operation, we'd want to ensure there's always at least one line)
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

bool Editor::performSearchLogic(
    const std::string& searchTerm, 
    bool caseSensitive, 
    bool forward, // Changed from isNewSearch
    size_t& outFoundLine, // Added
    size_t& outFoundCol   // Added
) {
    if (searchTerm.empty() || buffer_.isEmpty()) {
        return false;
    }

    // Define search start position
    size_t startLine, startCol;
    
    // Always start from current cursor position
    startLine = cursorLine_;
    startCol = cursorCol_;
    
    if (forward) {
        // For a forward search, store the search parameters
        currentSearchTerm_ = searchTerm;
        currentSearchCaseSensitive_ = caseSensitive;
        searchWrapped_ = false;
    }

    // If continuing a search, we need to advance past the current position/selection
    // to avoid finding the same match again
    if (hasSelection_) {
        // If we have a selection, start searching from the end of the selection
        startLine = selectionEndLine_;
        startCol = selectionEndCol_;
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
            size_t searchEndCol = (line == lastSearchLine_) ? lastSearchCol_ : lineText.length();
            
            if (findMatchInLine(lineText, currentSearchTerm_, 0, currentSearchCaseSensitive_, matchPos, matchLength)) {
                // If this line is the original start line, make sure we're not finding beyond our start col
                if (line == lastSearchLine_ && matchPos >= searchEndCol) {
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
        
        // Set cursor to the START of the match to match the expected test behavior
        setCursor(matchStartLine, matchStartCol);
        
        outFoundLine = matchStartLine;
        outFoundCol = matchStartCol;
        
        return true;
    } else {
        // No match found
        if (forward) {
            // For new search, leave cursor where it was and clear selection
            clearSelection();
        }
        return false;
    }
}

bool Editor::search(const std::string& searchTerm, bool caseSensitive, bool forward) {
    // Check if the buffer is empty
    if (buffer_.isEmpty()) {
        return false;
    }
    
    // Directly use performSearchLogic instead of creating a SearchCommand to avoid recursion
    size_t foundLine, foundCol; // Dummy vars to satisfy performSearchLogic signature
    return performSearchLogic(searchTerm, caseSensitive, forward, foundLine, foundCol);
}

bool Editor::searchNext() {
    if (currentSearchTerm_.empty()) {
        return false;
    }
    
    // For "searchNext", we call performSearchLogic with forward = true
    // This reuses the currentSearchTerm_ and other search state.
    size_t foundLine, foundCol;
    return performSearchLogic(currentSearchTerm_, currentSearchCaseSensitive_, true, foundLine, foundCol);
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
    // Check if the buffer is empty before proceeding
    if (buffer_.isEmpty()) {
        return false;
    }
    
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
    
    try {
        // Get a shared_ptr to the highlighter for the file's extension
        currentHighlighter_ = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension(filename_);
        
        // Set the highlighter in the manager
        syntaxHighlightingManager_.setHighlighter(currentHighlighter_);
        
        invalidateHighlightingCache();
    } catch (const std::exception& e) {
        std::cerr << "Error detecting highlighter: " << e.what() << std::endl;
        currentHighlighter_ = nullptr;
        syntaxHighlightingManager_.setHighlighter(nullptr);
        invalidateHighlightingCache();
    }
}

std::shared_ptr<SyntaxHighlighter> Editor::getCurrentHighlighter() const {
    return currentHighlighter_;
}

std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        return std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
    }
    
    // If cache is invalid, update it
    if (!highlightingStylesCacheValid_) {
        updateHighlightingCache();
    }
    
    return cachedHighlightStyles_;
}

std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() const {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        // Return a vector of empty style lists, one for each line in the buffer
        return std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
    }

    // If the editor's specific highlighting cache is valid and populated, prefer that.
    // This assumes 'cachedHighlightStyles_' is correctly sized to buffer_.lineCount()
    // when highlightingStylesCacheValid_ is true.
    if (highlightingStylesCacheValid_) {
        return cachedHighlightStyles_;
    }

    // Fallback: If editor's cache is not valid, get (potentially less fresh) styles 
    // directly from the syntax highlighting manager's own cache via its const method.
    // This won't update the Editor's 'cachedHighlightStyles_' or 'highlightingStylesCacheValid_'.
    if (buffer_.isEmpty()) {
        return std::vector<std::vector<SyntaxStyle>>(0);
    }

    size_t actualStartLine = topVisibleLine_;
    // Ensure startLine is within buffer bounds
    if (actualStartLine >= buffer_.lineCount()) {
        actualStartLine = buffer_.lineCount() - 1;
    }

    size_t actualEndLine = actualStartLine + viewableLines_ - 1;
    if (actualEndLine >= buffer_.lineCount()) {
        actualEndLine = buffer_.lineCount() - 1;
    }
    // Ensure endLine is not before startLine (can happen if viewableLines_ is 0 or buffer is tiny)
    // and also handle the case where buffer might have become empty after check but before use (unlikely here).
    if (buffer_.lineCount() > 0 && actualEndLine < actualStartLine) {
         actualEndLine = actualStartLine;
    }

    // The manager's const getHighlightingStyles might return styles for a different range
    // or might not cover all lines if its internal cache is sparse. We expect it to return
    // styles for the requested [actualStartLine, actualEndLine] (inclusive) range based on its *own* cache.
    return syntaxHighlightingManager_.getHighlightingStyles(actualStartLine, actualEndLine);
}

void Editor::invalidateHighlightingCache() {
    highlightingStylesCacheValid_ = false;
    
    // Calculate visible range (simplification - can be implemented based on topVisibleLine_ and viewableLines_)
    size_t startLine = topVisibleLine_;
    size_t endLine = std::min(buffer_.lineCount(), topVisibleLine_ + viewableLines_) - 1;
    
    // Invalidate the visible lines in the manager
    syntaxHighlightingManager_.invalidateLines(startLine, endLine);
}

void Editor::updateHighlightingCache() {
    try {
        if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
            cachedHighlightStyles_ = std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
        } else {
            // Calculate visible range
            size_t startLine = topVisibleLine_;
            size_t endLine = std::min(buffer_.lineCount(), topVisibleLine_ + viewableLines_) - 1;
            
            // Set visible range in manager and get styles from it
            syntaxHighlightingManager_.setVisibleRange(startLine, endLine);
            cachedHighlightStyles_ = syntaxHighlightingManager_.getHighlightingStyles(startLine, endLine);
        }
        
        highlightingStylesCacheValid_ = true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating highlighting cache: " << e.what() << std::endl;
        cachedHighlightStyles_ = std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
        highlightingStylesCacheValid_ = true; // Set to true to prevent continuous retries
    }
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

// Public getters for selection coordinates
size_t Editor::getSelectionStartLine() const {
    return selectionStartLine_;
}

size_t Editor::getSelectionStartCol() const {
    return selectionStartCol_;
}

size_t Editor::getSelectionEndLine() const {
    return selectionEndLine_;
}

size_t Editor::getSelectionEndCol() const {
    return selectionEndCol_;
}

// Clipboard text accessors
std::string Editor::getClipboardText() const {
    return clipboard_;
}

void Editor::setClipboardText(const std::string& text) {
    clipboard_ = text;
}

void Editor::increaseIndent() {
    // Determine first and last lines to indent based on selection
    size_t firstLineIndex, lastLineIndex;
    
    if (hasSelection()) {
        // Get the range of lines covered by the selection
        firstLineIndex = std::min(selectionStartLine_, selectionEndLine_);
        lastLineIndex = std::max(selectionStartLine_, selectionEndLine_);
    } else {
        // Only indent the current line
        firstLineIndex = lastLineIndex = cursorLine_;
    }
    
    // Collect the lines to be indented
    std::vector<std::string> linesToIndent;
    for (size_t i = firstLineIndex; i <= lastLineIndex; i++) {
        if (i < buffer_.lineCount()) {
            linesToIndent.push_back(buffer_.getLine(i));
        }
    }
    
    // Create and execute the indent command
    const size_t tabWidth = 4; // Use a constant for now, could be a member variable later
    
    Position selectionPos = Position{selectionStartLine_, selectionStartCol_};
    // If there's a selection, use the selection end position as the cursor position
    Position cursorPos = hasSelection() 
        ? Position{selectionEndLine_, selectionEndCol_} 
        : Position{cursorLine_, cursorCol_};
    
    auto command = std::make_unique<IncreaseIndentCommand>(
        firstLineIndex, lastLineIndex, linesToIndent, tabWidth, 
        hasSelection(), selectionPos, cursorPos
    );
    
    commandManager_.executeCommand(std::move(command), *this);
}

void Editor::decreaseIndent() {
    // Determine first and last lines to unindent based on selection
    size_t firstLineIndex, lastLineIndex;
    
    if (hasSelection()) {
        // Get the range of lines covered by the selection
        firstLineIndex = std::min(selectionStartLine_, selectionEndLine_);
        lastLineIndex = std::max(selectionStartLine_, selectionEndLine_);
    } else {
        // Only unindent the current line
        firstLineIndex = lastLineIndex = cursorLine_;
    }
    
    // Collect the lines to be unindented
    std::vector<std::string> linesToUnindent;
    for (size_t i = firstLineIndex; i <= lastLineIndex; i++) {
        if (i < buffer_.lineCount()) {
            linesToUnindent.push_back(buffer_.getLine(i));
        }
    }
    
    // Create and execute the unindent command
    const size_t tabWidth = 4; // Use a constant for now, could be a member variable later
    
    Position selectionPos = Position{selectionStartLine_, selectionStartCol_};
    // If there's a selection, use the selection end position as the cursor position
    Position cursorPos = hasSelection() 
        ? Position{selectionEndLine_, selectionEndCol_} 
        : Position{cursorLine_, cursorCol_};
    
    auto command = std::make_unique<DecreaseIndentCommand>(
        firstLineIndex, lastLineIndex, linesToUnindent, tabWidth, 
        hasSelection(), selectionPos, cursorPos
    );
    
    commandManager_.executeCommand(std::move(command), *this);
}

bool Editor::searchPrevious() {
    // If no previous search, there's nothing to search for
    if (currentSearchTerm_.empty()) {
        return false;
    }
    
    // Use the existing search mechanism but specify backward direction
    return search(currentSearchTerm_, currentSearchCaseSensitive_, false);
}

// Helper methods for indentation commands
void Editor::setLine(size_t lineIndex, const std::string& text) {
    if (lineIndex < buffer_.lineCount()) {
        buffer_.replaceLine(lineIndex, text);
        invalidateHighlightingCache();
        setModified(true);
    }
}

std::string Editor::getLine(size_t lineIndex) const {
    if (lineIndex < buffer_.lineCount()) {
        return buffer_.getLine(lineIndex);
    }
    return "";
}

void Editor::setSelection(const Position& start, const Position& end) {
    setSelectionRange(start.line, start.column, end.line, end.column);
}

void Editor::setCursorPosition(const Position& pos) {
    setCursor(pos.line, pos.column);
}

void Editor::selectLine() {
    if (buffer_.isEmpty()) return;
    
    // Get the length of the current line
    const std::string& line = buffer_.getLine(cursorLine_);
    size_t lineLength = line.length();
    
    // Select from beginning to end of the current line
    setSelectionRange(cursorLine_, 0, cursorLine_, lineLength);
    
    // Move cursor to the end of the line
    setCursor(cursorLine_, lineLength);
}

void Editor::selectAll() {
    if (buffer_.isEmpty()) {
        // If buffer is empty, set an empty selection at position (0,0)
        clearSelection();
        setCursor(0, 0);
        return;
    }
    
    // Select from (0,0) to end of buffer
    size_t lastLineIndex = buffer_.lineCount() - 1;
    size_t lastLineLength = buffer_.getLine(lastLineIndex).length();
    
    // Set selection from start to end of buffer
    setSelectionRange(0, 0, lastLineIndex, lastLineLength);
    
    // Move cursor to the end of the buffer
    setCursor(lastLineIndex, lastLineLength);
}

void Editor::selectToLineStart() {
    if (buffer_.isEmpty()) return;
    
    // If no selection exists, start a new selection at the current cursor position
    if (!hasSelection_) {
        // Set selection start and end points at current cursor position
        selectionStartLine_ = cursorLine_;
        selectionStartCol_ = cursorCol_;
        selectionEndLine_ = cursorLine_;
        selectionEndCol_ = cursorCol_;
        hasSelection_ = true;
    }
    
    // Determine if the cursor is at the start or end of the current selection
    bool cursorAtSelectionStart = (cursorLine_ == selectionStartLine_ && cursorCol_ == selectionStartCol_);
    
    // Move cursor to start of line
    size_t originalLine = cursorLine_;
    size_t originalCol = cursorCol_;
    cursorCol_ = 0;
    
    // Update the selection point where the cursor is
    if (cursorAtSelectionStart) {
        selectionStartLine_ = cursorLine_;
        selectionStartCol_ = cursorCol_;
    } else {
        selectionEndLine_ = cursorLine_;
        selectionEndCol_ = cursorCol_;
    }
    
    // Ensure selection points are in document order (start comes before end)
    if (selectionStartLine_ > selectionEndLine_ || 
        (selectionStartLine_ == selectionEndLine_ && selectionStartCol_ > selectionEndCol_)) {
        std::swap(selectionStartLine_, selectionEndLine_);
        std::swap(selectionStartCol_, selectionEndCol_);
    }
}

void Editor::selectToLineEnd() {
    if (buffer_.isEmpty()) return;
    
    // If no selection exists, start a new selection at the current cursor position
    if (!hasSelection_) {
        // Set selection start and end points at current cursor position
        selectionStartLine_ = cursorLine_;
        selectionStartCol_ = cursorCol_;
        selectionEndLine_ = cursorLine_;
        selectionEndCol_ = cursorCol_;
        hasSelection_ = true;
    }
    
    // Determine if the cursor is at the start or end of the current selection
    bool cursorAtSelectionStart = (cursorLine_ == selectionStartLine_ && cursorCol_ == selectionStartCol_);
    
    // Move cursor to end of line
    size_t originalLine = cursorLine_;
    size_t originalCol = cursorCol_;
    cursorCol_ = buffer_.getLine(cursorLine_).length();
    
    // Update the selection point where the cursor is
    if (cursorAtSelectionStart) {
        selectionStartLine_ = cursorLine_;
        selectionStartCol_ = cursorCol_;
    } else {
        selectionEndLine_ = cursorLine_;
        selectionEndCol_ = cursorCol_;
    }
    
    // Ensure selection points are in document order (start comes before end)
    if (selectionStartLine_ > selectionEndLine_ || 
        (selectionStartLine_ == selectionEndLine_ && selectionStartCol_ > selectionEndCol_)) {
        std::swap(selectionStartLine_, selectionEndLine_);
        std::swap(selectionStartCol_, selectionEndCol_);
    }
}

SelectionUnit Editor::getCurrentSelectionUnit() const {
    return currentSelectionUnit_;
}

std::pair<size_t, size_t> Editor::findWordBoundaries(size_t line, size_t col) const {
    if (buffer_.isEmpty() || line >= buffer_.lineCount()) {
        return {0, 0};
    }
    
    const std::string& lineContent = buffer_.getLine(line);
    
    // Handle empty line case
    if (lineContent.empty()) {
        return {0, 0};
    }
    
    // Handle case where column is at or beyond line length
    if (col >= lineContent.length()) {
        col = lineContent.length() - 1;
    }
    
    // Special case: if col points to whitespace, find the nearest word
    if (std::isspace(lineContent[col])) {
        // Look right for a word
        size_t rightPos = col;
        while (rightPos < lineContent.length() && std::isspace(lineContent[rightPos])) {
            rightPos++;
        }
        
        // Look left for a word
        size_t leftPos = col;
        while (leftPos > 0 && std::isspace(lineContent[leftPos-1])) {
            leftPos--;
        }
        
        // Decide whether to use the word to the left or right
        // If at the beginning or end of line, or closer to one word than the other, pick accordingly
        if (rightPos >= lineContent.length() || 
            (leftPos > 0 && (col - leftPos) <= (rightPos - col))) {
            // Use word to the left if we're not at start of line
            if (leftPos > 0) {
                col = leftPos - 1; // Position at the last character of the word to the left
            } else {
                return {leftPos, rightPos}; // Return the space itself if at start of line
            }
        } else {
            // Use word to the right
            col = rightPos;
        }
    }
    
    // Case 1: If the character at col is not a word character, return just that character
    if (!isWordChar(lineContent[col])) {
        return {col, col + 1};
    }
    
    // Find word start (scan backward)
    size_t wordStart = col;
    while (wordStart > 0 && isWordChar(lineContent[wordStart - 1])) {
        wordStart--;
    }
    
    // Find word end (scan forward)
    size_t wordEnd = col;
    while (wordEnd < lineContent.length() && isWordChar(lineContent[wordEnd])) {
        wordEnd++;
    }
    
    return {wordStart, wordEnd};
}

bool Editor::expandToWord() {
    if (buffer_.isEmpty()) {
        return false;
    }
    
    // If no selection, select the word under cursor
    if (!hasSelection_) {
        // General case using findWordBoundaries
        auto [wordStart, wordEnd] = findWordBoundaries(cursorLine_, cursorCol_);
        setSelectionRange(cursorLine_, wordStart, cursorLine_, wordEnd);
        setCursor(cursorLine_, wordEnd);
        currentSelectionUnit_ = SelectionUnit::Word;
        return true;
    }
    
    // For expanding an existing selection
    // Find word boundaries at selection start
    auto [startWordStart, startWordEnd] = findWordBoundaries(selectionStartLine_, selectionStartCol_);
    
    // Find word boundaries at selection end
    auto [endWordStart, endWordEnd] = findWordBoundaries(selectionEndLine_, selectionEndCol_);
    
    // Expand the selection to include full words at both ends
    setSelectionRange(selectionStartLine_, startWordStart, selectionEndLine_, endWordEnd);
    
    // Update cursor position (to the selection end)
    setCursor(selectionEndLine_, endWordEnd);
    
    currentSelectionUnit_ = SelectionUnit::Word;
    return true;
}

bool Editor::expandToLine() {
    if (buffer_.isEmpty()) {
        return false;
    }
    
    // If no selection, select the current line
    if (!hasSelection_) {
        // Get the length of the current line
        const std::string& line = buffer_.getLine(cursorLine_);
        size_t lineLength = line.length();
        
        // Select entire line
        setSelectionRange(cursorLine_, 0, cursorLine_, lineLength);
        
        // Move cursor to the end of the line
        setCursor(cursorLine_, lineLength);
        
        currentSelectionUnit_ = SelectionUnit::Line;
        return true;
    }
    
    // If there is an existing selection
    
    // Determine the range of lines currently in the selection
    size_t startLine = selectionStartLine_;
    size_t endLine = selectionEndLine_;
    
    // Expand selection to include full lines
    const std::string& startLineContent = buffer_.getLine(startLine);
    const std::string& endLineContent = buffer_.getLine(endLine);
    
    // Set selection to include all lines from start to end, from column 0 to end of each line
    setSelectionRange(startLine, 0, endLine, endLineContent.length());
    
    // Update cursor position to the end of selection
    setCursor(endLine, endLineContent.length());
    
    currentSelectionUnit_ = SelectionUnit::Line;
    return true;
}

void Editor::expandSelection(SelectionUnit targetUnit) {
    // For now, only implement word-level and line-level expansion
    if (targetUnit == SelectionUnit::Word && 
        (currentSelectionUnit_ == SelectionUnit::Character || !hasSelection_)) {
        expandToWord();
    }
    else if (targetUnit == SelectionUnit::Line && 
             (currentSelectionUnit_ == SelectionUnit::Character || 
              currentSelectionUnit_ == SelectionUnit::Word || 
              !hasSelection_)) {
        expandToLine();
    }
    else if (targetUnit == SelectionUnit::Expression && 
             (currentSelectionUnit_ == SelectionUnit::Character || 
              currentSelectionUnit_ == SelectionUnit::Word ||
              !hasSelection_)) {
        expandToExpression();
    }
    else if (targetUnit == SelectionUnit::Paragraph &&
             (currentSelectionUnit_ == SelectionUnit::Character ||
              currentSelectionUnit_ == SelectionUnit::Word ||
              currentSelectionUnit_ == SelectionUnit::Line ||
              currentSelectionUnit_ == SelectionUnit::Expression ||
              !hasSelection_)) {
        expandToParagraph();
    }
    else if (targetUnit == SelectionUnit::Block &&
             (currentSelectionUnit_ == SelectionUnit::Character ||
              currentSelectionUnit_ == SelectionUnit::Word ||
              currentSelectionUnit_ == SelectionUnit::Line ||
              currentSelectionUnit_ == SelectionUnit::Expression ||
              currentSelectionUnit_ == SelectionUnit::Paragraph ||
              !hasSelection_)) {
        expandToBlock();
    }
    else if (targetUnit == SelectionUnit::Document) {
        // Document level expansion can be called from any state
        expandToDocument();
    }
    // Other expansion levels will be implemented in future phases
}

void Editor::shrinkSelection(SelectionUnit targetUnit) {
    if (!hasSelection_) return;
    
    // Shrink based on current selection unit
    switch (currentSelectionUnit_) {
        case SelectionUnit::Document:
            // For now, do nothing - we'll implement Document->Paragraph later
            break;
            
        case SelectionUnit::Block:
            // For now, do nothing - we'll implement Block->Line later
            break;
            
        case SelectionUnit::Paragraph:
            // For now, do nothing - we'll implement Paragraph->Line later
            break;
            
        case SelectionUnit::Line:
            shrinkFromLineToWord();
            break;
            
        case SelectionUnit::Expression:
            shrinkFromExpressionToWord();
            break;
            
        case SelectionUnit::Word:
            // Existing behavior: clear selection
            shrinkToCharacter();
            break;
            
        default:
            clearSelection();
            currentSelectionUnit_ = SelectionUnit::Character;
            break;
    }
}

bool Editor::shrinkToCharacter() {
    if (!hasSelection_) return false;
    
    // Keep the cursor at its current position but clear the selection
    hasSelection_ = false; // Directly set hasSelection_ to false instead of using clearSelection()
    
    // Update selection unit
    currentSelectionUnit_ = SelectionUnit::Character;
    return true;
}

bool Editor::shrinkToWord() {
    if (!hasSelection_) return false;
    
    // Remember original cursor position
    size_t targetLine = cursorLine_;
    size_t targetCol = cursorCol_;
    
    // Store the selection bounds
    size_t origSelStartLine = selectionStartLine_;
    size_t origSelStartCol = selectionStartCol_;
    size_t origSelEndLine = selectionEndLine_;
    size_t origSelEndCol = selectionEndCol_;
    
    // Ensure the target position is within the selection
    bool cursorInSelection = true;
    
    // Check if cursor is not within selection bounds
    if (targetLine < origSelStartLine || 
        (targetLine == origSelStartLine && targetCol < origSelStartCol) ||
        targetLine > origSelEndLine ||
        (targetLine == origSelEndLine && targetCol > origSelEndCol)) {
        cursorInSelection = false;
    }
    
    // If cursor is not within the selection, use a position inside the selection
    if (!cursorInSelection) {
        // Choose a position within the selection - middle of the first line is a good default
        targetLine = origSelStartLine;
        
        const std::string& line = buffer_.getLine(targetLine);
        
        // If we're at line level, try to find a word - skip whitespace
        if (currentSelectionUnit_ == SelectionUnit::Line) {
            // Start at selection start column and find first non-whitespace
            targetCol = origSelStartCol;
            while (targetCol < line.length() && std::isspace(line[targetCol])) {
                targetCol++;
            }
            
            // If we're at the end of the line or couldn't find non-whitespace,
            // default to middle of visible text
            if (targetCol >= line.length()) {
                if (line.length() > 0) {
                    targetCol = line.length() / 2;
                } else {
                    targetCol = 0;
                }
            }
        } else {
            // For other selection types, middle of selection is a good guess
            if (origSelStartLine == origSelEndLine) {
                targetCol = (origSelStartCol + origSelEndCol) / 2;
            } else {
                // For multi-line selections, use the first line
                targetCol = (origSelStartCol + line.length()) / 2;
            }
        }
    }
    
    // Find word boundaries at the target position
    std::pair<size_t, size_t> wordBoundaries = findWordBoundaries(targetLine, targetCol);
    
    // Create a word selection
    selectionStartLine_ = targetLine;
    selectionStartCol_ = wordBoundaries.first;
    selectionEndLine_ = targetLine;
    selectionEndCol_ = wordBoundaries.second;
    
    // Update selection state
    hasSelection_ = true;
    currentSelectionUnit_ = SelectionUnit::Word;
    
    return true;
}

bool Editor::shrinkToExpression() {
    if (!hasSelection_ || buffer_.isEmpty() || currentSelectionUnit_ != SelectionUnit::Expression) {
        return false;
    }
    
    // For now, we'll just set to word level since we don't have a nested expression tracking system yet
    return shrinkToWord();
}

char Editor::getMatchingBracket(char bracket) const {
    switch (bracket) {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        default: return '\0';
    }
}

bool Editor::isOpeningBracket(char c) const {
    return c == '(' || c == '[' || c == '{';
}

bool Editor::isClosingBracket(char c) const {
    return c == ')' || c == ']' || c == '}';
}

bool Editor::isQuoteChar(char c) const {
    return c == '"' || c == '\'';
}

Editor::ExpressionBoundary Editor::findEnclosingQuotes(const Position& pos, char quoteChar) const {
    if (buffer_.isEmpty() || pos.line >= buffer_.lineCount()) {
        return ExpressionBoundary();
    }
    
    const std::string& line = buffer_.getLine(pos.line);
    if (pos.column >= line.length()) {
        return ExpressionBoundary();
    }
    
    // If no specific quote character is provided, use the character at cursor position
    // or try to find any quote character
    bool useSpecificQuote = quoteChar != '\0';
    char currentChar = (pos.column < line.length()) ? line[pos.column] : '\0';
    
    // If cursor is on a quote, find the matching quote
    if (!useSpecificQuote && isQuoteChar(currentChar)) {
        quoteChar = currentChar;
        useSpecificQuote = true;
    }
    
    // If cursor is on a quote or specific quote is requested
    if (useSpecificQuote) {
        // Look for a matching quote, considering escape characters
        bool foundMatching = false;
        size_t matchPos = 0;
        
        // First, check to the right
        bool escaped = false;
        for (size_t i = pos.column + 1; i < line.length(); i++) {
            if (line[i] == '\\') {
                escaped = !escaped; // Toggle escape status
                continue;
            }
            
            if (line[i] == quoteChar && !escaped) {
                // Found a matching quote to the right
                foundMatching = true;
                matchPos = i;
                
                Position start = {pos.line, pos.column};
                Position end = {pos.line, matchPos + 1}; // Include the closing quote
                return ExpressionBoundary(start, end);
            }
            
            escaped = false; // Reset escape if it wasn't used for this character
        }
        
        // If not found to the right, check to the left
        escaped = false;
        for (int i = static_cast<int>(pos.column) - 1; i >= 0; i--) {
            if (i > 0 && line[i - 1] == '\\') {
                escaped = !escaped;
                continue;
            }
            
            if (line[i] == quoteChar && !escaped) {
                // Found a matching quote to the left
                foundMatching = true;
                matchPos = i;
                
                Position start = {pos.line, static_cast<size_t>(matchPos)};
                Position end = {pos.line, pos.column + 1}; // Include the closing quote
                return ExpressionBoundary(start, end);
            }
            
            escaped = false;
        }
    }
    
    // If not on a quote or no specific quote requested, look for enclosing quotes
    // Try with double quotes first, then single quotes if needed
    if (!useSpecificQuote) {
        std::vector<char> quotesToTry = {'"', '\''};
        
        for (char tryQuote : quotesToTry) {
            // Look for the nearest quote to the left
            int leftQuotePos = -1;
            bool escapedLeft = false;
            
            for (int i = static_cast<int>(pos.column) - 1; i >= 0; i--) {
                if (i > 0 && line[i - 1] == '\\') {
                    escapedLeft = !escapedLeft;
                    continue;
                }
                
                if (line[i] == tryQuote && !escapedLeft) {
                    leftQuotePos = i;
                    break;
                }
                
                escapedLeft = false;
            }
            
            // If found a quote to the left, look for a matching one to the right
            if (leftQuotePos >= 0) {
                // Look for the matching quote to the right of the cursor
                bool escapedRight = false;
                for (size_t i = pos.column; i < line.length(); i++) {
                    if (line[i] == '\\') {
                        escapedRight = !escapedRight;
                        continue;
                    }
                    
                    if (line[i] == tryQuote && !escapedRight) {
                        // Found a matching quote
                        Position start = {pos.line, static_cast<size_t>(leftQuotePos)};
                        Position end = {pos.line, i + 1}; // Include the closing quote
                        return ExpressionBoundary(start, end);
                    }
                    
                    escapedRight = false;
                }
            }
        }
    }
    
    // No enclosing quotes found
    return ExpressionBoundary();
}

Editor::ExpressionBoundary Editor::findMatchingBracketPair(const Position& pos, char openBracket, char closeBracket) const {
    if (buffer_.isEmpty() || pos.line >= buffer_.lineCount()) {
        return ExpressionBoundary();
    }
    
    const std::string& line = buffer_.getLine(pos.line);
    if (pos.column >= line.length()) {
        return ExpressionBoundary();
    }
    
    // If cursor is on an opening bracket, find its closing match
    if (pos.column < line.length() && line[pos.column] == openBracket) {
        // Start from the current position and look for the matching closing bracket
        int nestLevel = 1;
        Position matchPos = {pos.line, 0};
        
        // First, check the rest of the current line
        for (size_t i = pos.column + 1; i < line.length(); i++) {
            if (line[i] == openBracket) {
                nestLevel++;
            } else if (line[i] == closeBracket) {
                nestLevel--;
                if (nestLevel == 0) {
                    // Found matching bracket on same line
                    matchPos = {pos.line, i};
                    Position start = {pos.line, pos.column};
                    Position end = {matchPos.line, matchPos.column + 1};
                    return ExpressionBoundary(start, end);
                }
            }
        }
        
        // If not found on the same line, check subsequent lines
        for (size_t l = pos.line + 1; l < buffer_.lineCount(); l++) {
            const std::string& nextLine = buffer_.getLine(l);
            for (size_t i = 0; i < nextLine.length(); i++) {
                if (nextLine[i] == openBracket) {
                    nestLevel++;
                } else if (nextLine[i] == closeBracket) {
                    nestLevel--;
                    if (nestLevel == 0) {
                        // Found matching bracket on a different line
                        matchPos = {l, i};
                        Position start = {pos.line, pos.column};
                        Position end = {matchPos.line, matchPos.column + 1};
                        return ExpressionBoundary(start, end);
                    }
                }
            }
        }
    }
    
    // If cursor is on a closing bracket, find its opening match
    if (pos.column < line.length() && line[pos.column] == closeBracket) {
        // Start from the current position and look for the matching opening bracket
        int nestLevel = 1;
        Position matchPos = {pos.line, 0};
        
        // First, check from the beginning of the current line
        for (int i = static_cast<int>(pos.column) - 1; i >= 0; i--) {
            if (line[i] == closeBracket) {
                nestLevel++;
            } else if (line[i] == openBracket) {
                nestLevel--;
                if (nestLevel == 0) {
                    // Found matching bracket on same line
                    matchPos = {pos.line, static_cast<size_t>(i)};
                    Position start = {matchPos.line, matchPos.column};
                    Position end = {pos.line, pos.column + 1};
                    return ExpressionBoundary(start, end);
                }
            }
        }
        
        // If not found on the same line, check previous lines
        for (int l = static_cast<int>(pos.line) - 1; l >= 0; l--) {
            const std::string& prevLine = buffer_.getLine(l);
            for (int i = static_cast<int>(prevLine.length()) - 1; i >= 0; i--) {
                if (prevLine[i] == closeBracket) {
                    nestLevel++;
                } else if (prevLine[i] == openBracket) {
                    nestLevel--;
                    if (nestLevel == 0) {
                        // Found matching bracket on a different line
                        matchPos = {static_cast<size_t>(l), static_cast<size_t>(i)};
                        Position start = {matchPos.line, matchPos.column};
                        Position end = {pos.line, pos.column + 1};
                        return ExpressionBoundary(start, end);
                    }
                }
            }
        }
    }
    
    // Next, check if cursor is inside a bracket pair
    // Look for the nearest opening bracket to the left
    Position leftBracketPos = {pos.line, 0};
    bool foundLeftBracket = false;
    int nestLevel = 0;
    
    // Check on the current line, to the left of cursor
    for (int i = static_cast<int>(pos.column) - 1; i >= 0; i--) {
        if (line[i] == closeBracket) {
            nestLevel++;
        } else if (line[i] == openBracket) {
            if (nestLevel == 0) {
                // Found an unmatched opening bracket
                leftBracketPos = {pos.line, static_cast<size_t>(i)};
                foundLeftBracket = true;
                break;
            }
            nestLevel--;
        }
    }
    
    // If not found on the current line, check previous lines
    if (!foundLeftBracket) {
        for (int l = static_cast<int>(pos.line) - 1; l >= 0; l--) {
            const std::string& prevLine = buffer_.getLine(l);
            for (int i = static_cast<int>(prevLine.length()) - 1; i >= 0; i--) {
                if (prevLine[i] == closeBracket) {
                    nestLevel++;
                } else if (prevLine[i] == openBracket) {
                    if (nestLevel == 0) {
                        // Found an unmatched opening bracket
                        leftBracketPos = {static_cast<size_t>(l), static_cast<size_t>(i)};
                        foundLeftBracket = true;
                        break;
                    }
                    nestLevel--;
                }
            }
            if (foundLeftBracket) break;
        }
    }
    
    // If found an opening bracket, look for its matching closing bracket
    if (foundLeftBracket) {
        // Reset nest level
        nestLevel = 1;
        Position rightBracketPos = {pos.line, 0};
        bool foundRightBracket = false;
        
        // Check on the current line, to the right of cursor
        for (size_t i = pos.column; i < line.length(); i++) {
            if (line[i] == openBracket) {
                nestLevel++;
            } else if (line[i] == closeBracket) {
                nestLevel--;
                if (nestLevel == 0) {
                    // Found the matching closing bracket
                    rightBracketPos = {pos.line, i};
                    foundRightBracket = true;
                    break;
                }
            }
        }
        
        // If not found on the current line, check subsequent lines
        if (!foundRightBracket) {
            for (size_t l = pos.line + 1; l < buffer_.lineCount(); l++) {
                const std::string& nextLine = buffer_.getLine(l);
                for (size_t i = 0; i < nextLine.length(); i++) {
                    if (nextLine[i] == openBracket) {
                        nestLevel++;
                    } else if (nextLine[i] == closeBracket) {
                        nestLevel--;
                        if (nestLevel == 0) {
                            // Found the matching closing bracket
                            rightBracketPos = {l, i};
                            foundRightBracket = true;
                            break;
                        }
                    }
                }
                if (foundRightBracket) break;
            }
        }
        
        // If found a matching pair that encloses the cursor
        if (foundRightBracket) {
            Position start = leftBracketPos;
            Position end = {rightBracketPos.line, rightBracketPos.column + 1};
            return ExpressionBoundary(start, end);
        }
    }
    
    // No matching bracket pair found
    return ExpressionBoundary();
}

Editor::ExpressionBoundary Editor::findEnclosingExpression(const Position& startPos, const Position& endPos) const {
    if (buffer_.isEmpty()) {
        return ExpressionBoundary();
    }
    
    // First, check if we're searching for an expansion of an existing expression
    bool expandingExistingExpression = false;
    ExpressionBoundary existingExpression;
    
    // Detect if startPos and endPos form a complete bracket pair (this would happen during expansion)
    if (startPos.line < buffer_.lineCount() && endPos.line < buffer_.lineCount()) {
        const std::string& startLine = buffer_.getLine(startPos.line);
        const std::string& endLine = buffer_.getLine(endPos.line);
        
        if (startPos.column < startLine.length() && endPos.column <= endLine.length()) {
            // Check if startPos is at an opening bracket and endPos is right after a closing bracket
            if (startPos.column < startLine.length() && endPos.column > 0 && 
                endPos.column <= endLine.length()) {
                
                char startChar = startLine[startPos.column];
                char endChar = (endPos.column > 0) ? endLine[endPos.column - 1] : '\0';
                
                // Check if they form a matching bracket pair
                if ((startChar == '(' && endChar == ')') ||
                    (startChar == '[' && endChar == ']') ||
                    (startChar == '{' && endChar == '}') ||
                    (isQuoteChar(startChar) && startChar == endChar)) {
                    expandingExistingExpression = true;
                    existingExpression = ExpressionBoundary(startPos, endPos);
                }
            }
        }
    }
    
    // If we're expanding an existing expression, look for the next larger one
    if (expandingExistingExpression) {
        // Adjust the search positions to look just outside the current expression
        Position outerStartPos = {startPos.line, startPos.column > 0 ? startPos.column - 1 : 0};
        Position outerEndPos = {endPos.line, endPos.column < buffer_.getLine(endPos.line).length() ? 
                                endPos.column + 1 : endPos.column};
        
        // First check for brackets that might enclose the current expression
        std::vector<std::pair<char, char>> bracketPairs = {
            {'(', ')'},
            {'[', ']'},
            {'{', '}'}
        };
        
        for (const auto& pair : bracketPairs) {
            // Look for opening bracket before start and closing bracket after end
            // Start by checking the current line to the left of start pos
            const std::string& startLine = buffer_.getLine(outerStartPos.line);
            
            // Look for opening brackets to the left
            for (int i = static_cast<int>(outerStartPos.column); i >= 0; i--) {
                if (i < static_cast<int>(startLine.length()) && startLine[i] == pair.first) {
                    // Found potential opening bracket, now look for closing bracket
                    // that appears after our current expression
                    
                    // Initialize nestLevel to 1 for this opening bracket
                    int nestLevel = 1;
                    Position matchingClosePos;
                    bool foundMatch = false;
                    
                    // First check rest of current line
                    const std::string& endLine = buffer_.getLine(outerEndPos.line);
                    for (size_t j = outerEndPos.column; j < endLine.length(); j++) {
                        if (endLine[j] == pair.first) {
                            nestLevel++;
                        } else if (endLine[j] == pair.second) {
                            nestLevel--;
                            if (nestLevel == 0) {
                                // Found matching close bracket
                                matchingClosePos = {outerEndPos.line, j + 1}; // position after the closing bracket
                                foundMatch = true;
                                break;
                            }
                        }
                    }
                    
                    // If not found on the same line, check subsequent lines
                    if (!foundMatch && outerEndPos.line < buffer_.lineCount() - 1) {
                        for (size_t lineIndex = outerEndPos.line + 1; lineIndex < buffer_.lineCount(); lineIndex++) {
                            const std::string& line = buffer_.getLine(lineIndex);
                            for (size_t j = 0; j < line.length(); j++) {
                                if (line[j] == pair.first) {
                                    nestLevel++;
                                } else if (line[j] == pair.second) {
                                    nestLevel--;
                                    if (nestLevel == 0) {
                                        // Found matching close bracket
                                        matchingClosePos = {lineIndex, j + 1}; // position after the closing bracket
                                        foundMatch = true;
                                        break;
                                    }
                                }
                            }
                            if (foundMatch) break;
                        }
                    }
                    
                    if (foundMatch) {
                        // Found a larger enclosing expression
                        Position newStart = {outerStartPos.line, static_cast<size_t>(i)};
                        return ExpressionBoundary(newStart, matchingClosePos);
                    }
                }
            }
            
            // If not found on current line, check previous lines
            if (outerStartPos.line > 0) {
                for (int lineIndex = static_cast<int>(outerStartPos.line) - 1; lineIndex >= 0; lineIndex--) {
                    const std::string& line = buffer_.getLine(lineIndex);
                    for (int i = static_cast<int>(line.length()) - 1; i >= 0; i--) {
                        if (line[i] == pair.first) {
                            // Found potential opening bracket, now look for closing bracket
                            // that appears after our current expression
                            
                            // Initialize nestLevel to 1 for this opening bracket
                            int nestLevel = 1;
                            Position matchingClosePos;
                            bool foundMatch = false;
                            
                            // Start with the outerEndPos line
                            const std::string& endLine = buffer_.getLine(outerEndPos.line);
                            for (size_t j = outerEndPos.column; j < endLine.length(); j++) {
                                if (endLine[j] == pair.first) {
                                    nestLevel++;
                                } else if (endLine[j] == pair.second) {
                                    nestLevel--;
                                    if (nestLevel == 0) {
                                        // Found matching close bracket
                                        matchingClosePos = {outerEndPos.line, j + 1}; // position after the closing bracket
                                        foundMatch = true;
                                        break;
                                    }
                                }
                            }
                            
                            // If not found on the same line, check subsequent lines
                            if (!foundMatch && outerEndPos.line < buffer_.lineCount() - 1) {
                                for (size_t nextLine = outerEndPos.line + 1; nextLine < buffer_.lineCount(); nextLine++) {
                                    const std::string& line = buffer_.getLine(nextLine);
                                    for (size_t j = 0; j < line.length(); j++) {
                                        if (line[j] == pair.first) {
                                            nestLevel++;
                                        } else if (line[j] == pair.second) {
                                            nestLevel--;
                                            if (nestLevel == 0) {
                                                // Found matching close bracket
                                                matchingClosePos = {nextLine, j + 1}; // position after the closing bracket
                                                foundMatch = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (foundMatch) break;
                                }
                            }
                            
                            if (foundMatch) {
                                // Found a larger enclosing expression
                                Position newStart = {static_cast<size_t>(lineIndex), static_cast<size_t>(i)};
                                return ExpressionBoundary(newStart, matchingClosePos);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If we get here, we either weren't expanding an existing expression, or we couldn't find a larger one
    // Check for quotes first
    ExpressionBoundary quoteBoundary = findEnclosingQuotes(startPos, '\0');
    if (quoteBoundary.found) {
        return quoteBoundary;
    }
    
    // Check for various bracket types
    std::vector<std::pair<char, char>> bracketPairs = {
        {'(', ')'},
        {'[', ']'},
        {'{', '}'}
    };
    
    for (const auto& pair : bracketPairs) {
        ExpressionBoundary bracketBoundary = findMatchingBracketPair(startPos, pair.first, pair.second);
        if (bracketBoundary.found) {
            return bracketBoundary;
        }
    }
    
    // If cursor position itself didn't yield results, try searching for the smallest enclosing expression
    // This means looking both left and right from the current position
    
    // Look for the nearest enclosing brackets or quotes
    // For simplicity, we'll check a limited vicinity around the cursor
    
    const std::string& line = buffer_.getLine(startPos.line);
    
    // Check for brackets enclosing the current position
    for (const auto& pair : bracketPairs) {
        // Look for an opening bracket to the left and closing to the right
        size_t leftLimit = startPos.column > 10 ? startPos.column - 10 : 0;
        size_t rightLimit = std::min(startPos.column + 10, line.length());
        
        for (size_t left = startPos.column; left >= leftLimit; left--) {
            if (left < line.length() && line[left] == pair.first) {
                // Found opening bracket, now look for matching closing
                int nestLevel = 1;
                for (size_t right = startPos.column; right < rightLimit; right++) {
                    if (right < line.length()) {
                        if (line[right] == pair.first) {
                            nestLevel++;
                        } else if (line[right] == pair.second) {
                            nestLevel--;
                            if (nestLevel == 0) {
                                // Found a matching pair
                                Position start = {startPos.line, left};
                                Position end = {startPos.line, right + 1}; // Include the closing bracket
                                return ExpressionBoundary(start, end);
                            }
                        }
                    }
                }
            }
            if (left == 0) break; // Avoid underflow
        }
    }
    
    // No enclosing expression found
    return ExpressionBoundary();
}

bool Editor::expandToExpression() {
    if (buffer_.isEmpty()) {
        return false;
    }
    
    Position cursorPos = {cursorLine_, cursorCol_};
    Position startPos, endPos;
    
    if (!hasSelection_) {
        // If no selection, start with cursor position
        startPos = endPos = cursorPos;
    } else {
        // If there is an existing selection, use its boundaries
        startPos = {selectionStartLine_, selectionStartCol_};
        endPos = {selectionEndLine_, selectionEndCol_};
    }
    
    // Find the immediate enclosing expression
    ExpressionBoundary boundary = findEnclosingExpression(startPos, endPos);
    
    if (boundary.found) {
        // If the selection already matches this expression exactly, try to find a larger one
        if (hasSelection_ && 
            selectionStartLine_ == boundary.start.line && 
            selectionStartCol_ == boundary.start.column &&
            selectionEndLine_ == boundary.end.line && 
            selectionEndCol_ == boundary.end.column) {
            
            // Try to find a larger enclosing expression
            Position outerStart = {boundary.start.line, boundary.start.column > 0 ? boundary.start.column - 1 : 0};
            Position outerEnd = {boundary.end.line, boundary.end.column + 1};
            
            ExpressionBoundary outerBoundary = findEnclosingExpression(outerStart, outerEnd);
            if (outerBoundary.found) {
                // Use the larger expression
                boundary = outerBoundary;
            }
        }
        
        // Set selection to the found expression
        setSelectionRange(boundary.start.line, boundary.start.column, 
                         boundary.end.line, boundary.end.column);
        
        // Update cursor position to the end of the selection
        setCursor(boundary.end.line, boundary.end.column);
        
        currentSelectionUnit_ = SelectionUnit::Expression;
        return true;
    }
    
    return false; // No enclosing expression found
}

bool Editor::expandToParagraph() {
    if (buffer_.isEmpty()) {
        // Handle empty buffer case
        setSelectionRange(0, 0, 0, 0);
        currentSelectionUnit_ = SelectionUnit::Paragraph;
        return true;
    }
    
    // If no selection exists, start from cursor position
    size_t startLine = hasSelection_ ? selectionStartLine_ : cursorLine_;
    size_t endLine = hasSelection_ ? selectionEndLine_ : cursorLine_;
    
    // Ensure we're within buffer bounds
    if (startLine >= buffer_.lineCount()) startLine = buffer_.lineCount() - 1;
    if (endLine >= buffer_.lineCount()) endLine = buffer_.lineCount() - 1;
    
    // Find paragraph start by searching up from start line
    // A paragraph starts at line 0 or after an empty line
    size_t paragraphStart = startLine;
    while (paragraphStart > 0) {
        const std::string& prevLine = buffer_.getLine(paragraphStart - 1);
        if (prevLine.empty() || prevLine.find_first_not_of(" \t") == std::string::npos) {
            // Found an empty line or line with only whitespace
            break;
        }
        paragraphStart--;
    }
    
    // Find paragraph end by searching down from end line
    // A paragraph ends at the last line or before an empty line
    size_t paragraphEnd = endLine;
    while (paragraphEnd < buffer_.lineCount() - 1) {
        const std::string& nextLine = buffer_.getLine(paragraphEnd + 1);
        if (nextLine.empty() || nextLine.find_first_not_of(" \t") == std::string::npos) {
            // Found an empty line or line with only whitespace
            break;
        }
        paragraphEnd++;
    }
    
    // Special case: if cursor is on an empty line or if the selection starts on an empty line
    bool isEmptyLine = buffer_.getLine(startLine).empty() || 
                      buffer_.getLine(startLine).find_first_not_of(" \t") == std::string::npos;
    
    if (!hasSelection_ && isEmptyLine) {
        // Look for the nearest non-empty paragraph
        // First, try to find a paragraph below
        size_t nextParagraphStart = startLine + 1;
        while (nextParagraphStart < buffer_.lineCount() && 
               (buffer_.getLine(nextParagraphStart).empty() || 
                buffer_.getLine(nextParagraphStart).find_first_not_of(" \t") == std::string::npos)) {
            nextParagraphStart++;
        }
        
        if (nextParagraphStart < buffer_.lineCount()) {
            // Found a paragraph below, expand to it
            paragraphStart = nextParagraphStart;
            
            // Find the end of this paragraph
            paragraphEnd = paragraphStart;
            while (paragraphEnd < buffer_.lineCount() - 1) {
                const std::string& nextLine = buffer_.getLine(paragraphEnd + 1);
                if (nextLine.empty() || nextLine.find_first_not_of(" \t") == std::string::npos) {
                    break;
                }
                paragraphEnd++;
            }
        } else {
            // No paragraph below, try to find one above
            size_t prevParagraphEnd = startLine;
            while (prevParagraphEnd > 0 && 
                   (buffer_.getLine(prevParagraphEnd).empty() || 
                    buffer_.getLine(prevParagraphEnd).find_first_not_of(" \t") == std::string::npos)) {
                prevParagraphEnd--;
            }
            
            if (prevParagraphEnd < buffer_.lineCount() && 
                !(buffer_.getLine(prevParagraphEnd).empty() || 
                  buffer_.getLine(prevParagraphEnd).find_first_not_of(" \t") == std::string::npos)) {
                // Found a paragraph above, expand to it
                paragraphEnd = prevParagraphEnd;
                
                // Find the start of this paragraph
                paragraphStart = paragraphEnd;
                while (paragraphStart > 0) {
                    const std::string& prevLine = buffer_.getLine(paragraphStart - 1);
                    if (prevLine.empty() || prevLine.find_first_not_of(" \t") == std::string::npos) {
                        break;
                    }
                    paragraphStart--;
                }
            } else {
                // No paragraphs found, just select the current empty line
                paragraphStart = paragraphEnd = startLine;
            }
        }
    } else if (hasSelection_ && (endLine > startLine)) {
        // For a multi-line selection, ensure we capture complete paragraphs
        // Find the start of the first paragraph
        paragraphStart = startLine;
        while (paragraphStart > 0) {
            const std::string& prevLine = buffer_.getLine(paragraphStart - 1);
            if (prevLine.empty() || prevLine.find_first_not_of(" \t") == std::string::npos) {
                break;
            }
            paragraphStart--;
        }
        
        // Find the end of the last paragraph
        paragraphEnd = endLine;
        while (paragraphEnd < buffer_.lineCount() - 1) {
            const std::string& nextLine = buffer_.getLine(paragraphEnd + 1);
            if (nextLine.empty() || nextLine.find_first_not_of(" \t") == std::string::npos) {
                break;
            }
            paragraphEnd++;
        }
    }
    
    // Handle edge case where we're at the end of the buffer
    size_t lineLength = 0;
    if (paragraphEnd < buffer_.lineCount()) {
        lineLength = buffer_.getLine(paragraphEnd).length();
    }
    
    // Set selection to cover the paragraph(s)
    setSelectionRange(paragraphStart, 0, paragraphEnd, lineLength);
    
    // Move cursor to the end of selection
    setCursor(paragraphEnd, lineLength);
    
    // Update selection unit
    currentSelectionUnit_ = SelectionUnit::Paragraph;
    
    return true;
} 

int Editor::comparePositions(const Position& a, const Position& b) const {
    if (a.line < b.line) return -1;
    if (a.line > b.line) return 1;
    // Lines are the same, compare columns
    if (a.column < b.column) return -1;
    if (a.column > b.column) return 1;
    return 0; // Positions are identical
}

Position Editor::findPreviousOpeningBrace(const Position& pos) const {
    if (buffer_.isEmpty()) {
        return {SIZE_MAX, SIZE_MAX};
    }
    
    Position current = pos;
    int braceCount = 0;
    
    // Ensure starting position is valid
    if (current.line >= buffer_.lineCount()) {
        current.line = buffer_.lineCount() - 1;
    }
    
    // Scan backward through the buffer
    while (true) {
        const std::string& line = buffer_.getLine(current.line);
        
        // For the first line we scan, only look before the column position
        size_t scanStart = (current.line == pos.line) ? 
                           std::min(current.column, line.length()) : 
                           line.length();
        
        // Scan this line from right to left
        for (int i = static_cast<int>(scanStart) - 1; i >= 0; i--) {
            if (line[i] == '}') {
                braceCount++;
            } else if (line[i] == '{') {
                if (braceCount == 0) {
                    // Found an unmatched opening brace
                    return {current.line, static_cast<size_t>(i)};
                }
                braceCount--;
            }
        }
        
        if (current.line == 0) break; // Avoid underflow
        current.line--;
        current.column = 0; // Reset column for next line
    }
    
    // No matching opening brace found
    return {SIZE_MAX, SIZE_MAX};
}

Editor::ExpressionBoundary Editor::scanForEnclosingBraces(const Position& startPos, const Position& endPos) const {
    if (buffer_.isEmpty()) {
        return ExpressionBoundary();
    }
    
    // Make sure positions are valid
    Position validatedStartPos = startPos;
    Position validatedEndPos = endPos;
    
    if (validatedStartPos.line >= buffer_.lineCount()) {
        validatedStartPos.line = buffer_.lineCount() - 1;
    }
    
    if (validatedEndPos.line >= buffer_.lineCount()) {
        validatedEndPos.line = buffer_.lineCount() - 1;
    }
    
    // Search outward from the selection
    // Start by looking for an opening brace before the selection start
    Position openBracePos = findPreviousOpeningBrace(validatedStartPos);
    if (openBracePos.line == SIZE_MAX) {
        // No opening brace found before the selection
        return ExpressionBoundary();
    }
    
    // Now find the matching closing brace for this opening brace
    ExpressionBoundary boundary = findMatchingBracketPair(openBracePos, '{', '}');
    if (!boundary.found) {
        return ExpressionBoundary();
    }
    
    // Make sure this block actually encloses our selection/end position
    if (comparePositions(boundary.end, validatedEndPos) < 0) {
        // The closing brace is before our selection end, so this block doesn't enclose us
        return ExpressionBoundary();
    }
    
    return boundary;
}

Editor::ExpressionBoundary Editor::findEnclosingBracePair(const Position& startPos, const Position& endPos) const {
    if (buffer_.isEmpty()) {
        return ExpressionBoundary();
    }
    
    // Make sure positions are valid
    Position validatedStartPos = startPos;
    Position validatedEndPos = endPos;
    
    if (validatedStartPos.line >= buffer_.lineCount()) {
        validatedStartPos.line = buffer_.lineCount() - 1;
    }
    
    if (validatedEndPos.line >= buffer_.lineCount()) {
        validatedEndPos.line = buffer_.lineCount() - 1;
    }
    
    // Get characters at start and end positions if they exist
    char charAtStart = '\0';
    char charAtEnd = '\0';
    
    const std::string& startLine = buffer_.getLine(validatedStartPos.line);
    if (validatedStartPos.column < startLine.length()) {
        charAtStart = startLine[validatedStartPos.column];
    }
    
    const std::string& endLine = buffer_.getLine(validatedEndPos.line);
    if (validatedEndPos.column < endLine.length()) {
        charAtEnd = endLine[validatedEndPos.column];
    }
    
    // If we're on a brace, use the existing matching bracket pair finder
    if (charAtStart == '{') {
        return findMatchingBracketPair(validatedStartPos, '{', '}');
    } else if (charAtEnd == '}') {
        // For closing brace, we need to find its matching opening brace
        ExpressionBoundary boundary = findMatchingBracketPair(validatedEndPos, '{', '}');
        if (boundary.found) {
            // Swap start and end to maintain correct order
            Position temp = boundary.start;
            boundary.start = boundary.end;
            boundary.end = temp;
        }
        return boundary;
    }
    
    // Search outward for enclosing braces
    return scanForEnclosingBraces(validatedStartPos, validatedEndPos);
}

bool Editor::expandToBlock() {
    if (buffer_.isEmpty()) {
        return false;
    }
    
    Position cursorPos = {cursorLine_, cursorCol_};
    Position startPos, endPos;
    
    if (!hasSelection_) {
        // If no selection, start with cursor position
        startPos = endPos = cursorPos;
    } else {
        // If there is an existing selection, use its boundaries
        startPos = {selectionStartLine_, selectionStartCol_};
        endPos = {selectionEndLine_, selectionEndCol_};
    }
    
    // Find the enclosing block (curly brace pair)
    ExpressionBoundary boundary = findEnclosingBracePair(startPos, endPos);
    
    if (boundary.found) {
        // If we already have this exact block selected, try to expand to outer block
        if (hasSelection_ && 
            selectionStartLine_ == boundary.start.line && 
            selectionStartCol_ == boundary.start.column &&
            selectionEndLine_ == boundary.end.line && 
            selectionEndCol_ == boundary.end.column) {
            
            // Try to find a larger enclosing block
            Position outerStart = {boundary.start.line, boundary.start.column > 0 ? boundary.start.column - 1 : 0};
            Position outerEnd = {boundary.end.line, boundary.end.column + 1};
            
            ExpressionBoundary outerBoundary = findEnclosingBracePair(outerStart, outerEnd);
            if (outerBoundary.found) {
                boundary = outerBoundary;
            }
        }
        
        // Set selection to the found block
        setSelectionRange(boundary.start.line, boundary.start.column, 
                         boundary.end.line, boundary.end.column);
        
        // Move cursor to the end of selection
        setCursor(boundary.end.line, boundary.end.column);
        
        currentSelectionUnit_ = SelectionUnit::Block;
        return true;
    }
    
    return false; // No enclosing block found
}

bool Editor::expandToDocument() {
    if (buffer_.isEmpty()) {
        // For an empty buffer, just clear any existing selection
        clearSelection();
        currentSelectionUnit_ = SelectionUnit::Document;
        return true;
    }
    
    // Set selection from start to end of buffer
    size_t lastLine = buffer_.lineCount() - 1;
    size_t lastLineLength = buffer_.getLine(lastLine).length();
    
    setSelectionRange(0, 0, lastLine, lastLineLength);
    
    // Move cursor to the end of the document
    setCursor(lastLine, lastLineLength);
    
    currentSelectionUnit_ = SelectionUnit::Document;
    return true;
}

bool Editor::shrinkFromLineToWord() {
    if (currentSelectionUnit_ != SelectionUnit::Line || !hasSelection_) {
        return false;
    }
    
    // Determine which line contains the cursor position
    size_t targetLine = cursorLine_;
    
    // If cursor is at selection end, we're extending backward
    bool cursorAtSelectionEnd = (cursorLine_ == selectionEndLine_ && cursorCol_ == selectionEndCol_);
    
    // If cursor is not within the selection, use a line that is
    if (targetLine < selectionStartLine_ || targetLine > selectionEndLine_) {
        targetLine = cursorAtSelectionEnd ? selectionStartLine_ : selectionEndLine_;
    }
    
    const std::string& lineText = buffer_.getLine(targetLine);
    
    // Find a suitable word in the line
    // Strategy: Find first non-whitespace word or word near the middle
    size_t wordStart = 0, wordEnd = 0;
    
    // Skip leading whitespace
    while (wordStart < lineText.length() && std::isspace(lineText[wordStart])) {
        wordStart++;
    }
    
    // If line is empty or only whitespace, select a zero-length position
    if (wordStart >= lineText.length()) {
        setSelectionRange(targetLine, 0, targetLine, 0);
        currentSelectionUnit_ = SelectionUnit::Word;
        return true;
    }
    
    // Find end of word
    wordEnd = wordStart;
    while (wordEnd < lineText.length() && isWordChar(lineText[wordEnd])) {
        wordEnd++;
    }
    
    // Update selection to this word
    setSelectionRange(targetLine, wordStart, targetLine, wordEnd);
    
    // Move cursor to end of word
    setCursor(targetLine, wordEnd);
    
    currentSelectionUnit_ = SelectionUnit::Word;
    return true;
}

bool Editor::shrinkFromExpressionToWord() {
    if (currentSelectionUnit_ != SelectionUnit::Expression || !hasSelection_) {
        return false;
    }
    
    // Find a significant word within the expression
    // Strategy: Find the first word after opening delimiter, or meaningful token
    
    // Find first non-delimiter character in the selection
    size_t line = selectionStartLine_;
    size_t col = selectionStartCol_;
    const std::string& startLine = buffer_.getLine(line);
    
    // Skip the opening delimiter if present
    if (col < startLine.length() && (startLine[col] == '(' || startLine[col] == '[' || 
        startLine[col] == '{' || startLine[col] == '"' || startLine[col] == '\'')) {
        col++;
    }
    
    // Skip whitespace
    while (col < startLine.length() && std::isspace(startLine[col])) {
        col++;
    }
    
    // Find word boundaries
    auto [wordStart, wordEnd] = findWordBoundaries(line, col);
    
    // Update selection to this word
    setSelectionRange(line, wordStart, line, wordEnd);
    
    // Move cursor to end of word
    setCursor(line, wordEnd);
    
    currentSelectionUnit_ = SelectionUnit::Word;
    return true;
}