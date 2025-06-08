#include "Editor.h"
#include "EditorCommands.h"
#include <iostream> // For std::cout, std::cerr (used in printView, and potentially by TextBuffer methods if they still print errors)
#include <algorithm> // For std::min, std::max
#include <cctype>    // For isalnum, isspace
#include <fstream>   // For file operations (std::ifstream)
#include <sstream>   // For string stream operations
#include "AppDebugLog.h"
#include <chrono>    // For std::chrono
#include "MultiCursor.h"

// Constructor without dependencies - for backward compatibility
Editor::Editor()
    : textBuffer_(std::make_shared<TextBuffer>()),
      commandManager_(std::make_shared<CommandManager>()),
      syntaxHighlightingManager_(std::make_shared<SyntaxHighlightingManager>()),
      multiCursor_(std::make_unique<MultiCursor>())
{
    initialize();
}

// Constructor with dependencies injected
Editor::Editor(
    std::shared_ptr<ITextBuffer> textBuffer,
    std::shared_ptr<ICommandManager> commandManager,
    std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager,
    std::shared_ptr<IDiffEngine> diffEngine,
    std::shared_ptr<IMergeEngine> mergeEngine)
    : textBuffer_(textBuffer),
      commandManager_(commandManager),
      syntaxHighlightingManager_(syntaxHighlightingManager),
      multiCursor_(std::make_unique<MultiCursor>()),
      diffEngine_(diffEngine),
      mergeEngine_(mergeEngine)
{
    initialize();
}

// Common initialization logic
void Editor::initialize() {
    LOG_DEBUG("Initializing Editor");
    
    // Set a starting time for timeout detection
    auto startTime = std::chrono::high_resolution_clock::now();
    auto checkTimeout = [startTime]() -> bool {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (duration > 5000) {
            LOG_ERROR("Editor initialization timeout detected! Breaking out to prevent infinite loop.");
            return true;
        }
        return false;
    };
    
    // Ensure buffer starts non-empty for initial cursor validation, or handle empty buffer case.
    if (textBuffer_->isEmpty()) {
        textBuffer_->addLine(""); // Start with one empty line so cursor at (0,0) is valid.
    }
    validateAndClampCursor(); 
    
    // Check for timeout
    if (checkTimeout()) return;

    // Initialize syntax highlighting manager with the buffer
    if (auto* concreteBuffer = dynamic_cast<TextBuffer*>(textBuffer_.get())) {
        syntaxHighlightingManager_->setBuffer(concreteBuffer);
    } else {
        LOG_WARNING("Could not set buffer for syntax highlighting manager: buffer is not of type TextBuffer");
    }
    syntaxHighlightingManager_->setEnabled(syntaxHighlightingEnabled_);
    
    // Check for timeout
    if (checkTimeout()) return;

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
    
    // Check for timeout one last time
    if (checkTimeout()) return;
    
    LOG_DEBUG("Editor initialized successfully");
}

void Editor::setCursor(size_t line, size_t col) {
    // Handle empty buffer case
    if (textBuffer_->isEmpty()) {
        cursorLine_ = 0;
        cursorCol_ = 0;
        
        // Update multi-cursor if enabled
        if (multiCursorEnabled_ && multiCursor_) {
            multiCursor_->setPrimaryCursorPosition({0, 0});
        }
        
        return;
    }
    
    // Ensure line is within valid bounds
    line = std::min(line, textBuffer_->lineCount() - 1);
    
    // Ensure column is valid for the line
    const std::string& lineText = textBuffer_->getLine(line);
    col = std::min(col, lineText.length());
    
    // Set the cursor position
    cursorLine_ = line;
    cursorCol_ = col;
    
    // Update multi-cursor if enabled
    if (multiCursorEnabled_ && multiCursor_) {
        multiCursor_->setPrimaryCursorPosition({line, col});
    }
    
    // Adjust viewport if needed to ensure cursor visibility
    if (cursorLine_ < topVisibleLine_) {
        topVisibleLine_ = cursorLine_;
    } else if (cursorLine_ >= topVisibleLine_ + viewableLines_) {
        topVisibleLine_ = cursorLine_ - viewableLines_ + 1;
    }
    
    // Update code context
    updateCodeContext();
}

