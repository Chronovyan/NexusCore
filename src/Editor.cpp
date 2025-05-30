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
    if (cursorLine_ < viewportTopLine_) {
        viewportTopLine_ = cursorLine_;
    }
    // If cursor is below viewport, scroll down
    else if (cursorLine_ >= viewportTopLine_ + viewableLines_) {
        viewportTopLine_ = cursorLine_ - viewableLines_ + 1;
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

void Editor::printView() {
    // Clear the screen
    std::cout << "\033[2J\033[H";
    
    // Calculate visible range
    size_t startLine = viewportTopLine_;
    size_t endLine = std::min(viewportTopLine_ + viewableLines_ - 1, textBuffer_->lineCount() - 1);
    
    // Update syntax highlighting for visible lines if enabled
    if (syntaxHighlightingEnabled_ && currentHighlighter_) {
        syntaxHighlightingManager_->setVisibleRange(startLine, endLine);
    }
    
    // Display line numbers and content
    for (size_t i = startLine; i <= endLine; ++i) {
        // Print line number
        std::cout << std::setw(4) << (i + 1) << " | ";
        
        const std::string& line = textBuffer_->getLine(i);
        
        // Get syntax highlighting styles for this line if enabled
        std::vector<SyntaxStyle> lineStyles;
        if (syntaxHighlightingEnabled_ && currentHighlighter_) {
            lineStyles = syntaxHighlightingManager_->getHighlightingStyles(i, i)[0];
        }
        
        // Print the line content with highlighting if available
        if (!lineStyles.empty()) {
            printLineWithHighlighting(line, lineStyles);
        } else {
            std::cout << line;
        }
        
        // Indicate if this is the cursor line
        if (i == cursorLine_) {
            std::cout << " ←";
        }
        
        std::cout << std::endl;
    }
    
    // Display status bar
    printStatusBar();
    
    // Position cursor
    positionCursor();
}

void Editor::printLineWithHighlighting(const std::string& line, const std::vector<SyntaxStyle>& styles) {
    // Default style
    const SyntaxStyle defaultStyle = {SyntaxStyleType::Normal, 0};
    
    for (size_t i = 0; i < line.length(); ++i) {
        // Find style for this character
        SyntaxStyle style = defaultStyle;
        
        for (const auto& s : styles) {
            if (s.position <= i && i < s.position + s.length) {
                style = s;
                break;
            }
        }
        
        // Apply the style
        applyStyle(style.type);
        
        // Print the character
        std::cout << line[i];
        
        // Reset to normal
        std::cout << "\033[0m";
    }
}

void Editor::printStatusBar() {
    // Move to status bar position
    std::cout << "\033[" << (viewableLines_ + 1) << ";0H";
    
    // Clear line
    std::cout << "\033[K";
    
    // Print file information and cursor position
    std::string filename = filePath_.empty() ? "[No File]" : filePath_;
    std::cout << filename << " - ";
    std::cout << "Line: " << (cursorLine_ + 1) << ", Col: " << (cursorCol_ + 1);
    
    // Show modified indicator
    if (isModified_) {
        std::cout << " [Modified]";
    }
    
    // Show current highlighter if enabled
    if (syntaxHighlightingEnabled_ && currentHighlighter_) {
        std::cout << " [" << currentHighlighter_->getName() << "]";
    }
}

void Editor::positionCursor() {
    // Calculate screen position
    size_t screenLine = cursorLine_ - viewportTopLine_ + 1; // 1-based for ANSI
    size_t screenCol = cursorCol_ + 7; // 7 = 4 (line number width) + 3 (separator " | ")
    
    // Set cursor position
    std::cout << "\033[" << screenLine << ";" << screenCol << "H";
    std::cout.flush();
}

size_t Editor::getTopVisibleLine() const {
    return viewportTopLine_;
}

size_t Editor::getBottomVisibleLine() const {
    return std::min(viewportTopLine_ + viewableLines_ - 1, textBuffer_->lineCount() - 1);
}

void Editor::startSelection() {
    selectionActive_ = true;
    selectionStartLine_ = cursorLine_;
    selectionStartCol_ = cursorCol_;
}

void Editor::endSelection() {
    selectionActive_ = false;
}

bool Editor::hasSelection() const {
    return selectionActive_ && (selectionStartLine_ != cursorLine_ || selectionStartCol_ != cursorCol_);
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
    size_t maxLine = std::max(0UL, textBuffer_->lineCount() - 1);
    cursorLine_ = std::min(cursorLine_, maxLine);
    
    size_t lineLength = textBuffer_->getLine(cursorLine_).length();
    cursorCol_ = std::min(cursorCol_, lineLength);
}

void Editor::addLine(const std::string& text) {
    auto command = std::make_shared<InsertLineCommand>(textBuffer_, textBuffer_->lineCount(), text);
    commandManager_->executeCommand(command);
    setModified(true);
}

void Editor::insertLine(size_t lineIndex, const std::string& text) {
    auto command = std::make_shared<InsertLineCommand>(textBuffer_, lineIndex, text);
    commandManager_->executeCommand(command);
    setModified(true);
}

void Editor::deleteLine(size_t lineIndex) {
    auto command = std::make_shared<DeleteLineCommand>(textBuffer_, lineIndex);
    commandManager_->executeCommand(command);
    setModified(true);
}

void Editor::replaceLine(size_t lineIndex, const std::string& text) {
    auto command = std::make_shared<ReplaceLineCommand>(textBuffer_, lineIndex, text);
    commandManager_->executeCommand(command);
    setModified(true);
}

bool Editor::loadFile(const std::string& filePath) {
    // Create a command for loading a file
    auto command = std::make_shared<LoadFileCommand>(textBuffer_, filePath);
    bool success = commandManager_->executeCommand(command);
    
    if (success) {
        filePath_ = filePath;
        isModified_ = false;
        cursorLine_ = 0;
        cursorCol_ = 0;
        
        // Try to detect syntax highlighting based on file extension
        detectAndSetHighlighter();
        
        // Reset view position
        viewportTopLine_ = 0;
    }
    
    return success;
}

bool Editor::saveFile(const std::string& filePath) {
    std::string pathToUse = filePath.empty() ? filePath_ : filePath;
    
    if (pathToUse.empty()) {
        return false;
    }
    
    // Create a command for saving a file
    auto command = std::make_shared<SaveFileCommand>(textBuffer_, pathToUse);
    bool success = commandManager_->executeCommand(command);
    
    if (success) {
        if (!filePath.empty()) {
            filePath_ = filePath;
        }
        isModified_ = false;
    }
    
    return success;
}

void Editor::detectAndSetHighlighter() {
    if (!syntaxHighlightingEnabled_) {
        return;
    }
    
    // Detect highlighter based on file extension
    if (!filePath_.empty()) {
        size_t dotPos = filePath_.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string extension = filePath_.substr(dotPos + 1);
            setHighlighter(extension);
        }
    }
}

