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
│   └── ...
├── tests/                  # Test files
│   ├── RunAllTests.cpp            # Main test runner
│   ├── OpenAIApiEndpointsTest.cpp # API endpoint tests
│   └── ...
├── external/               # Third-party dependencies
│   ├── cpr/                # HTTP client library
│   ├── json/               # JSON parsing (nlohmann/json)
│   └── googletest/         # Testing framework
└── CMakeLists.txt          # Main build configuration
```

## Key Components

### UI Layer

The UI is built using Dear ImGui and handles user input, rendering, and interaction with the editor components.

Key files:
- `src/main.cpp` - Application entry point
- `src/EditorUI.cpp` - Main editor UI components
- `src/UIModel.cpp` - UI state management

### AI Integration

The AI integration layer handles communication with OpenAI's API, processes responses, and coordinates tool calls.

Key files:
- `src/AIAgentOrchestrator.cpp` - Orchestrates AI interactions
- `src/OpenAI_API_Client.cpp` - API communication
- `src/MockOpenAI_API_Client.cpp` - Mock implementation for testing

### Editor Core

The editor core manages text editing, buffers, and workspace operations.

Key files:
- `src/TextBuffer.cpp` - Text buffer implementation
- `src/WorkspaceManager.cpp` - File system operations
- `src/EditorCommands.cpp` - Command pattern implementations for editing operations

### Test Framework

Tests are implemented using GoogleTest and organized by component.

Key files:
- `tests/RunAllTests.cpp` - Test runner
- `tests/AIAgentOrchestratorTest.cpp` - AI orchestrator tests
- `tests/OpenAIApiEndpointsTest.cpp` - API endpoint tests

## Class Relationships

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

## Build System

The project uses CMake for building, with the main configuration in `CMakeLists.txt`.

Key targets:
- `TextEditor` - Main application
- `RunAllTests` - Test runner
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
1. Define the feature interface in a header file in `src/`
2. Implement the feature in a corresponding cpp file
3. Add tests in `tests/`
4. Update the CMakeLists.txt if necessary
5. If the feature involves UI, update the appropriate UI files 