void Editor::moveCursorUp() {
    if (multiCursorEnabled_ && multiCursor_) {
        multiCursor_->moveCursors("up", *textBuffer_);
        
        // Update the primary editor cursor from the multi-cursor
        CursorPosition primaryPos = multiCursor_->getPrimaryCursorPosition();
        cursorLine_ = primaryPos.line;
        cursorCol_ = primaryPos.column;
    } else {
        if (cursorLine_ > 0) {
            // Move up one line
            cursorLine_--;
            
            // Adjust column if the new line is shorter
            const std::string& lineText = textBuffer_->getLine(cursorLine_);
            cursorCol_ = std::min(cursorCol_, lineText.length());
        }
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::moveCursorDown() {
    if (multiCursorEnabled_ && multiCursor_) {
        multiCursor_->moveCursors("down", *textBuffer_);
        
        // Update the primary editor cursor from the multi-cursor
        CursorPosition primaryPos = multiCursor_->getPrimaryCursorPosition();
        cursorLine_ = primaryPos.line;
        cursorCol_ = primaryPos.column;
    } else {
        if (cursorLine_ < textBuffer_->lineCount() - 1) {
            // Move down one line
            cursorLine_++;
            
            // Adjust column if the new line is shorter
            const std::string& lineText = textBuffer_->getLine(cursorLine_);
            cursorCol_ = std::min(cursorCol_, lineText.length());
        }
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::moveCursorLeft() {
    if (multiCursorEnabled_ && multiCursor_) {
        multiCursor_->moveCursors("left", *textBuffer_);
        
        // Update the primary editor cursor from the multi-cursor
        CursorPosition primaryPos = multiCursor_->getPrimaryCursorPosition();
        cursorLine_ = primaryPos.line;
        cursorCol_ = primaryPos.column;
    } else {
        if (cursorCol_ > 0) {
            // Move left one character
            cursorCol_--;
        } else if (cursorLine_ > 0) {
            // Move to the end of the previous line
            cursorLine_--;
            const std::string& lineText = textBuffer_->getLine(cursorLine_);
            cursorCol_ = lineText.length();
        }
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::moveCursorRight() {
    if (multiCursorEnabled_ && multiCursor_) {
        multiCursor_->moveCursors("right", *textBuffer_);
        
        // Update the primary editor cursor from the multi-cursor
        CursorPosition primaryPos = multiCursor_->getPrimaryCursorPosition();
        cursorLine_ = primaryPos.line;
        cursorCol_ = primaryPos.column;
    } else {
        const std::string& lineText = textBuffer_->getLine(cursorLine_);
        
        if (cursorCol_ < lineText.length()) {
            // Move right one character
            cursorCol_++;
        } else if (cursorLine_ < textBuffer_->lineCount() - 1) {
            // Move to the beginning of the next line
            cursorLine_++;
            cursorCol_ = 0;
        }
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::ensureCursorVisible() {
    // If cursor is above viewport, scroll up
    if (cursorLine_ < topVisibleLine_) {
        topVisibleLine_ = cursorLine_;
    }
    // If cursor is below viewport, scroll down
    else if (cursorLine_ >= topVisibleLine_ + viewableLines_) {
        topVisibleLine_ = cursorLine_ - viewableLines_ + 1;
    }
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
    // Clear the screen - note: we should avoid direct output when using a stream parameter
    // Instead of std::cout, use the provided stream parameter
    os << "\033[2J\033[H";
    
    // Calculate visible range
    size_t startLine = topVisibleLine_;
    size_t endLine = std::min(topVisibleLine_ + viewableLines_ - 1, textBuffer_->lineCount() - 1);
    
    // Update syntax highlighting for visible lines if enabled
    if (syntaxHighlightingEnabled_ && currentHighlighter_) {
        syntaxHighlightingManager_->setVisibleRange(startLine, endLine);
    }
    
    // Display line numbers and content
    for (size_t i = startLine; i <= endLine; ++i) {
        // Print line number
        os << std::setw(4) << (i + 1) << " | ";
        
        const std::string& line = textBuffer_->getLine(i);
        
        // Get syntax highlighting styles for this line if enabled
        std::vector<SyntaxStyle> lineStyles;
        if (syntaxHighlightingEnabled_ && currentHighlighter_) {
            lineStyles = syntaxHighlightingManager_->getHighlightingStyles(i, i)[0];
        }
        
        // Print the line content with highlighting if available
        if (!lineStyles.empty()) {
            printLineWithHighlighting(os, line, lineStyles);
        } else {
            os << line;
        }
        
        // Indicate if this is the cursor line
        if (i == cursorLine_) {
            os << " <--";
        }
        
        os << std::endl;
    }
    
    // Display status bar
    printStatusBar(os);
    
    // Note: We don't position cursor in the const method that outputs to a stream
    // This should be done separately if needed
}

void Editor::printLineWithHighlighting(std::ostream& os, const std::string& line, const std::vector<SyntaxStyle>& styles) const {
    // Default style
    const SyntaxColor defaultStyle = SyntaxColor::Default;
    
    for (size_t i = 0; i < line.length(); ++i) {
        SyntaxColor currentStyle = defaultStyle;
        
        // Find the appropriate style for this position
        for (const auto& style : styles) {
            if (i >= style.startCol && i < style.endCol) {
                currentStyle = style.color;
                break;
            }
        }
        
        // Apply the style and print the character
        applyColorForSyntaxColor(os, currentStyle);
        os << line[i];
    }
    
    // Reset to default style
    applyColorForSyntaxColor(os, defaultStyle);
}

// Helper function to apply console color based on syntax color
void Editor::applyColorForSyntaxColor([[maybe_unused]] std::ostream& os, [[maybe_unused]] SyntaxColor color) const {
    // This is a placeholder implementation
    // In a real implementation, this would set terminal/console colors
}

void Editor::printStatusBar(std::ostream& os) const {
    // Move to status bar position
    os << "\033[" << (viewableLines_ + 1) << ";0H";
    
    // Clear line
    os << "\033[K";
    
    // Print file information and cursor position
    std::string filename = filename_.empty() ? "[No File]" : filename_;
    os << filename << " - ";
    os << "Line: " << (cursorLine_ + 1) << ", Col: " << (cursorCol_ + 1);
    
    // Show modified indicator
    if (modified_) {
        os << " [Modified]";
    }
    
    // Show current highlighter if enabled
    if (syntaxHighlightingEnabled_ && currentHighlighter_) {
        os << " [" << currentHighlighter_->getLanguageName() << "]";
    }
}

void Editor::positionCursor() {
    // Ensure cursor is visible in the viewport
    if (cursorLine_ < topVisibleLine_) {
        topVisibleLine_ = cursorLine_;
    } else if (cursorLine_ >= topVisibleLine_ + viewableLines_) {
        topVisibleLine_ = cursorLine_ - viewableLines_ + 1;
    }
    
    // Calculate the screen position
    int screenLine = static_cast<int>(cursorLine_ - topVisibleLine_ + 1); // +1 for 1-based line counting on screen
    int screenCol = static_cast<int>(cursorCol_ + 6); // +6 for line number display and margin (e.g., "123 | ")
    
    // Position the cursor using ANSI escape sequence
    std::cout << "\033[" << screenLine << ";" << screenCol << "H";
}

size_t Editor::getBottomVisibleLine() const {
    return std::min(topVisibleLine_ + viewableLines_ - 1, textBuffer_->lineCount() - 1);
}

void Editor::startSelection() {
    if (multiCursorEnabled_ && multiCursor_) {
        // Start selection for all cursors
        for (size_t i = 0; i < multiCursor_->getCursorCount(); ++i) {
            multiCursor_->startSelection(i);
        }
    }
    
    // Always update the single-cursor selection too for backward compatibility
    hasSelection_ = true;
    selectionStartLine_ = cursorLine_;
    selectionStartCol_ = cursorCol_;
}

void Editor::endSelection() {
    // In practice, this just ensures that hasSelection_ is set to true
    // The actual selection is defined by the start and current cursor positions
    
    if (multiCursorEnabled_ && multiCursor_) {
        // Ending selection just means we're done moving it
        // The selection remains active until cleared
    }
    
    hasSelection_ = true;
}

std::string Editor::getSelectedText() const {
    if (!hasSelection()) {
        return "";
    }
    
    if (multiCursorEnabled_ && multiCursor_ && multiCursor_->getCursorCount() > 1) {
        // Combine text from all selections
        std::string combinedText;
        
        // Get all selections
        std::vector<TextSelection> selections = multiCursor_->getAllSelections();
        if (selections.empty()) {
            // Fall back to single selection
            return getSingleSelectionText();
        }
        
        // Sort selections by position
        std::sort(selections.begin(), selections.end(), 
                 [](const TextSelection& a, const TextSelection& b) {
                     return a.start < b.start;
                 });
        
        // Extract text for each selection
        for (const auto& selection : selections) {
            // Ensure selection is normalized (start <= end)
            TextSelection normalizedSelection = selection;
            normalizedSelection.normalize();
            
            const size_t startLine = normalizedSelection.start.line;
            const size_t startCol = normalizedSelection.start.column;
            const size_t endLine = normalizedSelection.end.line;
            const size_t endCol = normalizedSelection.end.column;
            
            std::string selectedText;
            
            if (startLine == endLine) {
                // Selection is within a single line
                const std::string& line = textBuffer_->getLine(startLine);
                selectedText = line.substr(startCol, endCol - startCol);
            } else {
                // Selection spans multiple lines
                // First line
                const std::string& firstLine = textBuffer_->getLine(startLine);
                selectedText = firstLine.substr(startCol) + "\n";
                
                // Middle lines (if any)
                for (size_t i = startLine + 1; i < endLine; ++i) {
                    selectedText += textBuffer_->getLine(i) + "\n";
                }
                
                // Last line
                const std::string& lastLine = textBuffer_->getLine(endLine);
                selectedText += lastLine.substr(0, endCol);
            }
            
            if (!combinedText.empty()) {
                combinedText += "\n";
            }
            combinedText += selectedText;
        }
        
        return combinedText;
    }
    
    // Single selection (traditional behavior)
    return getSingleSelectionText();
}

// Helper method to extract text from a single selection
std::string Editor::getSingleSelectionText() const {
    if (!hasSelection_) {
        return "";
    }

    // Get start and end positions, ensuring start is before end
    size_t startLine = selectionStartLine_;
    size_t startCol = selectionStartCol_;
    size_t endLine = cursorLine_;
    size_t endCol = cursorCol_;

    // Swap if selection is backwards
    if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
        std::swap(startLine, endLine);
        std::swap(startCol, endCol);
    }

    std::string selectedText;
    if (startLine == endLine) {
        // Single-line selection
        selectedText = textBuffer_->getLineSegment(startLine, startCol, endCol);
    } else {
        // Multi-line selection
        selectedText = textBuffer_->getLineSegment(startLine, startCol, textBuffer_->lineLength(startLine));
        selectedText += '\n';

        // Middle lines (if any)
        for (size_t line = startLine + 1; line < endLine; ++line) {
            selectedText += textBuffer_->getLine(line) + '\n';
        }

        // Last line
        selectedText += textBuffer_->getLineSegment(endLine, 0, endCol);
    }

    return selectedText;
}

bool Editor::undo() {
    if (!commandManager_->canUndo()) {
        return false;
    }
    
    bool result = commandManager_->undo(*this);
    if (result) {
        setModified(true);
        invalidateHighlightingCache();
    }
    
    return result;
}

bool Editor::redo() {
    if (!commandManager_->canRedo()) {
        return false;
    }
    
    bool result = commandManager_->redo(*this);
    if (result) {
        setModified(true);
        invalidateHighlightingCache();
    }
    
    return result;
}

void Editor::enableSyntaxHighlighting(bool enable) {
    syntaxHighlightingEnabled_ = enable;
    syntaxHighlightingManager_->setEnabled(enable);
    
    // If we're enabling it and don't have a highlighter yet, try to detect one
    if (enable && !currentHighlighter_) {
        detectAndSetHighlighter();
    }
    
    // Invalidate the cache so it will be regenerated on next view
    invalidateHighlightingCache();
}

void Editor::invalidateHighlightingCache() {
    highlightingStylesCacheValid_ = false;
    syntaxHighlightingManager_->invalidateAllLines();
}

std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() {
    updateHighlightingCache();
    return syntaxHighlightingManager_->getHighlightingStyles(0, textBuffer_->lineCount() - 1);
}

std::vector<std::vector<SyntaxStyle>> Editor::getHighlightingStyles() const {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        return std::vector<std::vector<SyntaxStyle>>();
    }
    
    return syntaxHighlightingManager_->getHighlightingStyles(0, textBuffer_->lineCount() - 1);
}

void Editor::updateHighlightingCache() {
    if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
        return;
    }
    
    // Determine visible range to highlight
    size_t startLine = getTopVisibleLine();
    size_t endLine = std::min(getBottomVisibleLine(), textBuffer_->lineCount() - 1);
    
    // Update the visible range in the highlighting manager
    syntaxHighlightingManager_->setVisibleRange(startLine, endLine);
    
    // Mark as valid
    highlightingStylesCacheValid_ = true;
}

void Editor::validateAndClampCursor() {
    // Ensure cursor stays within valid buffer bounds
    // First check if buffer is empty
    if (textBuffer_->isEmpty()) {
        cursorLine_ = 0;
        cursorCol_ = 0;
        return;
    }
    
    // Clamp line to valid range
    size_t maxLine = textBuffer_->lineCount() - 1;
    cursorLine_ = std::min(cursorLine_, maxLine);
    
    // Clamp column to valid range for the current line
    const std::string& lineText = textBuffer_->getLine(cursorLine_);
    cursorCol_ = std::min(cursorCol_, lineText.length());
}

void Editor::addLine(const std::string& text) {
    auto command = std::make_unique<AddLineCommand>(textBuffer_, text);
    commandManager_->executeCommand(std::move(command), *this);
    modified_ = true;
}

void Editor::insertLine(size_t lineIndex, const std::string& text) {
    auto command = std::make_unique<InsertLineCommand>(textBuffer_, lineIndex, text);
    commandManager_->executeCommand(std::move(command), *this);
    modified_ = true;
}

void Editor::deleteLine(size_t lineIndex) {
    auto command = std::make_unique<DeleteLineCommand>(textBuffer_, lineIndex);
    commandManager_->executeCommand(std::move(command), *this);
    modified_ = true;
}

void Editor::replaceLine(size_t lineIndex, const std::string& text) {
    auto command = std::make_unique<ReplaceLineCommand>(textBuffer_, lineIndex, text);
    commandManager_->executeCommand(std::move(command), *this);
    modified_ = true;
}

bool Editor::loadFile(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    // Directly load the file rather than using a command
    // This avoids potential recursive calls between commands and editor methods
    bool success = false;
    try {
        // Clear the buffer first
        while (textBuffer_->lineCount() > 0) {
            textBuffer_->deleteLine(0);
        }
        
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                textBuffer_->addLine(line);
            }
            
            // Only update state if the load was successful
            filename_ = filename;
            modified_ = false;
            cursorLine_ = 0;
            cursorCol_ = 0;
            
            // Reset viewport position
            topVisibleLine_ = 0;
            
            // Update syntax highlighting
            detectAndSetHighlighter();
            
            success = true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading file: " + std::string(e.what()));
        success = false;
    }
    
    return success;
}

bool Editor::saveFile() {
    if (filename_.empty()) {
        return false;
    }
    
    // Directly save the file instead of using a command
    // This avoids potential recursive calls between commands and editor methods
    bool success = false;
    try {
        std::ofstream file(filename_);
        if (file.is_open()) {
            // Write each line to the file
            for (size_t i = 0; i < textBuffer_->lineCount(); ++i) {
                file << textBuffer_->getLine(i);
                
                // Add newline if it's not the last line or the line doesn't end with newline
                if (i < textBuffer_->lineCount() - 1 || 
                    (textBuffer_->getLine(i).length() > 0 && 
                     textBuffer_->getLine(i).back() != '\n')) {
                    file << std::endl;
                }
            }
            success = true;
            modified_ = false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving file: " + std::string(e.what()));
        success = false;
    }
    
    return success;
}

bool Editor::saveFileAs(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    // Directly save the file instead of using a command
    // This avoids potential recursive calls between commands and editor methods
    bool success = false;
    try {
        std::ofstream file(filename);
        if (file.is_open()) {
            // Write each line to the file
            for (size_t i = 0; i < textBuffer_->lineCount(); ++i) {
                file << textBuffer_->getLine(i);
                
                // Add newline if it's not the last line or the line doesn't end with newline
                if (i < textBuffer_->lineCount() - 1 || 
                    (textBuffer_->getLine(i).length() > 0 && 
                     textBuffer_->getLine(i).back() != '\n')) {
                    file << std::endl;
                }
            }
            success = true;
            filename_ = filename;
            modified_ = false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving file: " + std::string(e.what()));
        success = false;
    }
    
    return success;
}

void Editor::setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) {
    if (!syntaxHighlightingEnabled_) {
        return;
    }
    
    currentHighlighter_ = highlighter;
    invalidateHighlightingCache();
}

void Editor::detectAndSetHighlighter() {
    if (!syntaxHighlightingEnabled_ || !syntaxHighlightingManager_) {
        return;
    }
    
    // Set a starting time for timeout detection
    auto startTime = std::chrono::high_resolution_clock::now();
    auto checkTimeout = [startTime]() -> bool {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (duration > 5000) {
            LOG_ERROR("detectAndSetHighlighter timeout detected! Breaking out to prevent infinite loop.");
            return true;
        }
        return false;
    };
    
    // Extract file extension from filename
    std::string extension;
    size_t dotPos = filename_.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = filename_.substr(dotPos + 1); // Get extension without the dot
    }
    
    // Check for timeout
    if (checkTimeout()) return;
    
    // Create or get appropriate highlighter based on extension
    std::shared_ptr<SyntaxHighlighter> highlighter = nullptr;
    
    // Get highlighter from registry without causing any callbacks to editor methods
    if (extension == "cpp" || extension == "h" || extension == "hpp") {
        highlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("cpp");
    } else if (extension == "py") {
        highlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("py");
    } else if (extension == "js") {
        highlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("js");
    } else if (extension == "html") {
        highlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("html");
    } else if (extension == "css") {
        highlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("css");
    } else {
        highlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("txt");
    }
    
    // Check for timeout
    if (checkTimeout()) return;
    
    // Only set the highlighter if we found one
    if (highlighter) {
        currentHighlighter_ = highlighter;
        invalidateHighlightingCache();
    }
}

void Editor::typeChar(char charToInsert) {
    // Directly handle character insertion without using commands
    if (hasSelection_) {
        // Get the selection bounds
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t startCol = (startLine == selectionStartLine_) ? selectionStartCol_ : selectionEndCol_;
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        size_t endCol = (endLine == selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
        
        // Delete the selected text
        directDeleteTextRange(startLine, startCol, endLine, endCol);
        
        // Set cursor to the deleted text position
        cursorLine_ = startLine;
        cursorCol_ = startCol;
        
        // Clear the selection
        clearSelection();
    }

    // Get the current line
    std::string currentLine = textBuffer_->getLine(cursorLine_);
    
    // Create the new line with the character inserted
    std::string newLine = currentLine.substr(0, cursorCol_) + charToInsert + currentLine.substr(cursorCol_);
    
    // Replace the line directly without using a command
    textBuffer_->replaceLine(cursorLine_, newLine);
    
    // Update cursor position
    cursorCol_++;
    
    // Mark as modified
    modified_ = true;
    
    // Update code context
    updateCodeContext();
}

void Editor::newLine() {
    // Directly insert a new line without using commands
    if (hasSelection_) {
        // Get the selection bounds
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t startCol = (startLine == selectionStartLine_) ? selectionStartCol_ : selectionEndCol_;
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        size_t endCol = (endLine == selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
        
        // Delete the selected text
        directDeleteTextRange(startLine, startCol, endLine, endCol);
        
        // Set cursor to the deleted text position
        cursorLine_ = startLine;
        cursorCol_ = startCol;
        
        // Clear the selection
        clearSelection();
    }

    std::string currentLine = textBuffer_->getLine(cursorLine_);
    std::string firstPart = currentLine.substr(0, cursorCol_);
    std::string secondPart = currentLine.substr(cursorCol_);

    // Replace the current line with the first part
    textBuffer_->replaceLine(cursorLine_, firstPart);
    
    // Insert the new line with the second part
    textBuffer_->insertLine(cursorLine_ + 1, secondPart);
    
    // Update cursor position
    cursorLine_++;
    cursorCol_ = 0;
    
    modified_ = true;
    
    // Update code context
    updateCodeContext();
}

void Editor::deleteCharacter() {
    // Directly handle delete character without using commands
    if (hasSelection_) {
        // Get the selection bounds
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t startCol = (startLine == selectionStartLine_) ? selectionStartCol_ : selectionEndCol_;
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        size_t endCol = (endLine == selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
        
        // Delete the selected text
        directDeleteTextRange(startLine, startCol, endLine, endCol);
        
        // Set cursor to the deleted text position
        cursorLine_ = startLine;
        cursorCol_ = startCol;
        
        // Clear the selection
        clearSelection();
        
        modified_ = true;
        return;
    }

    std::string currentLine = textBuffer_->getLine(cursorLine_);
    
    if (cursorCol_ < currentLine.length()) {
        // Delete character at cursor position
        std::string newLine = currentLine.substr(0, cursorCol_) + currentLine.substr(cursorCol_ + 1);
        
        // Replace the line directly
        textBuffer_->replaceLine(cursorLine_, newLine);
        modified_ = true;
    } else if (cursorLine_ < textBuffer_->lineCount() - 1) {
        // At the end of a line, join with the next line
        std::string nextLine = textBuffer_->getLine(cursorLine_ + 1);
        std::string combinedLine = currentLine + nextLine;
        
        // Replace current line with combined content
        textBuffer_->replaceLine(cursorLine_, combinedLine);
        
        // Delete the next line
        textBuffer_->deleteLine(cursorLine_ + 1);
        
        modified_ = true;
    }
}

void Editor::backspace() {
    // Directly handle backspace without using commands
    if (hasSelection_) {
        // Get the selection bounds
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t startCol = (startLine == selectionStartLine_) ? selectionStartCol_ : selectionEndCol_;
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        size_t endCol = (endLine == selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
        
        // Delete the selected text
        directDeleteTextRange(startLine, startCol, endLine, endCol);
        
        // Set cursor to the deleted text position
        cursorLine_ = startLine;
        cursorCol_ = startCol;
        
        // Clear the selection
        clearSelection();
        
        modified_ = true;
        return;
    }

    if (cursorCol_ > 0) {
        // Not at the beginning of a line, delete the character before the cursor
        std::string currentLine = textBuffer_->getLine(cursorLine_);
        std::string newLine = currentLine.substr(0, cursorCol_ - 1) + currentLine.substr(cursorCol_);
        
        // Replace the line directly
        textBuffer_->replaceLine(cursorLine_, newLine);
        
        cursorCol_--;
        modified_ = true;
    } 
    else if (cursorLine_ > 0) {
        // At the beginning of a line (not the first line), join with the previous line
        std::string previousLine = textBuffer_->getLine(cursorLine_ - 1);
        std::string currentLine = textBuffer_->getLine(cursorLine_);
        std::string combinedLine = previousLine + currentLine;
        
        // Replace the previous line with combined content
        textBuffer_->replaceLine(cursorLine_ - 1, combinedLine);
        
        // Delete the current line
        textBuffer_->deleteLine(cursorLine_);
        
        // Update cursor position
        cursorLine_--;
        cursorCol_ = previousLine.length();
        modified_ = true;
    }
}

void Editor::processCharacterInput(char ch) {
    if (ch == '\n') {
        this->newLine();
    } else if (ch == '\b') {
        this->backspace();
    } else if (ch >= 32 && ch <= 126) {  // Printable ASCII
        this->typeChar(ch);
    } else if (ch == '\t') {  // Tab character
        // Insert 4 spaces (or whatever tab size is configured)
        for (int i = 0; i < 4; ++i) {
            this->typeChar(' ');
        }
    }
}

class DeleteSelectionCommand : public Command {
public:
    DeleteSelectionCommand(std::shared_ptr<ITextBuffer> textBuffer, size_t startLine, size_t startCol, size_t endLine, size_t endCol)
        : textBuffer_(textBuffer), startLine_(startLine), startCol_(startCol), endLine_(endLine), endCol_(endCol) {}
    
    void execute([[maybe_unused]] Editor& editor) override {
        // Store the original text for undo
        originalText_ = "";
        auto& buffer = *textBuffer_;
        
        if (startLine_ == endLine_) {
            // Single line selection
            originalText_ = buffer.getLineSegment(startLine_, startCol_, endCol_);
        } else {
            // Multi-line selection
            // Get first line segment (from startCol to end of line)
            originalText_ = buffer.getLineSegment(startLine_, startCol_, buffer.lineLength(startLine_)) + "\n";
            
            // Get any middle lines in full
            for (size_t i = startLine_ + 1; i < endLine_; ++i) {
                originalText_ += buffer.getLine(i) + "\n";
            }
            
            // Get last line segment (from start of line to endCol)
            originalText_ += buffer.getLineSegment(endLine_, 0, endCol_);
        }
        
        // Delete the selected text
        buffer.deleteText(startLine_, startCol_, endLine_, endCol_);
    }
    
    void undo(Editor& editor) override {
        // Insert the original text back at the selection start
        auto& buffer = *textBuffer_;
        buffer.insertText(startLine_, startCol_, originalText_);
        
        // Restore the selection state
        editor.setSelectionRange(startLine_, startCol_, endLine_, endCol_);
        
        // Position cursor at the end of the selection (matching typical selection behavior)
        editor.setCursor(endLine_, endCol_);
    }
    
    std::string getDescription() const override {
        return "Delete selection";
    }
    
private:
    std::shared_ptr<ITextBuffer> textBuffer_;
    size_t startLine_;
    size_t startCol_;
    size_t endLine_;
    size_t endCol_;
    std::string originalText_;
};

void Editor::deleteSelection() {
    if (!hasSelection()) {
        return;
    }
    
    // Ensure start position is before end position
    size_t startLine = selectionStartLine_;
    size_t startCol = selectionStartCol_;
    size_t endLine = selectionEndLine_;
    size_t endCol = selectionEndCol_;
    
    if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
        std::swap(startLine, endLine);
        std::swap(startCol, endCol);
    }
    
    // Directly delete the selected range without using a command
    directDeleteTextRange(startLine, startCol, endLine, endCol);
    
    // Move cursor to the start of the deleted selection
    cursorLine_ = startLine;
    cursorCol_ = startCol;
    clearSelection();
    modified_ = true;
    
    // Update code context
    updateCodeContext();
}

void Editor::clearSelection() {
    if (multiCursorEnabled_ && multiCursor_) {
        // Clear selection for all cursors
        multiCursor_->clearAllSelections();
    }
    
    hasSelection_ = false;
    
    // Update code context
    updateCodeContext();
}

void Editor::setFilename(const std::string& filename) {
    filename_ = filename;
    detectAndSetHighlighter();
}

bool Editor::openFile(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    // loadFile already sets the filename and other state variables
    // so we don't need to call setFilename separately
    bool success = loadFile(filename);
    
    // Note: We don't call setFilename here anymore since loadFile already does that
    
    return success;
}

// Returns the extension of the current file
std::string Editor::getFileExtension() const {
    std::string ext;
    std::string filename = getFilename();
    
    // Check if the filename has a path component and extract just the filename
    size_t lastSlash = filename.find_last_of("/\\");
    std::string baseFilename = (lastSlash != std::string::npos) ? 
                               filename.substr(lastSlash + 1) : 
                               filename;
    
    // Handle special case for dotfiles (.hidden_file)
    // If the filename starts with a dot and has no other dots, it has no extension
    if (baseFilename.length() > 0 && baseFilename[0] == '.' && 
        baseFilename.find('.', 1) == std::string::npos) {
        return "";
    }
    
    size_t dotPos = baseFilename.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < baseFilename.length() - 1) {
        ext = baseFilename.substr(dotPos + 1);
    }
    
    return ext;
}

// Returns true if the file is new/untitled and unmodified
bool Editor::isNewFile() const {
    // New file is defined as untitled (or default name) and not modified
    return (filename_ == "untitled.txt" || filename_.empty()) && !isModified();
}

// Returns the text of the current line
std::string Editor::getCurrentLineText() const {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return "";
    }
    
    return textBuffer_->getLine(cursorLine_);
}

// Returns true if cursor is at the start of the line
bool Editor::isCursorAtLineStart() const {
    return cursorCol_ == 0;
}

// Returns true if cursor is at the end of the line
bool Editor::isCursorAtLineEnd() const {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return true;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    return cursorCol_ >= line.length();
}

// Returns true if cursor is at the start of the buffer
bool Editor::isCursorAtBufferStart() const {
    return cursorLine_ == 0 && cursorCol_ == 0;
}

// Returns true if cursor is at the end of the buffer
bool Editor::isCursorAtBufferEnd() const {
    if (textBuffer_->isEmpty()) {
        return true;
    }
    
    size_t lastLine = textBuffer_->lineCount() - 1;
    std::string lastLineText = textBuffer_->getLine(lastLine);
    
    return cursorLine_ == lastLine && cursorCol_ >= lastLineText.length();
}

// Returns the first visible line in the viewport
size_t Editor::getViewportStartLine() const {
    return topVisibleLine_;
}

// Returns the height of the viewport in lines
size_t Editor::getViewportHeight() const {
    return viewableLines_;
}

// Returns the word at the current cursor position
std::string Editor::getWordUnderCursor() const {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return "";
    }
    
    const std::string& line = textBuffer_->getLine(cursorLine_);
    if (line.empty() || cursorCol_ > line.length()) {
        return "";
    }
    
    // If we're on whitespace between words, return empty string
    if (cursorCol_ < line.length() && isWhitespace(line[cursorCol_]) && 
        (cursorCol_ == 0 || isWhitespace(line[cursorCol_ - 1]))) {
        return "";
    }
    
    // Find the start of the word
    size_t wordStart = cursorCol_;
    while (wordStart > 0 && (isWordChar(line[wordStart - 1]) || isSpecialChar(line[wordStart - 1]))) {
        wordStart--;
    }
    
    // If cursor is at the end of a word, move it to the last character
    if (cursorCol_ > 0 && cursorCol_ < line.length() && 
        isWordChar(line[cursorCol_ - 1]) && !isWordChar(line[cursorCol_])) {
        // We're just after a word, so we'll consider the previous word
    }
    
    // Find the end of the word
    size_t wordEnd = cursorCol_;
    while (wordEnd < line.length() && (isWordChar(line[wordEnd]) || isSpecialChar(line[wordEnd]))) {
        wordEnd++;
    }
    
    // If we found a valid word, return it
    if (wordEnd > wordStart) {
        return line.substr(wordStart, wordEnd - wordStart);
    }
    
    return "";
}

// Print buffer content to stream for debugging/testing
void Editor::printBuffer(std::ostream& os) const {
    if (textBuffer_->isEmpty()) {
        return;
    }
    
    for (size_t i = 0; i < textBuffer_->lineCount(); ++i) {
        os << textBuffer_->getLine(i);
        if (i < textBuffer_->lineCount() - 1 || textBuffer_->getLine(i).back() != '\n') {
            os << '\n';
        }
    }
}

// Character classification methods used by getWordUnderCursor()
bool Editor::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool Editor::isWordChar(char c) const {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || 
           c == '_';
}

bool Editor::isSpecialChar(char c) const {
    // Characters that are not word chars or whitespace
    return !isWordChar(c) && !isWhitespace(c);
}

// Terminal dimension methods (stubbed for now)
int Editor::getTerminalWidth() const {
    return displayWidth_;
}

int Editor::getTerminalHeight() const {
    return displayHeight_;
}

void Editor::moveCursorToLineStart() {
    cursorCol_ = 0;
}

void Editor::moveCursorToLineEnd() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    cursorCol_ = line.length();
}

void Editor::moveCursorToBufferStart() {
    cursorLine_ = 0;
    cursorCol_ = 0;
}

void Editor::moveCursorToBufferEnd() {
    if (textBuffer_->isEmpty()) {
        cursorLine_ = 0;
        cursorCol_ = 0;
        return;
    }
    
    cursorLine_ = textBuffer_->lineCount() - 1;
    std::string lastLine = textBuffer_->getLine(cursorLine_);
    cursorCol_ = lastLine.length();
}

void Editor::deleteWord() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    if (line.empty() || cursorCol_ >= line.length()) {
        // At the end of the line, delete the newline character
        joinWithNextLine();
        return;
    }
    
    // Find the end of the word
    size_t wordEnd = cursorCol_;
    // If we're at the beginning of a word, delete that word
    if (wordEnd < line.length() && isWordChar(line[wordEnd])) {
        while (wordEnd < line.length() && isWordChar(line[wordEnd])) {
            wordEnd++;
        }
    } 
    // If we're at a special character or whitespace, delete until the next word
    else {
        // Skip current character type (whitespace or special char)
        char currentType = line[wordEnd];
        bool isCurrentWhitespace = isWhitespace(currentType);
        
        while (wordEnd < line.length() && 
              (isCurrentWhitespace ? isWhitespace(line[wordEnd]) : isSpecialChar(line[wordEnd]))) {
            wordEnd++;
        }
    }
    
    // Delete from cursor to word end
    if (wordEnd > cursorCol_) {
        std::string newLine = line.substr(0, cursorCol_) + line.substr(wordEnd);
        textBuffer_->replaceLine(cursorLine_, newLine);
        setModified(true);
    }
}

// Selection methods
void Editor::setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    if (multiCursorEnabled_ && multiCursor_) {
        // Set selection for primary cursor
        CursorPosition start{startLine, startCol};
        CursorPosition end{endLine, endCol};
        multiCursor_->setSelectionRange(start, end, 0);
    }
    
    // Always update the single-cursor selection too for backward compatibility
    hasSelection_ = true;
    selectionStartLine_ = startLine;
    selectionStartCol_ = startCol;
    cursorLine_ = endLine;
    cursorCol_ = endCol;
    
    // Update code context
    updateCodeContext();
}

void Editor::updateSelection() {
    if (multiCursorEnabled_ && multiCursor_) {
        // Update selection for all cursors
        for (size_t i = 0; i < multiCursor_->getCursorCount(); ++i) {
            multiCursor_->updateSelection(i);
        }
    }
    
    // Always update the single-cursor selection too for backward compatibility
    hasSelection_ = true;
}

void Editor::selectLine() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return;
    }
    
    setSelectionRange(cursorLine_, 0, cursorLine_, 
                     textBuffer_->getLine(cursorLine_).length());
}

