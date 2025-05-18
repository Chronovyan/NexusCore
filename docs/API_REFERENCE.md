# TextEditor API Reference

This document provides a reference for the primary API methods available in the TextEditor application, with special emphasis on the selection and text manipulation capabilities.

## Core Editor API

### Buffer & File Operations

```cpp
// Buffer operations
void addLine(const std::string& text);
void insertLine(size_t lineIndex, const std::string& text);
void deleteLine(size_t lineIndex);
void replaceLine(size_t lineIndex, const std::string& text);
TextBuffer& getBuffer();
const TextBuffer& getBuffer() const;

// File operations
bool openFile(const std::string& filename);
bool saveFile();
bool saveFile(const std::string& filename);
std::string getFilename() const;
void setFilename(const std::string& filename);
bool isModified() const;
void setModified(bool modified);
```

### Cursor Management

```cpp
// Cursor positioning
void setCursor(size_t line, size_t col);
size_t getCursorLine() const;
size_t getCursorCol() const;

// Cursor movement
void moveCursorUp();
void moveCursorDown();
void moveCursorLeft();
void moveCursorRight();
void moveCursorToLineStart();
void moveCursorToLineEnd();
void moveCursorToBufferStart();
void moveCursorToBufferEnd();
void moveCursorToNextWord();
void moveCursorToPrevWord();

// Cursor state querying
bool isCursorAtLineStart() const;
bool isCursorAtLineEnd() const;
bool isCursorAtBufferStart() const;
bool isCursorAtBufferEnd() const;
```

### Text Editing

```cpp
// Text input
void typeText(const std::string& textToInsert);
void typeChar(char charToInsert);
void processCharacterInput(char ch);

// Text deletion
void backspace();
void deleteForward();
void deleteWord();

// Line operations
void newLine();
void joinWithNextLine();

// Indentation
void increaseIndent();
void decreaseIndent();
```

### Undo/Redo

```cpp
bool canUndo() const;
bool canRedo() const;
bool undo();
bool redo();
```

### Search and Replace

```cpp
bool search(const std::string& searchTerm, bool caseSensitive = true, bool forward = true);
bool searchNext();
bool searchPrevious();
bool replace(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);
bool replaceAll(const std::string& searchTerm, const std::string& replacementText, bool caseSensitive = true);
```

## Selection System

### Selection Unit Enum

The `SelectionUnit` enum class defines the different semantic units used for selection operations:

```cpp
enum class SelectionUnit {
    Character,   // Individual characters
    Word,        // Words (alphanumeric + underscore)
    Expression,  // Expressions (function calls, parenthesized expressions)
    Line,        // Entire lines
    Paragraph,   // Groups of consecutive non-empty lines
    Block,       // Code blocks (typically in curly braces)
    Document     // Entire document
};
```

### Basic Selection Operations

```cpp
// Selection state
bool hasSelection() const;
void clearSelection();
void setSelectionRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
std::string getSelectedText() const;

// Selection coordinates
size_t getSelectionStartLine() const;
size_t getSelectionStartCol() const;
size_t getSelectionEndLine() const;
size_t getSelectionEndCol() const;

// Selection creation/modification
void startSelection();
void updateSelection();
void replaceSelection(const std::string& text);
```

### Direct Selection Commands

```cpp
void selectLine();
void selectAll();
void selectToLineStart();
void selectToLineEnd();
void selectWord();
```

### Semantic Selection Operations

```cpp
// Core expansion/shrinking methods
void expandSelection(SelectionUnit targetUnit = SelectionUnit::Word);
void shrinkSelection(SelectionUnit targetUnit = SelectionUnit::Character);
SelectionUnit getCurrentSelectionUnit() const;

// Specific expansion methods (called by expandSelection)
bool expandToWord();
bool expandToLine();
bool expandToExpression();
bool expandToParagraph(); 
bool expandToBlock();
bool expandToDocument();

// Specific shrinking methods (called by shrinkSelection)
bool shrinkToCharacter();
bool shrinkToWord();
bool shrinkFromLineToWord();
bool shrinkFromExpressionToWord();
bool shrinkFromParagraphToLine();
bool shrinkFromBlockToLine();
bool shrinkFromDocumentToParagraph();
bool shrinkNestedExpression();
```