void Editor::insertCharacter(char c) {
    const std::string& currentLine = textBuffer_->getLine(cursorLine_);
    
    // Create a new line with the character inserted
    std::string newLine = currentLine;
    newLine.insert(cursorCol_, 1, c);
    
    // Create and execute a command to replace the line
    auto command = std::make_shared<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
    commandManager_->executeCommand(command);
    
    // Move cursor right
    cursorCol_++;
    
    // Mark editor as modified
    setModified(true);
}

void Editor::insertNewline() {
    const std::string& currentLine = textBuffer_->getLine(cursorLine_);
    
    // Split the current line at cursor position
    std::string firstPart = currentLine.substr(0, cursorCol_);
    std::string secondPart = currentLine.substr(cursorCol_);
    
    // Create commands to update the current line and insert a new one
    auto replaceCommand = std::make_shared<ReplaceLineCommand>(textBuffer_, cursorLine_, firstPart);
    auto insertCommand = std::make_shared<InsertLineCommand>(textBuffer_, cursorLine_ + 1, secondPart);
    
    // Execute the commands as a batch
    auto batch = std::make_shared<BatchCommand>();
    batch->addCommand(replaceCommand);
    batch->addCommand(insertCommand);
    commandManager_->executeCommand(batch);
    
    // Move cursor to the beginning of the new line
    cursorLine_++;
    cursorCol_ = 0;
    
    // Mark editor as modified
    setModified(true);
}