void Editor::selectAll() {
    if (textBuffer_->isEmpty()) {
        return;
    }
    
    size_t lastLine = textBuffer_->lineCount() - 1;
    size_t lastLineLength = textBuffer_->getLine(lastLine).length();
    
    setSelectionRange(0, 0, lastLine, lastLineLength);
}

void Editor::shrinkSelection([[maybe_unused]] SelectionUnit targetUnit) {
    if (!hasSelection_) {
        return;
    }
    
    // For now, just clear the selection
    // In the future, this could implement more sophisticated selection shrinking
    clearSelection();
}

// Clipboard operations
void Editor::cutSelection() {
    if (!hasSelection_) {
        return;
    }
    
    copySelection();
    deleteSelection();
}

void Editor::copySelection() {
    if (!hasSelection_) {
        return;
    }
    
    std::string selectedText = getSelectedText();
    setClipboardText(selectedText);
}

void Editor::pasteAtCursor() {
    std::string text = getClipboardText();
    if (!text.empty()) {
        replaceSelection(text);
    }
}

std::string Editor::getClipboardText() const {
    return clipboard_;
}

void Editor::setClipboardText(const std::string& text) {
    clipboard_ = text;
}

// Search operations
bool Editor::searchNext() {
    if (currentSearchTerm_.empty()) {
        return false;
    }
    
    return search(currentSearchTerm_, currentSearchCaseSensitive_, true);
}