### Selection Helper Methods

```cpp
// Position utilities
struct Position {
    size_t line;
    size_t column;
};

// Helper methods for finding semantic units
std::pair<size_t, size_t> findWordBoundaries(size_t line, size_t col) const;
char getMatchingBracket(char bracket) const;
bool isOpeningBracket(char c) const;
bool isClosingBracket(char c) const;
bool isQuoteChar(char c) const;
bool isWordChar(char c) const;

// Expression boundary tracking
struct ExpressionBoundary {
    Position start;
    Position end;
    bool found;
};

// Methods for finding expressions and blocks
ExpressionBoundary findEnclosingExpression(const Position& startPos, const Position& endPos) const;
ExpressionBoundary findMatchingBracketPair(const Position& pos, char openBracket, char closeBracket) const;
ExpressionBoundary findEnclosingQuotes(const Position& pos, char quoteChar) const;
ExpressionBoundary findEnclosingBracePair(const Position& startPos, const Position& endPos) const;
```

### Clipboard Operations

```cpp
void cutSelection();
void copySelection();
void pasteAtCursor();
std::string getClipboardText() const;
void setClipboardText(const std::string& text);
```

## Syntax Highlighting

```cpp
void enableSyntaxHighlighting(bool enable = true);
bool isSyntaxHighlightingEnabled() const;
std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const;
void detectAndSetHighlighter();
std::shared_ptr<SyntaxHighlighter> getCurrentHighlighter() const;
```

## Direct Buffer Manipulation

These methods are primarily used internally or by commands:

```cpp
void directDeleteTextRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
void directInsertText(size_t line, size_t col, const std::string& text, size_t& outEndLine, size_t& outEndCol);
```

## Usage Examples

### Basic Editing

```cpp
// Create an editor instance
Editor editor;

// Set up some content
editor.addLine("Hello, world!");
editor.addLine("This is a test.");

// Position cursor and type text
editor.setCursor(0, 7);  // After "Hello,"
editor.typeText(" beautiful");

// Result: "Hello, beautiful world!"
```

### Using Selection Units

```cpp
// Start with text "The quick brown fox jumps over the lazy dog."
editor.setCursor(0, 10);  // Cursor at 'o' in "brown"

// Expand to word
editor.expandSelection(SelectionUnit::Word);
// Selection is now "brown"

// Replace the selection
editor.replaceSelection("red");
// Text is now "The quick red fox jumps over the lazy dog."

// Select the whole line
editor.expandSelection(SelectionUnit::Line);
// Entire line is selected

// Shrink selection to a word
editor.shrinkSelection();
// Selection might be "red" or another word in the line
```

### Working with Code Blocks

```cpp
// Given code like:
// void function() {
//     int x = 10;
//     int y = 20;
// }

editor.setCursor(1, 5);  // Inside the block

// Expand to the entire block
editor.expandSelection(SelectionUnit::Block);
// Selection is now the entire block including braces

// Indent the selected block
editor.increaseIndent();
// Block is now indented by 4 spaces
```

See [SELECTION_UNITS.md](SELECTION_UNITS.md) for more detailed examples and usage patterns.

## OpenAI API Client Architecture

The AI-First TextEditor integrates with OpenAI's API through a modular, testable architecture.

### Core Interface

```cpp
// Interface for OpenAI API client implementations
class IOpenAI_API_Client {
public:
    virtual ~IOpenAI_API_Client() = default;
    
    // API request functions
    virtual ApiResponse sendChatCompletion(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& toolDefinitions = {}
    ) = 0;
    
    virtual ApiResponse streamChatCompletion(
        const std::vector<ApiChatMessage>& messages,
        const std::function<void(const std::string&)>& onContentCallback,
        const std::function<void(const ApiToolCall&)>& onToolCallCallback,
        const std::vector<ApiToolDefinition>& toolDefinitions = {}
    ) = 0;
};
```

