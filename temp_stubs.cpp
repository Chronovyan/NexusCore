// Correct implementations for the three key virtual methods

void Editor::enableSyntaxHighlighting(bool enable) {
    syntaxHighlightingEnabled_ = enable;
    syntaxHighlightingManager_.setEnabled(enable);
    if (enable && !currentHighlighter_ && !filename_.empty()) {
        detectAndSetHighlighter();
    }
    invalidateHighlightingCache();
}

void Editor::detectAndSetHighlighter() {
    // Stub: This would typically analyze filename extension and set appropriate highlighter
    // For now, we'll just reset the highlighter
    currentHighlighter_ = nullptr;
    invalidateHighlightingCache();
}

std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() const {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_ || buffer_.isEmpty()) {
        return {};
    }
    return syntaxHighlightingManager_.getStyles();
}

// Other missing methods

void Editor::setCursor(size_t line, size_t col) {
    cursorLine_ = line;
    cursorCol_ = col;
    validateAndClampCursor();
}

void Editor::validateAndClampCursor() {
    // Ensure cursor is within valid buffer bounds
    if (buffer_.isEmpty()) {
        // If the buffer is empty, add an empty line
        buffer_.addLine("");
    }
    
    // Ensure line is valid
    if (cursorLine_ >= buffer_.lineCount()) {
        cursorLine_ = buffer_.lineCount() - 1;
    }
    
    // Ensure column is valid for the current line
    const std::string& line = buffer_.getLine(cursorLine_);
    if (cursorCol_ > line.length()) {
        cursorCol_ = line.length();
    }
}

int Editor::getTerminalWidth() const {
    // Stub implementation - return a reasonable default
    return 80;
}

int Editor::getTerminalHeight() const {
    // Stub implementation - return a reasonable default
    return 24;
}

void Editor::invalidateHighlightingCache() {
    highlightingStylesCacheValid_ = false;
}

void Editor::updateHighlightingCache() {
    if (syntaxHighlightingEnabled_ && currentHighlighter_ && !buffer_.isEmpty()) {
        // In a real implementation, this would update the highlighting cache
        // For now, just mark it as valid
        highlightingStylesCacheValid_ = true;
    }
}

// Methods for selection management
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

// Direct buffer manipulation methods
void Editor::directDeleteTextRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    // Stub implementation - in a real editor this would delete text in the buffer
    // and update cursor position
    setModified(true);
    invalidateHighlightingCache();
}

void Editor::directInsertText(size_t line, size_t col, const std::string& text, size_t& outEndLine, size_t& outEndCol) {
    // Stub implementation - in a real editor this would insert text at the specified position
    // For now, just set output parameters to indicate insertion at the same position
    outEndLine = line;
    outEndCol = col + text.length(); // Simple case assuming no newlines
    
    setModified(true);
    invalidateHighlightingCache();
}

std::string Editor::getClipboardText() const {
    return clipboard_;
}

void Editor::setClipboardText(const std::string& text) {
    clipboard_ = text;
}

void Editor::setLine(size_t lineIndex, const std::string& text) {
    if (lineIndex < buffer_.lineCount()) {
        buffer_.replaceLine(lineIndex, text);
        setModified(true);
        invalidateHighlightingCache();
    }
}

void Editor::setCursorPosition(const Position& pos) {
    setCursor(pos.line, pos.column);
}

bool Editor::undo() {
    if (commandManager_.canUndo()) {
        commandManager_.undo(*this);
        return true;
    }
    return false;
}

bool Editor::redo() {
    if (commandManager_.canRedo()) {
        commandManager_.redo(*this);
        return true;
    }
    return false;
}

bool Editor::performSearchLogic(const std::string& searchTerm, bool caseSensitive, bool forward,
                                size_t& outFoundLine, size_t& outFoundCol) {
    // Stub implementation - in a real editor this would search the buffer for the given term
    // For now, just return false to indicate no match
    return false;
}

bool Editor::performReplaceLogic(const std::string& searchTerm, const std::string& replacementText, 
                                bool caseSensitive, std::string& outOriginalText, 
                                size_t& outReplacedAtLine, size_t& outReplacedAtCol,
                                size_t& outOriginalEndLine, size_t& outOriginalEndCol) {
    // Stub implementation - in a real editor this would perform replacement
    // For now, just return false to indicate no replacement was done
    return false;
}

// Non-const version of getHighlightingStyles
std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() {
    // For now, just delegate to the const version by casting *this to const
    return static_cast<const Editor*>(this)->getHighlightingStyles();
} 