bool Editor::searchPrevious() {
    if (currentSearchTerm_.empty()) {
        return false;
    }
    
    return search(currentSearchTerm_, currentSearchCaseSensitive_, false);
}

// Replace operations
bool Editor::replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive) {
    if (searchTerm.empty()) {
        return false;
    }
    
    std::string originalText;
    size_t replacedAtLine, replacedAtCol, originalEndLine, originalEndCol;
    
    if (performReplaceLogic(searchTerm, replacementText, caseSensitive, 
                           originalText, replacedAtLine, replacedAtCol, 
                           originalEndLine, originalEndCol)) {
        setModified(true);
        return true;
    }
    
    return false;
}

bool Editor::replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive) {
    if (searchTerm.empty()) {
        return false;
    }
    
    // Save cursor position
    size_t savedLine = cursorLine_;
    size_t savedCol = cursorCol_;
    
    // Start from the beginning of the document
    cursorLine_ = 0;
    cursorCol_ = 0;
    
    bool replacedAny = false;
    bool replacedOne;
    
    do {
        replacedOne = replace(searchTerm, replacementText, caseSensitive);
        replacedAny |= replacedOne;
    } while (replacedOne);
    
    // Restore cursor position
    cursorLine_ = savedLine;
    cursorCol_ = savedCol;
    validateAndClampCursor();
    
    return replacedAny;
}