### Data Types

The API uses several structured data types defined in `OpenAI_API_Client_types.h`:

```cpp
// Chat message structure for API requests
struct ApiChatMessage {
    std::string role;
    std::string content;
    std::optional<std::vector<ApiToolCall>> tool_calls;
    std::optional<std::string> tool_call_id;
    std::optional<std::string> name;
    // ...
};

// Tool parameter definition
struct ApiFunctionParameter {
    std::string name;
    std::string type;
    std::string description;
    bool required;
    // ...
};

// Tool/function definition
struct ApiToolDefinition {
    std::string type;
    std::string function_name;
    std::string description;
    std::vector<ApiFunctionParameter> parameters;
    // ...
};

// Tool call representation
struct ApiToolCall {
    std::string id;
    std::string type;
    std::string function_name;
    std::string function_args;
    // ...
};

// Structured API response
struct ApiResponse {
    std::string id;
    std::string model;
    std::string content;
    std::vector<ApiToolCall> tool_calls;
    bool success;
    std::string error_message;
    // ...
};
```

### Concrete Implementation

```cpp
// Production implementation of the OpenAI API client interface
class OpenAI_API_Client : public IOpenAI_API_Client {
public:
    // Constructor with API key configuration
    OpenAI_API_Client(const std::string& apiKey, const std::string& model = "gpt-4o");
    
    // Destructor handles cleanup of internal implementation
    ~OpenAI_API_Client();
    
    // Synchronous chat completion
    ApiResponse sendChatCompletion(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& toolDefinitions = {}
    ) override;
    
    // Streaming chat completion with callbacks
    ApiResponse streamChatCompletion(
        const std::vector<ApiChatMessage>& messages,
        const std::function<void(const std::string&)>& onContentCallback,
        const std::function<void(const ApiToolCall&)>& onToolCallCallback,
        const std::vector<ApiToolDefinition>& toolDefinitions = {}
    ) override;
};
```

### Mock Implementation

For testing purposes, the application includes a mock implementation:

```cpp
// Mock implementation for testing and development
class MockOpenAI_API_Client : public IOpenAI_API_Client {
public:
    MockOpenAI_API_Client();
    
    // Configure mock responses
    void setNextResponse(const ApiResponse& response);
    void setNextResponses(const std::vector<ApiResponse>& responses);
    
    // Record calls for verification
    size_t getChatCompletionCallCount() const;
    size_t getStreamChatCompletionCallCount() const;
    std::vector<std::vector<ApiChatMessage>> getRecordedMessages() const;
    
    // Interface implementation with configurable responses
    ApiResponse sendChatCompletion(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& toolDefinitions = {}
    ) override;
    
    ApiResponse streamChatCompletion(
        const std::vector<ApiChatMessage>& messages,
        const std::function<void(const std::string&)>& onContentCallback,
        const std::function<void(const ApiToolCall&)>& onToolCallCallback,
        const std::vector<ApiToolDefinition>& toolDefinitions = {}
    ) override;
};
```

### Usage Example

```cpp
// Creating a production client
auto apiClient = std::make_unique<OpenAI_API_Client>("your_api_key", "gpt-4o");

// Creating a mock client for testing
auto mockClient = std::make_unique<MockOpenAI_API_Client>();
mockClient->setNextResponse({
    .id = "mock-response-123",
    .model = "gpt-4o-mock",
    .content = "This is a mock response",
    .success = true
});

// Using dependency injection with AIAgentOrchestrator
AIAgentOrchestrator orchestrator(std::move(apiClient), uiModel);

// Making a chat completion request
std::vector<ApiChatMessage> messages = {
    {.role = "system", .content = "You are a helpful assistant."},
    {.role = "user", .content = "Hello, can you help me?"}
};

auto response = apiClient->sendChatCompletion(messages);
if (response.success) {
    std::cout << "AI response: " << response.content << std::endl;
}
``` 