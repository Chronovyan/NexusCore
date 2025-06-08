# Codebase Structure

This document outlines the key components and organization of the AI-First TextEditor codebase.

## Directory Structure

```
AI-First-TextEditor/
├── src/                    # Source code
│   ├── AIAgentOrchestrator.cpp   # AI interaction orchestration
│   ├── OpenAI_API_Client.cpp     # OpenAI API interaction
│   ├── WorkspaceManager.cpp      # File system operations
│   ├── UIModel.cpp               # User interface state
│   ├── TextBuffer.cpp            # Core text editing functionality
│   ├── Editor.cpp                # Main editor functionality
│   ├── EditorCommands.cpp        # Command pattern implementations
│   ├── EditorError.cpp           # Error handling and logging
│   └── ...
├── include/                # Public headers
│   └── ...
├── tests/                  # Test files
│   ├── RunAllTests.cpp            # Main test runner
│   ├── fuzz_testing.cpp           # Randomized input testing
│   ├── AsyncLoggingTest.cpp       # Async logging system tests
│   ├── cpp_highlighter_multiline_test.cpp # Syntax highlighting tests
│   └── ...
├── docs/                   # Documentation
│   ├── project/                   # Project planning and status
│   ├── features/                  # Feature documentation
│   ├── development/               # Developer guides
│   └── testing/                   # Testing documentation
├── external/               # Third-party dependencies
│   ├── cpr/                       # HTTP client library
│   ├── json/                      # JSON parsing (nlohmann/json)
│   └── googletest/                # Testing framework
└── CMakeLists.txt          # Main build configuration
```

## Key Components

### UI Layer

The UI is built using Dear ImGui and handles user input, rendering, and interaction with the editor components.

Key files:
- `src/main.cpp` - Application entry point
- `src/EditorUI.cpp` - Main editor UI components
- `src/UIModel.cpp` - UI state management
- `src/ImGuiWrapper.cpp` - ImGui integration utilities

### AI Integration

The AI integration layer handles communication with OpenAI's API, processes responses, and coordinates tool calls.

Key files:
- `src/AIAgentOrchestrator.cpp` - Orchestrates AI interactions
- `src/OpenAI_API_Client.cpp` - API communication
- `src/MockOpenAI_API_Client.cpp` - Mock implementation for testing
- `src/AIToolExecution.cpp` - Tool execution handling

### Editor Core

The editor core manages text editing, buffers, and workspace operations.

Key files:
- `src/TextBuffer.cpp` - Text buffer implementation
- `src/Editor.cpp` - Core editor functionality
- `src/WorkspaceManager.cpp` - File system operations
- `src/EditorCommands.cpp` - Command pattern implementations for editing operations

### Syntax Highlighting

The syntax highlighting system provides language-specific code coloring.

Key files:
- `src/SyntaxHighlighter.cpp` - Base highlighting functionality
- `src/CppHighlighter.cpp` - C++ specific highlighting
- `src/SyntaxHighlightingManager.cpp` - Highlighting management and caching
- `src/SyntaxHighlighterRegistry.cpp` - Registry for language highlighters

### Logging System

The logging system provides configurable, high-performance logging capabilities.

Key files:
- `src/EditorError.cpp` - Error handling and logging implementation
- `src/EditorError.h` - ErrorReporter class and logging interfaces

### Test Framework

Tests are implemented using GoogleTest and organized by component.

Key files:
- `tests/RunAllTests.cpp` - Test runner
- `tests/AIAgentOrchestratorTest.cpp` - AI orchestrator tests
- `tests/TextBufferTest.cpp` - Text buffer tests
- `tests/AsyncLoggingTest.cpp` - Async logging system tests
- `tests/fuzz_testing.cpp` - Randomized input testing

## Class Relationships

### Core Component Interactions

The core components of the editor interact as follows:

1. `Editor` - Central class that coordinates:
   - Contains a `TextBuffer` for text content
   - Manages a collection of `EditorCommand` objects
   - Coordinates with `SyntaxHighlightingManager` for code highlighting
   - Updates `UIModel` to reflect changes