// Helper to set cursor position
void Editor::setCursorPosition(const Position& pos) {
    cursorLine_ = pos.line;
    cursorCol_ = pos.column;
    validateAndClampCursor();
}

void Editor::typeText(const std::string& textToInsert) {
    if (textToInsert.empty()) {
        return;
    }
    
    // Directly insert text without using commands
    if (hasSelection_) {
        // Get the selection bounds
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t startCol = (startLine == selectionStartLine_) ? selectionStartCol_ : selectionEndCol_;
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        size_t endCol = (endLine == selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
        
        // Delete the selected text directly
        directDeleteTextRange(startLine, startCol, endLine, endCol);
        
        // Insert the new text
        size_t newEndLine, newEndCol;
        directInsertText(startLine, startCol, textToInsert, newEndLine, newEndCol);
        
        // Update cursor position
        cursorLine_ = newEndLine;
        cursorCol_ = newEndCol;
        
        // Clear the selection
        clearSelection();
    } else {
        // No selection, just insert at cursor
        size_t endLine, endCol;
        directInsertText(cursorLine_, cursorCol_, textToInsert, endLine, endCol);
        
        // Update cursor position
        cursorLine_ = endLine;
        cursorCol_ = endCol;
    }
    
    modified_ = true;
    
    // Update code context
    updateCodeContext();
}

void Editor::deleteForward() {
    if (hasSelection_) {
        deleteSelection();
        return;
    }
    
    if (textBuffer_->isEmpty()) {
        return;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    
    // If cursor is at the end of the line, join with next line
    if (cursorCol_ >= line.length()) {
        joinWithNextLine();
        return;
    }
    
    // Delete character at cursor position
    std::string newLine = line.substr(0, cursorCol_) + line.substr(cursorCol_ + 1);
    textBuffer_->replaceLine(cursorLine_, newLine);
    setModified(true);
}

void Editor::joinWithNextLine() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount() - 1) {
        return;
    }
    
    std::string currentLine = textBuffer_->getLine(cursorLine_);
    std::string nextLine = textBuffer_->getLine(cursorLine_ + 1);
    
    // Combine the lines
    std::string combinedLine = currentLine + nextLine;
    
    // Replace current line with combined content
    textBuffer_->replaceLine(cursorLine_, combinedLine);
    
    // Delete the next line
    textBuffer_->deleteLine(cursorLine_ + 1);
    
    setModified(true);
}

