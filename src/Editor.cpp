#include "Editor.h"
#include "EditorCommands.h"
#include <iostream> // For std::cout, std::cerr (used in printView, and potentially by TextBuffer methods if they still print errors)
#include <algorithm> // For std::min, std::max
#include <cctype>    // For isalnum, isspace
#include <fstream>   // For file operations (std::ifstream)
#include <sstream>   // For string stream operations
#include "AppDebugLog.h"

// Constructor without dependencies - for backward compatibility
Editor::Editor()
    : textBuffer_(std::make_shared<TextBuffer>()),
      commandManager_(std::make_shared<CommandManager>()),
      syntaxHighlightingManager_(std::make_shared<SyntaxHighlightingManager>()),
      cursorLine_(0), cursorCol_(0), 
      hasSelection_(false), selectionStartLine_(0), selectionStartCol_(0),
      selectionEndLine_(0), selectionEndCol_(0), clipboard_(""),
      currentSearchTerm_(""), currentSearchCaseSensitive_(true),
      lastSearchLine_(0), lastSearchCol_(0), searchWrapped_(false),
      syntaxHighlightingEnabled_(false), filename_("untitled.txt"), currentHighlighter_(nullptr),
      highlightingStylesCacheValid_(false), commandLineHeight_(1), statusLineHeight_(1),
      displayWidth_(80), displayHeight_(24), viewableLines_(22) {
    initialize();
}

// Constructor with dependencies injected
Editor::Editor(
    std::shared_ptr<ITextBuffer> textBuffer,
    std::shared_ptr<ICommandManager> commandManager,
    std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager
)
    : textBuffer_(textBuffer),
      commandManager_(commandManager),
      syntaxHighlightingManager_(syntaxHighlightingManager),
      cursorLine_(0), cursorCol_(0), 
      hasSelection_(false), selectionStartLine_(0), selectionStartCol_(0),
      selectionEndLine_(0), selectionEndCol_(0), clipboard_(""),
      currentSearchTerm_(""), currentSearchCaseSensitive_(true),
      lastSearchLine_(0), lastSearchCol_(0), searchWrapped_(false),
      syntaxHighlightingEnabled_(false), filename_("untitled.txt"), currentHighlighter_(nullptr),
      highlightingStylesCacheValid_(false), commandLineHeight_(1), statusLineHeight_(1),
      displayWidth_(80), displayHeight_(24), viewableLines_(22) {
    initialize();
}

// Common initialization logic
void Editor::initialize() {
    LOG_DEBUG("Initializing Editor");
    
    // Ensure buffer starts non-empty for initial cursor validation, or handle empty buffer case.
    if (textBuffer_->isEmpty()) {
        textBuffer_->addLine(""); // Start with one empty line so cursor at (0,0) is valid.
    }
    validateAndClampCursor(); 

    // Initialize syntax highlighting manager with the buffer
    if (auto* concreteBuffer = dynamic_cast<TextBuffer*>(textBuffer_.get())) {
        syntaxHighlightingManager_->setBuffer(concreteBuffer);
    } else {
        LOG_WARNING("Could not set buffer for syntax highlighting manager: buffer is not of type TextBuffer");
    }
    syntaxHighlightingManager_->setEnabled(syntaxHighlightingEnabled_);

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
    
    LOG_DEBUG("Editor initialized successfully");
}

void Editor::setCursor(size_t line, size_t col) {
    // Ensure we're within valid bounds
    line = std::min(line, textBuffer_->lineCount() - 1);
    
    // Make sure the column is valid for the line
    if (line < textBuffer_->lineCount()) {
        const std::string& lineText = textBuffer_->getLine(line);
        col = std::min(col, lineText.length());
    } else {
        col = 0;
    }
    
    // Set the cursor position
    cursorLine_ = line;
    cursorCol_ = col;
    
    // Adjust viewport if needed to ensure cursor visibility
    ensureCursorVisible();
}

void Editor::moveCursorUp() {
    if (cursorLine_ > 0) {
        // Move up one line
        cursorLine_--;
        
        // Adjust column if the new line is shorter
        const std::string& lineText = textBuffer_->getLine(cursorLine_);
        cursorCol_ = std::min(cursorCol_, lineText.length());
        
        // Ensure cursor is visible
        ensureCursorVisible();
    }
}

void Editor::moveCursorDown() {
    if (cursorLine_ < textBuffer_->lineCount() - 1) {
        // Move down one line
        cursorLine_++;
        
        // Adjust column if the new line is shorter
        const std::string& lineText = textBuffer_->getLine(cursorLine_);
        cursorCol_ = std::min(cursorCol_, lineText.length());
        
        // Ensure cursor is visible
        ensureCursorVisible();
    }
}