void Editor::deleteCharacter() {
    const std::string& currentLine = textBuffer_->getLine(cursorLine_);
    
    if (cursorCol_ < currentLine.length()) {
        // Delete character at cursor position
        std::string newLine = currentLine;
        newLine.erase(cursorCol_, 1);
        
        // Create and execute a command to replace the line
        auto command = std::make_shared<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
        commandManager_->executeCommand(command);
        
        // Mark editor as modified
        setModified(true);
    } else if (cursorLine_ < textBuffer_->lineCount() - 1) {
        // At end of line, join with next line
        std::string nextLine = textBuffer_->getLine(cursorLine_ + 1);
        std::string newLine = currentLine + nextLine;
        
        // Create commands to update the current line and delete the next one
        auto replaceCommand = std::make_shared<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
        auto deleteCommand = std::make_shared<DeleteLineCommand>(textBuffer_, cursorLine_ + 1);
        
        // Execute the commands as a batch
        auto batch = std::make_shared<BatchCommand>();
        batch->addCommand(replaceCommand);
        batch->addCommand(deleteCommand);
        commandManager_->executeCommand(batch);
        
        // Mark editor as modified
        setModified(true);
    }
}

void Editor::backspace() {
    if (cursorCol_ > 0) {
        // Delete character before cursor
        const std::string& currentLine = textBuffer_->getLine(cursorLine_);
        std::string newLine = currentLine;
        newLine.erase(cursorCol_ - 1, 1);
        
        // Create and execute a command to replace the line
        auto command = std::make_shared<ReplaceLineCommand>(textBuffer_, cursorLine_, newLine);
        commandManager_->executeCommand(command);
        
        // Move cursor left
        cursorCol_--;
        
        // Mark editor as modified
        setModified(true);
    } else if (cursorLine_ > 0) {
        // At beginning of line, join with previous line
        const std::string& prevLine = textBuffer_->getLine(cursorLine_ - 1);
        const std::string& currentLine = textBuffer_->getLine(cursorLine_);
        std::string newLine = prevLine + currentLine;
        
        // Create commands to update the previous line and delete the current one
        auto replaceCommand = std::make_shared<ReplaceLineCommand>(textBuffer_, cursorLine_ - 1, newLine);
        auto deleteCommand = std::make_shared<DeleteLineCommand>(textBuffer_, cursorLine_);
        
        // Execute the commands as a batch
        auto batch = std::make_shared<BatchCommand>();
        batch->addCommand(replaceCommand);
        batch->addCommand(deleteCommand);
        commandManager_->executeCommand(batch);
        
        // Move cursor to the join point
        cursorLine_--;
        cursorCol_ = prevLine.length();
        
        // Mark editor as modified
        setModified(true);
    }
}

// Implementation of getBuffer for backward compatibility
TextBuffer& Editor::getBuffer() {
    // Try to cast the textBuffer_ to a TextBuffer
    TextBuffer* concreteBuffer = dynamic_cast<TextBuffer*>(textBuffer_.get());
    if (!concreteBuffer) {
        // If the cast fails, log an error and throw an exception
        LOG_ERROR("getBuffer called on an Editor with a non-TextBuffer implementation");
        throw std::runtime_error("getBuffer called on an Editor with a non-TextBuffer implementation");
    }
    return *concreteBuffer;
}

// ...and so on for other methods that access the private members we've changed