void Editor::increaseIndent() {
    if (textBuffer_->isEmpty()) {
        return;
    }
    
    if (hasSelection_) {
        // Indent all lines in the selection
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        
        for (size_t i = startLine; i <= endLine; ++i) {
            std::string line = textBuffer_->getLine(i);
            textBuffer_->replaceLine(i, "    " + line);  // Use 4 spaces for indentation
        }
    } else {
        // Indent just the current line
        std::string line = textBuffer_->getLine(cursorLine_);
        textBuffer_->replaceLine(cursorLine_, "    " + line);
        
        // Adjust cursor position
        cursorCol_ += 4;
    }
    
    setModified(true);
}

void Editor::decreaseIndent() {
    if (textBuffer_->isEmpty()) {
        return;
    }
    
    if (hasSelection_) {
        // Unindent all lines in the selection
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        
        for (size_t i = startLine; i <= endLine; ++i) {
            std::string line = textBuffer_->getLine(i);
            
            // Remove up to 4 spaces or a tab at the beginning of the line
            size_t spaceCount = 0;
            while (spaceCount < line.length() && spaceCount < 4 && line[spaceCount] == ' ') {
                spaceCount++;
            }
            
            if (spaceCount > 0) {
                textBuffer_->replaceLine(i, line.substr(spaceCount));
            } else if (!line.empty() && line[0] == '\t') {
                textBuffer_->replaceLine(i, line.substr(1));
            }
        }
    } else {
        // Unindent just the current line
        std::string line = textBuffer_->getLine(cursorLine_);
        
        // Remove up to 4 spaces or a tab at the beginning of the line
        size_t spaceCount = 0;
        while (spaceCount < line.length() && spaceCount < 4 && line[spaceCount] == ' ') {
            spaceCount++;
        }
        
        if (spaceCount > 0) {
            textBuffer_->replaceLine(cursorLine_, line.substr(spaceCount));
            
            // Adjust cursor position
            cursorCol_ = cursorCol_ > spaceCount ? cursorCol_ - spaceCount : 0;
        } else if (!line.empty() && line[0] == '\t') {
            textBuffer_->replaceLine(cursorLine_, line.substr(1));
            
            // Adjust cursor position
            cursorCol_ = cursorCol_ > 0 ? cursorCol_ - 1 : 0;
        }
    }
    
    setModified(true);
}

bool Editor::isSyntaxHighlightingEnabled() const {
    return syntaxHighlightingEnabled_;
}

std::string Editor::getFilename() const {
    return filename_;
}

std::shared_ptr<SyntaxHighlighter> Editor::getCurrentHighlighter() const {
    return currentHighlighter_;
}

void Editor::replaceSelection(const std::string& text) {
    // Directly implement replacement without using commands
    if (hasSelection_) {
        // Get the selection bounds
        size_t startLine = std::min(selectionStartLine_, selectionEndLine_);
        size_t startCol = (startLine == selectionStartLine_) ? selectionStartCol_ : selectionEndCol_;
        size_t endLine = std::max(selectionStartLine_, selectionEndLine_);
        size_t endCol = (endLine == selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
        
        // Delete the selected text
        directDeleteTextRange(startLine, startCol, endLine, endCol);
        
        // Insert the new text
        size_t newEndLine, newEndCol;
        directInsertText(startLine, startCol, text, newEndLine, newEndCol);
        
        // Update cursor position
        cursorLine_ = newEndLine;
        cursorCol_ = newEndCol;
        
        // Clear the selection
        clearSelection();
        
        modified_ = true;
    } else {
        // No selection, just insert at cursor
        size_t endLine, endCol;
        directInsertText(cursorLine_, cursorCol_, text, endLine, endCol);
        
        // Update cursor position
        cursorLine_ = endLine;
        cursorCol_ = endCol;
        
        modified_ = true;
    }
}

void Editor::directDeleteTextRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    // Validate input
    if (textBuffer_->isEmpty() || startLine >= textBuffer_->lineCount() || 
        endLine >= textBuffer_->lineCount() || 
        (startLine == endLine && startCol >= endCol)) {
        return;
    }
    
    // Handle single line case
    if (startLine == endLine) {
        std::string line = textBuffer_->getLine(startLine);
        if (startCol >= line.length() || endCol > line.length()) {
            return;
        }
        
        std::string newLine = line.substr(0, startCol) + line.substr(endCol);
        textBuffer_->replaceLine(startLine, newLine);
        return;
    }
    
    // Handle multi-line case
    std::string startLineText = textBuffer_->getLine(startLine);
    std::string endLineText = textBuffer_->getLine(endLine);
    
    // Create new combined line
    std::string newLine = startLineText.substr(0, startCol);
    if (endCol < endLineText.length()) {
        newLine += endLineText.substr(endCol);
    }
    
    // Replace the start line with the new combined content
    textBuffer_->replaceLine(startLine, newLine);
    
    // Delete all lines in between
    for (size_t i = endLine; i > startLine; --i) {
        textBuffer_->deleteLine(i);
    }
}

void Editor::directInsertText(size_t line, size_t col, const std::string& text, size_t& outEndLine, size_t& outEndCol) {
    // Validate input
    if (line >= textBuffer_->lineCount()) {
        // If inserting beyond buffer end, add empty lines
        while (textBuffer_->lineCount() <= line) {
            textBuffer_->addLine("");
        }
    }
    
    std::string currentLine = textBuffer_->getLine(line);
    
    // Ensure column is valid
    if (col > currentLine.length()) {
        col = currentLine.length();
    }
    
    // Check if text contains newlines
    size_t pos = 0;
    size_t nextNewline = text.find('\n', pos);
    
    if (nextNewline == std::string::npos) {
        // Simple case: no newlines in text
        std::string newLine = currentLine.substr(0, col) + text + currentLine.substr(col);
        textBuffer_->replaceLine(line, newLine);
        
        // Set output position
        outEndLine = line;
        outEndCol = col + text.length();
        return;
    }
    
    // Complex case: text contains newlines
    // First line: from start of current line to first newline in text
    std::string firstLine = currentLine.substr(0, col) + text.substr(0, nextNewline);
    textBuffer_->replaceLine(line, firstLine);
    
    // Insert middle lines
    size_t currentPos = nextNewline + 1;
    size_t lineOffset = 1;
    
    while (true) {
        nextNewline = text.find('\n', currentPos);
        
        if (nextNewline == std::string::npos) {
            // Last segment without newline
            break;
        }
        
        std::string middleLine = text.substr(currentPos, nextNewline - currentPos);
        textBuffer_->insertLine(line + lineOffset, middleLine);
        
        currentPos = nextNewline + 1;
        lineOffset++;
    }
    
    // Last line: from last newline to end of text + remainder of original line
    std::string lastLine = text.substr(currentPos) + currentLine.substr(col);
    textBuffer_->insertLine(line + lineOffset, lastLine);
    
    // Set output position
    outEndLine = line + lineOffset;
    outEndCol = text.substr(currentPos).length();
}

bool Editor::performReplaceLogic(
    const std::string& searchTerm, 
    const std::string& replacementText, 
    bool caseSensitive, 
    std::string& outOriginalText, 
    size_t& outReplacedAtLine, 
    size_t& outReplacedAtCol,
    size_t& outOriginalEndLine,
    size_t& outOriginalEndCol) {
    
    // First, find the search term
    size_t foundLine, foundCol;
    if (!performSearchLogic(searchTerm, caseSensitive, true, foundLine, foundCol)) {
        return false;
    }
    
    // Store outputs
    outReplacedAtLine = foundLine;
    outReplacedAtCol = foundCol;
    
    // Calculate end position of search term
    outOriginalEndLine = foundLine;
    outOriginalEndCol = foundCol + searchTerm.length();
    
    // Handle multi-line search terms
    std::string lineText = textBuffer_->getLine(foundLine);
    if (foundCol + searchTerm.length() > lineText.length()) {
        // This is a multi-line match, we need to handle it differently
        // For now, we'll just support single-line replacements
        // In a full implementation, we'd need to track multi-line selections
        return false;
    }
    
    // Store original text
    outOriginalText = lineText.substr(foundCol, searchTerm.length());
    
    // Perform the replacement
    std::string newLine = lineText.substr(0, foundCol) + replacementText + 
                         lineText.substr(foundCol + searchTerm.length());
    
    textBuffer_->replaceLine(foundLine, newLine);
    
    // Update cursor position
    cursorLine_ = foundLine;
    cursorCol_ = foundCol + replacementText.length();
    
    return true;
}

