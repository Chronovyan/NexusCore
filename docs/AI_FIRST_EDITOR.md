# AI-First TextEditor

## Overview

The AI-First TextEditor reimagines the traditional coding experience by inverting the typical development workflow. Instead of the human writing code directly, the user provides high-level guidance and requirements while an AI agent handles the implementation details.

## Core Components

### 1. User Interface (UIModel.h, AITextEditorApp.cpp)

The application features a modern Dear ImGui-based interface with:

- **Conversation View**: A chat-like interface for natural language interaction with the AI
- **File Sidebar**: Displays project files with their current status
- **Chat Input Panel**: Multi-line input for user prompts and instructions
- **Global Status Display**: Shows the current state of the application

### 2. AI Integration (IOpenAI_API_Client.h, OpenAI_API_Client.h/cpp, MockOpenAI_API_Client.h/cpp)

Integration with OpenAI's API enables:

- **Modular API Client Architecture**: Interface-based design with concrete and mock implementations
- **Chat Completion Requests**: Communication with models like GPT-4o
- **Function Calling**: Allows the AI to call predefined tools to manipulate the codebase
- **Asynchronous Processing**: Handles API communication without blocking the UI
- **Testable Design**: Mock implementation facilitates unit testing without API dependencies

### 3. Development Pipeline (In Progress)

The system orchestrates a complete development workflow:

- **Project Generation**: Creates file structures based on user requirements
- **Code Generation**: Transforms user descriptions into executable C++ code
- **Compilation**: Builds the generated code using CMake
- **Testing**: Runs basic tests on the generated code
- **Execution**: Runs the compiled application

## Workflow

1. **Planning**: User describes the desired application or feature
2. **Preview**: AI proposes a project structure and implementation approach
3. **Generation**: AI creates the necessary files and code
4. **Compilation**: System compiles the generated code
5. **Testing**: Basic tests verify the generated code works as expected
6. **Execution**: The compiled application runs for user validation
7. **Refinement**: User provides feedback for improvements

## Current Status

The AI-First TextEditor is currently in active development with:

- ✅ Basic UI framework implemented
- ✅ OpenAI API client integration with interface-based design
- ✅ Mock OpenAI client for testing
- 🔄 AI agent orchestration system
- ⏳ Project generation from natural language
- ⏳ Compilation and execution pipeline

## Implementation Details

The current implementation leverages:

- **Dear ImGui**: For immediate-mode GUI rendering
- **GLFW**: For window management and OpenGL context
- **OpenAI API**: For AI-powered code generation and assistance
- **CMake**: For the build system of generated projects

## Future Enhancements

Planned improvements include:

- Enhanced error recovery and debugging tools
- Integrated visualization of code execution
- Support for multiple programming languages
- Collaborative editing capabilities
- Integration with version control systems 