void Editor::moveCursorLeft() {
    if (cursorCol_ > 0) {
        // Move left one character
        cursorCol_--;
    } else if (cursorLine_ > 0) {
        // Move to the end of the previous line
        cursorLine_--;
        const std::string& lineText = textBuffer_->getLine(cursorLine_);
        cursorCol_ = lineText.length();
    }
    
    // Ensure cursor is visible
    ensureCursorVisible();
}

void Editor::moveCursorRight() {
    const std::string& lineText = textBuffer_->getLine(cursorLine_);
    
    if (cursorCol_ < lineText.length()) {
        // Move right one character
        cursorCol_++;
    } else if (cursorLine_ < textBuffer_->lineCount() - 1) {
        // Move to the beginning of the next line
        cursorLine_++;
        cursorCol_ = 0;
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
            os << " ←";
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
void Editor::applyColorForSyntaxColor(std::ostream& os, SyntaxColor color) const {
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
    int screenLine = cursorLine_ - topVisibleLine_ + 1; // +1 for 1-based line counting on screen
    int screenCol = cursorCol_ + 6; // +6 for line number display and margin (e.g., "123 | ")
    
    // Position the cursor using ANSI escape sequence
    std::cout << "\033[" << screenLine << ";" << screenCol << "H";
}

size_t Editor::getBottomVisibleLine() const {
    return std::min(topVisibleLine_ + viewableLines_ - 1, textBuffer_->lineCount() - 1);
}

void Editor::startSelection() {
    hasSelection_ = true;
    selectionStartLine_ = cursorLine_;
    selectionStartCol_ = cursorCol_;
}

void Editor::endSelection() {
    hasSelection_ = true;
}

std::string Editor::getSelectedText() const {
    if (!hasSelection()) {
        return "";
    }
    
    // Determine the start and end points of the selection
    size_t startLine, startCol, endLine, endCol;
    if (selectionStartLine_ < cursorLine_ || (selectionStartLine_ == cursorLine_ && selectionStartCol_ < cursorCol_)) {
        startLine = selectionStartLine_;
        startCol = selectionStartCol_;
        endLine = cursorLine_;
        endCol = cursorCol_;
    } else {
        startLine = cursorLine_;
        startCol = cursorCol_;
        endLine = selectionStartLine_;
        endCol = selectionStartCol_;
    }
    
    // Extract the selected text
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
    size_t maxLine = std::max<size_t>(0, textBuffer_->lineCount() - 1);
    cursorLine_ = std::min(cursorLine_, maxLine);
    
    size_t lineLength = textBuffer_->getLine(cursorLine_).length();
    cursorCol_ = std::min(cursorCol_, lineLength);
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
    
    auto command = std::make_unique<LoadFileCommand>(textBuffer_, filename);
    commandManager_->executeCommand(std::move(command), *this);
    
    filename_ = filename;
    modified_ = false;
    cursorLine_ = 0;
    cursorCol_ = 0;
    
    // Reset viewport position
    topVisibleLine_ = 0;
    
    detectAndSetHighlighter();
    return true;
}

bool Editor::saveFile() {
    if (filename_.empty()) {
        return false;
    }
    
    auto command = std::make_unique<SaveFileCommand>(textBuffer_, filename_);
    commandManager_->executeCommand(std::move(command), *this);
    modified_ = false;
    return true;
}

bool Editor::saveFileAs(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    auto command = std::make_unique<SaveFileCommand>(textBuffer_, filename);
    commandManager_->executeCommand(std::move(command), *this);
    filename_ = filename;
    modified_ = false;
    return true;
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
    
    std::string extension;
    size_t dotPos = filename_.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = filename_.substr(dotPos);
    }
    
    if (extension == ".cpp" || extension == ".h" || extension == ".hpp") {
        setHighlighter(SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("cpp"));
    } else if (extension == ".py") {
        setHighlighter(SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("py"));
    } else if (extension == ".js") {
        setHighlighter(SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("js"));
    } else if (extension == ".html") {
        setHighlighter(SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("html"));
    } else if (extension == ".css") {
        setHighlighter(SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("css"));
    } else {
        setHighlighter(SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("txt"));
    }
}

void Editor::typeChar(char charToInsert) {
    if (hasSelection()) {
        deleteSelection();
    }

    std::string currentLine = textBuffer_->getLine(cursorLine_);
    std::string newLine = currentLine.substr(0, cursorCol_) + charToInsert + currentLine.substr(cursorCol_);

    auto command = std::make_unique<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
    bool success = false;
    if (command) {
        commandManager_->executeCommand(std::move(command), *this);
        success = true;
    }
    if (success) {
        cursorCol_++;
        modified_ = true;
    }
}

void Editor::newLine() {
    if (hasSelection()) {
        deleteSelection();
    }

    std::string currentLine = textBuffer_->getLine(cursorLine_);
    std::string firstPart = currentLine.substr(0, cursorCol_);
    std::string secondPart = currentLine.substr(cursorCol_);

    // Replace the original line
    auto replaceCmd = std::make_unique<ReplaceLineCommand>(textBuffer_, cursorLine_, firstPart);
    
    // Insert the new line
    auto insertCmd = std::make_unique<InsertLineCommand>(textBuffer_, cursorLine_ + 1, secondPart);
    
    // Create a batch command
    auto batchCommand = std::make_unique<BatchCommand>();
    batchCommand->addCommand(std::move(replaceCmd));
    batchCommand->addCommand(std::move(insertCmd));
    
    // Execute the batch command
    commandManager_->executeCommand(std::move(batchCommand), *this);
    
    // Update cursor position
    cursorLine_++;
    cursorCol_ = 0;
    
    modified_ = true;
}

void Editor::deleteCharacter() {
    if (hasSelection()) {
        deleteSelection();
        return;
    }

    std::string currentLine = textBuffer_->getLine(cursorLine_);
    
    if (cursorCol_ < currentLine.length()) {
        // Delete character at cursor position
        std::string newLine = currentLine.substr(0, cursorCol_) + currentLine.substr(cursorCol_ + 1);
        auto command = std::make_unique<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
        commandManager_->executeCommand(std::move(command), *this);
        modified_ = true;
    } else if (cursorLine_ < textBuffer_->getLineCount() - 1) {
        // At the end of a line, join with the next line
        std::string nextLine = textBuffer_->getLine(cursorLine_ + 1);
        std::string combinedLine = currentLine + nextLine;
        
        // Create batch command
        auto batchCommand = std::make_unique<BatchCommand>();
        
        // Replace current line with combined content
        auto replaceCommand = std::make_unique<ReplaceLineCommand>(textBuffer_, cursorLine_, combinedLine);
        batchCommand->addCommand(std::move(replaceCommand));
        
        // Delete the next line (which is now combined with the current one)
        auto deleteCommand = std::make_unique<DeleteLineCommand>(textBuffer_, cursorLine_ + 1);
        batchCommand->addCommand(std::move(deleteCommand));
        
        // Execute the batch command
        commandManager_->executeCommand(std::move(batchCommand), *this);
        
        modified_ = true;
    }
}

void Editor::backspace() {
    if (hasSelection()) {
        deleteSelection();
        return;
    }

    if (cursorCol_ > 0) {
        // Not at the beginning of a line, delete the character before the cursor
        std::string currentLine = textBuffer_->getLine(cursorLine_);
        std::string newLine = currentLine.substr(0, cursorCol_ - 1) + currentLine.substr(cursorCol_);
        
        auto command = std::make_unique<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
        commandManager_->executeCommand(std::move(command), *this);
        
        cursorCol_--;
        modified_ = true;
    } 
    else if (cursorLine_ > 0) {
        // At the beginning of a line (not the first line), join with the previous line
        std::string previousLine = textBuffer_->getLine(cursorLine_ - 1);
        std::string currentLine = textBuffer_->getLine(cursorLine_);
        std::string combinedLine = previousLine + currentLine;
        
        // Create batch command
        auto batchCommand = std::make_unique<BatchCommand>();
        
        // Replace the previous line with combined content
        auto replaceCommand = std::make_unique<ReplaceLineCommand>(textBuffer_, cursorLine_ - 1, combinedLine);
        batchCommand->addCommand(std::move(replaceCommand));
        
        // Delete the current line (which is now combined with the previous one)
        auto deleteCommand = std::make_unique<DeleteLineCommand>(textBuffer_, cursorLine_);
        batchCommand->addCommand(std::move(deleteCommand));
        
        // Execute the batch command
        commandManager_->executeCommand(std::move(batchCommand), *this);
        
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
    
    void execute(Editor& editor) override {
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
    
    auto command = std::make_unique<DeleteSelectionCommand>(textBuffer_, startLine, startCol, endLine, endCol);
    commandManager_->executeCommand(std::move(command), *this);
    
    // Move cursor to the start of the deleted selection
    cursorLine_ = startLine;
    cursorCol_ = startCol;
    clearSelection();
    modified_ = true;
}

void Editor::clearSelection() {
    hasSelection_ = false;
    selectionStartLine_ = 0;
    selectionStartCol_ = 0;
    selectionEndLine_ = 0;
    selectionEndCol_ = 0;
}

void Editor::setFilename(const std::string& filename) {
    filename_ = filename;
    detectAndSetHighlighter();
}

bool Editor::openFile(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    bool success = loadFile(filename);
    if (success) {
        setFilename(filename);
    }
    
    return success;
}