bool Editor::performSearchLogic(
    const std::string& searchTerm, 
    bool caseSensitive, 
    bool forward,
    size_t& outFoundLine, 
    size_t& outFoundCol) {
    
    if (searchTerm.empty() || textBuffer_->isEmpty()) {
        return false;
    }
    
    // Starting position for search
    size_t startLine = cursorLine_;
    size_t startCol = cursorCol_;
    
    // When searching forward, start from current position
    // When searching backward, start from just before current position
    if (!forward && startCol > 0) {
        startCol--;
    } else if (!forward && startCol == 0) {
        if (startLine > 0) {
            startLine--;
            std::string prevLine = textBuffer_->getLine(startLine);
            startCol = prevLine.length();
        }
    }
    
    // Helper for case-insensitive comparison
    auto compareChars = [caseSensitive](char a, char b) -> bool {
        if (caseSensitive) {
            return a == b;
        } else {
            return std::tolower(a) == std::tolower(b);
        }
    };
    
    // Search logic
    if (forward) {
        // Forward search
        for (size_t line = startLine; line < textBuffer_->lineCount(); ++line) {
            std::string lineText = textBuffer_->getLine(line);
            size_t col = (line == startLine) ? startCol : 0;
            
            while (col <= lineText.length() - searchTerm.length()) {
                bool found = true;
                
                for (size_t i = 0; i < searchTerm.length(); ++i) {
                    if (!compareChars(lineText[col + i], searchTerm[i])) {
                        found = false;
                        break;
                    }
                }
                
                if (found) {
                    outFoundLine = line;
                    outFoundCol = col;
                    return true;
                }
                
                col++;
            }
        }
        
        // If we reach here, wrap around to beginning of file
        for (size_t line = 0; line <= startLine; ++line) {
            std::string lineText = textBuffer_->getLine(line);
            size_t maxCol = (line == startLine) ? startCol : lineText.length();
            
            for (size_t col = 0; col < maxCol; ++col) {
                if (col > lineText.length() - searchTerm.length()) {
                    break;
                }
                
                bool found = true;
                
                for (size_t i = 0; i < searchTerm.length(); ++i) {
                    if (!compareChars(lineText[col + i], searchTerm[i])) {
                        found = false;
                        break;
                    }
                }
                
                if (found) {
                    outFoundLine = line;
                    outFoundCol = col;
                    return true;
                }
            }
        }
    } else {
        // Backward search
        for (int line = static_cast<int>(startLine); line >= 0; --line) {
            std::string lineText = textBuffer_->getLine(line);
            size_t maxCol = (line == static_cast<int>(startLine)) ? startCol : lineText.length();
            
            for (int col = static_cast<int>(maxCol) - static_cast<int>(searchTerm.length()); col >= 0; --col) {
                bool found = true;
                
                for (size_t i = 0; i < searchTerm.length(); ++i) {
                    if (!compareChars(lineText[col + i], searchTerm[i])) {
                        found = false;
                        break;
                    }
                }
                
                if (found) {
                    outFoundLine = line;
                    outFoundCol = col;
                    return true;
                }
            }
        }
        
        // If we reach here, wrap around to end of file
        for (int line = static_cast<int>(textBuffer_->lineCount()) - 1; line >= static_cast<int>(startLine); --line) {
            std::string lineText = textBuffer_->getLine(line);
            
            if (line == static_cast<int>(startLine) && lineText.length() <= startCol) {
                continue; // Already searched this part
            }
            
            for (int col = static_cast<int>(lineText.length()) - static_cast<int>(searchTerm.length()); col >= 0; --col) {
                bool found = true;
                
                for (size_t i = 0; i < searchTerm.length(); ++i) {
                    if (!compareChars(lineText[col + i], searchTerm[i])) {
                        found = false;
                        break;
                    }
                }
                
                if (found) {
                    outFoundLine = line;
                    outFoundCol = col;
                    return true;
                }
            }
        }
    }
    
    // Not found
    return false;
}

void Editor::setLine(size_t lineIndex, const std::string& text) {
    if (lineIndex >= textBuffer_->lineCount()) {
        // Add lines if necessary
        while (textBuffer_->lineCount() <= lineIndex) {
            textBuffer_->addLine("");
        }
    }
    
    textBuffer_->replaceLine(lineIndex, text);
    setModified(true);
}

// Selection coordinate getters
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

// Search operation
bool Editor::search(const std::string& searchTerm, bool caseSensitive, bool forward) {
    if (searchTerm.empty() || textBuffer_->isEmpty()) {
        return false;
    }
    
    // Save current search term and case sensitivity
    currentSearchTerm_ = searchTerm;
    currentSearchCaseSensitive_ = caseSensitive;
    
    // Perform search
    size_t foundLine, foundCol;
    if (performSearchLogic(searchTerm, caseSensitive, forward, foundLine, foundCol)) {
        // Move cursor to found position
        cursorLine_ = foundLine;
        cursorCol_ = foundCol;
        
        // Update last search position
        lastSearchLine_ = foundLine;
        lastSearchCol_ = foundCol;
        
        // Select the found text
        setSelectionRange(foundLine, foundCol, foundLine, foundCol + searchTerm.length());
        
        return true;
    }
    
    return false;
}