2. `AIAgentOrchestrator` - Manages AI interactions:
   - Uses `OpenAI_API_Client` to communicate with the API
   - Updates `UIModel` for conversation state
   - Uses `WorkspaceManager` for file operations
   - Manages tool execution through `AIToolExecution`

3. `ErrorReporter` - Centralized logging and error handling:
   - Manages multiple `LogDestination` implementations
   - Provides synchronous and asynchronous logging
   - Tracks error statistics and retry operations

### AIAgentOrchestrator

The `AIAgentOrchestrator` class is the central component for AI integration. It:
- Uses `OpenAI_API_Client` to communicate with the API
- Updates `UIModel` to reflect changes in conversation state
- Uses `WorkspaceManager` to create and manage files
- Manages the state machine for AI conversation flow

```cpp
// Basic orchestrator flow
AIAgentOrchestrator orchestrator(apiClient, uiModel, workspaceManager);
orchestrator.handleSubmitUserPrompt("Create a C++ project");
// ... user reviews plan ...
orchestrator.handleSubmitUserFeedback("Looks good");
// ... AI generates file content ...
```

### OpenAI API Client

The `OpenAI_API_Client` handles communication with OpenAI's API, including authentication, request formatting, and response parsing.

```cpp
// API usage example
OpenAI_API_Client apiClient;
std::vector<ApiChatMessage> messages = { /* conversation history */ };
std::vector<ApiToolDefinition> tools = { /* available tools */ };
ApiResponse response = apiClient.createChatCompletion(messages, tools);
```

### WorkspaceManager

The `WorkspaceManager` handles file operations for the workspace, including creating, reading, and listing files.

```cpp
// WorkspaceManager example
WorkspaceManager workspace("./projects/my_project");
bool success = workspace.writeFile("main.cpp", "#include <iostream>\n...");
std::string content = workspace.readFile("main.cpp");
std::vector<std::string> files = workspace.listFiles();
```

### EditorCommands

The editor uses the Command pattern for editing operations, allowing for undo/redo functionality.

```cpp
// Command usage example
auto command = std::make_unique<InsertTextCommand>(editor, "Hello, World!", cursorPos);
editor.executeCommand(std::move(command));
// ... Later ...
editor.undo(); // Reverts the insertion
```

### ErrorReporter

The `ErrorReporter` provides a centralized logging and error handling system.

```cpp
// Configure and use the error reporter
ErrorReporter::enableFileLogging("logs/editor.log");
ErrorReporter::enableAsyncLogging(true);
ErrorReporter::logDebug("Initializing editor");

try {
    // Some operation
} catch (const EditorException& ex) {
    ErrorReporter::logException(ex);
}
```

## Build System

The project uses CMake for building, with the main configuration in `CMakeLists.txt`.

Key targets:
- `TextEditor` - Main application
- `RunAllTests` - Test runner
- `EditorLib` - Core library for editor functionality
- Individual test executables for specific components

## Third-Party Dependencies

| Library | Purpose | Integration |
|---------|---------|-------------|
| Dear ImGui | UI rendering | Included in source |
| nlohmann/json | JSON parsing | External submodule |
| cpr | HTTP client | External submodule |
| GoogleTest | Testing framework | External submodule |

## Adding New Features

When adding new features:
1. Define the feature interface in a header file in `include/`
2. Implement the feature in a corresponding cpp file in `src/`
3. Add tests in `tests/`
4. Update the CMakeLists.txt if necessary
5. If the feature involves UI, update the appropriate UI files
6. Document the feature in `docs/features/`

## Code Style Guidelines

The project follows these guidelines:
- Use camelCase for variables and methods
- Use PascalCase for class names
- Use underscores for file names (e.g., `text_buffer.cpp`)
- Use namespaces to organize code
- Use const-correctness throughout
- Prefer smart pointers over raw pointers
- Document all public interfaces with clear comments 