void Editor::moveCursorToNextWord() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    
    // If we're at the end of the line, move to the next line
    if (cursorCol_ >= line.length()) {
        if (cursorLine_ < textBuffer_->lineCount() - 1) {
            cursorLine_++;
            cursorCol_ = 0;
            line = textBuffer_->getLine(cursorLine_);
        } else {
            // Already at the end of the buffer
            return;
        }
    }
    
    // Skip any current word characters
    if (cursorCol_ < line.length() && isWordChar(line[cursorCol_])) {
        // We're in a word, move to the end of it
        while (cursorCol_ < line.length() && isWordChar(line[cursorCol_])) {
            cursorCol_++;
        }
    }
    
    // Skip any whitespace or special characters
    while (cursorCol_ < line.length() && !isWordChar(line[cursorCol_])) {
        cursorCol_++;
    }
    
    // If we're at the end of the line but not the end of the buffer, move to the next line
    if (cursorCol_ >= line.length() && cursorLine_ < textBuffer_->lineCount() - 1) {
        cursorLine_++;
        cursorCol_ = 0;
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::moveCursorToPrevWord() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    
    // If we're at the beginning of the line, move to the previous line
    if (cursorCol_ == 0) {
        if (cursorLine_ > 0) {
            cursorLine_--;
            line = textBuffer_->getLine(cursorLine_);
            cursorCol_ = line.length();
        } else {
            // Already at the beginning of the buffer
            return;
        }
    } else {
        // Move back one position to start checking from the previous character
        cursorCol_--;
    }
    
    // Skip any whitespace or special characters backwards
    while (cursorCol_ > 0 && !isWordChar(line[cursorCol_])) {
        cursorCol_--;
    }
    
    // Skip any word characters backwards to find the start of the word
    if (cursorCol_ > 0 && isWordChar(line[cursorCol_])) {
        while (cursorCol_ > 0 && isWordChar(line[cursorCol_ - 1])) {
            cursorCol_--;
        }
    }
    
    // If we're at the beginning of the line and not at the start of the buffer, move to previous line
    if (cursorCol_ == 0 && cursorLine_ > 0 && line.empty()) {
        cursorLine_--;
        line = textBuffer_->getLine(cursorLine_);
        cursorCol_ = line.length();
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::selectWord() {
    if (textBuffer_->isEmpty() || cursorLine_ >= textBuffer_->lineCount()) {
        return;
    }
    
    std::string line = textBuffer_->getLine(cursorLine_);
    if (line.empty()) {
        // Nothing to select on an empty line
        return;
    }
    
    // If the cursor is beyond the end of the line, adjust it
    if (cursorCol_ >= line.length()) {
        cursorCol_ = line.length() > 0 ? line.length() - 1 : 0;
    }
    
    // If we're on whitespace, we might want to select all adjacent whitespace
    if (isWhitespace(line[cursorCol_])) {
        // Find the start of the whitespace
        size_t start = cursorCol_;
        while (start > 0 && isWhitespace(line[start - 1])) {
            start--;
        }
        
        // Find the end of the whitespace
        size_t end = cursorCol_;
        while (end < line.length() - 1 && isWhitespace(line[end + 1])) {
            end++;
        }
        
        setSelectionRange(cursorLine_, start, cursorLine_, end + 1);
        return;
    }
    
    // Get the boundaries of the word at the cursor
    auto [wordStart, wordEnd] = findWordBoundaries(cursorLine_, cursorCol_);
    
    // If we found a valid word, select it
    if (wordEnd > wordStart) {
        setSelectionRange(cursorLine_, wordStart, cursorLine_, wordEnd);
        // Update selection unit for future operations
        updateSelectionUnit(SelectionUnit::Word);
    }
}

std::pair<size_t, size_t> Editor::findWordBoundaries(size_t line, size_t col) const {
    if (textBuffer_->isEmpty() || line >= textBuffer_->lineCount()) {
        return {0, 0};
    }
    
    const std::string& lineText = textBuffer_->getLine(line);
    if (lineText.empty() || col >= lineText.length()) {
        return {0, 0};
    }
    
    // Find the start of the word
    size_t wordStart = col;
    if (isWordChar(lineText[wordStart])) {
        // If we're on a word character, go back to the start of the word
        while (wordStart > 0 && isWordChar(lineText[wordStart - 1])) {
            wordStart--;
        }
    } else if (isSpecialChar(lineText[wordStart])) {
        // If we're on a special character, just select that character
        return {wordStart, wordStart + 1};
    } else {
        // We're on whitespace, try to find the next word
        size_t nextWordStart = wordStart;
        while (nextWordStart < lineText.length() && isWhitespace(lineText[nextWordStart])) {
            nextWordStart++;
        }
        
        if (nextWordStart < lineText.length()) {
            wordStart = nextWordStart;
        } else {
            // No word found after whitespace
            return {wordStart, wordStart};
        }
    }
    
    // Find the end of the word
    size_t wordEnd = wordStart;
    if (wordStart < lineText.length() && isWordChar(lineText[wordStart])) {
        // If we found a word, find its end
        while (wordEnd < lineText.length() && isWordChar(lineText[wordEnd])) {
            wordEnd++;
        }
    } else if (wordStart < lineText.length() && isSpecialChar(lineText[wordStart])) {
        // If we found a special character, just select that one character
        wordEnd = wordStart + 1;
    }
    
    return {wordStart, wordEnd};
}

void Editor::updateSelectionUnit(SelectionUnit unit) {
    currentSelectionUnit_ = unit;
}

bool Editor::findNext(const std::string& pattern) {
    // Store the search pattern for potential repeat searches
    currentSearchTerm_ = pattern;
    currentSearchCaseSensitive_ = true; // Default to case sensitive

    // Start search from current cursor position
    size_t foundLine = 0;
    size_t foundCol = 0;
    
    // Perform the search
    if (performSearchLogic(pattern, true, true, foundLine, foundCol)) {
        // Set cursor to the found position
        setCursor(foundLine, foundCol);
        
        // Update last search position for next search
        lastSearchLine_ = foundLine;
        lastSearchCol_ = foundCol + pattern.length(); // Start next search after this match
        
        return true;
    }
    
    return false;
}

void Editor::pageUp() {
    // Move cursor up by the number of viewable lines (or to the top if less than that)
    size_t linesToMove = std::min(viewableLines_, cursorLine_);
    if (linesToMove > 0) {
        cursorLine_ -= linesToMove;
        
        // Adjust cursor column if needed
        const std::string& lineText = textBuffer_->getLine(cursorLine_);
        cursorCol_ = std::min(cursorCol_, lineText.length());
        
        // Ensure cursor is visible - adjust viewport
        if (cursorLine_ < topVisibleLine_) {
            topVisibleLine_ = cursorLine_;
        }
    }
}

void Editor::pageDown() {
    size_t maxLine = textBuffer_->lineCount() - 1;
    
    // Move cursor down by the number of viewable lines (or to the bottom if less than that)
    size_t linesToMove = std::min(viewableLines_, maxLine - cursorLine_);
    if (linesToMove > 0) {
        cursorLine_ += linesToMove;
        
        // Adjust cursor column if needed
        const std::string& lineText = textBuffer_->getLine(cursorLine_);
        cursorCol_ = std::min(cursorCol_, lineText.length());
        
        // Ensure cursor is visible - adjust viewport
        size_t bottomVisibleLine = topVisibleLine_ + viewableLines_ - 1;
        if (cursorLine_ > bottomVisibleLine) {
            topVisibleLine_ = cursorLine_ - viewableLines_ + 1;
        }
    }
}

// Multiple cursor operations implementation
bool Editor::isMultiCursorEnabled() const {
    return multiCursorEnabled_;
}

void Editor::setMultiCursorEnabled(bool enable) {
    multiCursorEnabled_ = enable;
    
    if (enable) {
        // Initialize the multi-cursor if it doesn't exist
        if (!multiCursor_) {
            multiCursor_ = std::make_unique<MultiCursor>();
            
            // Set the primary cursor to the current editor cursor
            CursorPosition primaryPos{cursorLine_, cursorCol_};
            multiCursor_->setPrimaryCursorPosition(primaryPos);
        }
    } else {
        // When disabling multi-cursor, keep only the primary cursor
        if (multiCursor_) {
            removeAllSecondaryCursors();
            
            // Get the primary cursor position and update the editor cursor
            CursorPosition primaryPos = multiCursor_->getPrimaryCursorPosition();
            cursorLine_ = primaryPos.line;
            cursorCol_ = primaryPos.column;
        }
    }
}

size_t Editor::getCursorCount() const {
    if (!multiCursorEnabled_ || !multiCursor_) {
        return 1; // Default to single cursor when multi-cursor is disabled
    }
    
    return multiCursor_->getCursorCount();
}

bool Editor::addCursor(size_t line, size_t col) {
    // Enable multi-cursor mode if it's not already enabled
    if (!multiCursorEnabled_) {
        setMultiCursorEnabled(true);
    }
    
    // Add the cursor
    CursorPosition newPos{line, col};
    return multiCursor_->addCursor(newPos);
}

bool Editor::removeCursor(size_t line, size_t col) {
    if (!multiCursorEnabled_ || !multiCursor_) {
        return false;
    }
    
    CursorPosition pos{line, col};
    return multiCursor_->removeCursor(pos);
}

void Editor::removeAllSecondaryCursors() {
    if (multiCursorEnabled_ && multiCursor_) {
        multiCursor_->removeAllSecondaryCursors();
    }
}

size_t Editor::addCursorsAtAllOccurrences(const std::string& text, bool caseSensitive) {
    if (text.empty()) {
        return 0;
    }
    
    // Enable multi-cursor mode if it's not already enabled
    if (!multiCursorEnabled_) {
        setMultiCursorEnabled(true);
    }
    
    return multiCursor_->addCursorsAtAllOccurrences(text, *textBuffer_, caseSensitive);
}

size_t Editor::addCursorsAtColumn(size_t startLine, size_t endLine, size_t column) {
    // Enable multi-cursor mode if it's not already enabled
    if (!multiCursorEnabled_) {
        setMultiCursorEnabled(true);
    }
    
    return multiCursor_->addCursorsAtColumn(startLine, endLine, column, *textBuffer_);
}

IMultiCursor& Editor::getMultiCursor() {
    // Initialize the multi-cursor if it doesn't exist
    if (!multiCursor_) {
        multiCursor_ = std::make_unique<MultiCursor>();
        
        // Set the primary cursor to the current editor cursor
        CursorPosition primaryPos{cursorLine_, cursorCol_};
        multiCursor_->setPrimaryCursorPosition(primaryPos);
    }
    
    return *multiCursor_;
}

const IMultiCursor& Editor::getMultiCursor() const {
    // This should never happen if we always initialize in the non-const getter
    if (!multiCursor_) {
        static MultiCursor dummyCursor;
        return dummyCursor;
    }
    
    return *multiCursor_;
}

// Add a new private method to update code context in the AIAgentOrchestrator
void Editor::updateCodeContext()
{
    // Skip if we don't have an AIAgentOrchestrator
    if (!aiAgentOrchestrator_) {
        return;
    }
    
    // Get the current file path
    std::string filePath = getFilename();
    if (filePath.empty()) {
        return;
    }
    
    // Get the current cursor position
    size_t line = getCursorLine();
    size_t column = getCursorCol();
    
    // Get the currently selected text
    std::string selectedText = hasSelection() ? getSelectedText() : "";
    
    // Get list of visible files (in a real implementation, this would come from the editor's UI)
    std::vector<std::string> visibleFiles = {filePath}; // Just the current file for now
    
    // Update the context in the AIAgentOrchestrator
    aiAgentOrchestrator_->updateEditingContext(filePath, line, column, selectedText, visibleFiles);
}

// Add a new method to set the AIAgentOrchestrator
void Editor::setAIAgentOrchestrator(std::shared_ptr<ai_editor::AIAgentOrchestrator> orchestrator)
{
    aiAgentOrchestrator_ = orchestrator;
    
    // Update context immediately if we have a valid orchestrator
    if (aiAgentOrchestrator_) {
        updateCodeContext();
    }
}
