# TextEditor (C++17)

A command-line C++17 text editor focused on stability, performance, and core editing features, now enhanced with AI capabilities for code generation and project management.

## Key Features & Status

- ✅ Core text buffer, cursor navigation, text manipulation (insert/delete)
- ✅ Selection management with semantic units (words, lines, expressions, blocks, etc.)
- ✅ Clipboard operations (copy/cut/paste) with robust memory management
- ✅ File I/O (open/save) with modification tracking
- ✅ Search & Replace with case sensitivity options
- ✅ Indentation management (increase/decrease)
- ✅ Undo/redo functionality with efficient command history
- ✅ Syntax highlighting for C++ (stable with ongoing optimizations)
- ✅ Dear ImGui-based user interface with conversation view, file sidebar, and status display
- ✅ OpenAI API integration with interface-based architecture for testing
- ✅ AI orchestration system managing conversation flow and development process
- ✅ Project generation from natural language descriptions
- 🔄 Compilation, testing, and execution pipeline for generated code

## AI-First TextEditor Capabilities

Our AI-First approach reimagines the editor as an environment where users provide high-level guidance while the AI handles implementation details:

- **Conversational Code Generation**: Describe your project in natural language, and the AI guides you through creating a fully functional C++ project.
- **Project Structure Planning**: The AI suggests a project structure with appropriate files and dependencies.
- **Iterative Development**: Provide feedback to refine the AI's approach before any code is generated.
- **File Generation**: AI generates source code, build files, and tests based on your specifications.
- **Testable Architecture**: Interface-based design enables mock-based testing without real API calls.

## Project Architecture

The project follows a clean, modular architecture:

- **Text Editor Core**: Foundational editing capabilities with robust text manipulation.
- **OpenAI API Integration**: 
  - Interface-based design (`IOpenAI_API_Client`) enabling real API calls and mock testing
  - Type definitions in separate header for clean dependency management
  - Full mock implementation for unit testing without API calls
  - CPR (C++ Requests) library for robust HTTP communications
- **AI Agent Orchestrator**: 
  - Manages conversation flow between user and AI
  - Processes tool calls for planning, code generation, and building
  - Tracks project state through file generation pipeline
- **Workspace Manager**: Handles file operations for AI-generated content
- **UI Layer**: Dear ImGui-based interface with chat, file browser, and status components

## Building & Running

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake (version 3.10+ recommended)
- OpenAI API key (for AI functionality)

### Build Instructions

Primary method (CMake):
```bash
# From project root
mkdir build && cd build
cmake ..
cmake --build . # Or: make / msbuild TextEditor.sln (depending on generator)
# Executable typically in: build/src/TextEditor, build/bin/TextEditor, or build/TextEditor
```
(For more detailed instructions, see [docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md).)

Alternative (Windows Batch for building and testing):
```batch
REM From project root - ensures clean build and runs tests
build_run_test.bat
REM Executable in: bin/TextEditor.exe (if produced by this script)
```
For just building with batch scripts, see `build.bat`.

### Running Tests

Using CTest (after CMake build, from `build` directory):
```bash
ctest -C Debug # Or Release, etc.
```
(For more detailed instructions, see [docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md).)

Alternative (Windows Batch for unit tests):
```batch
REM From project root
build_and_run_unit_tests.bat
```

## Testing the AI Features

The project includes a comprehensive testing framework for AI components:

- **Mock OpenAI API Client**: Test AI orchestration without making real API calls
- **Simulated Response Queue**: Prime the mock with tailored API responses
- **Request Inspection**: Verify the exact contents of API requests
- **Temporary Workspace**: Create and clean up test file directories automatically

Example test:
```cpp
// Create dependencies with mocks
UIModel uiModel;
MockOpenAI_API_Client mockApiClient;
WorkspaceManager workspaceManager("./test_dir");

// Prime the mock with specific response
mockApiClient.primeJsonResponse(jsonResponse, true);

// Test orchestrator behavior
AIAgentOrchestrator orchestrator(mockApiClient, uiModel, workspaceManager);
orchestrator.handleSubmitUserPrompt("Create a C++ calculator");

// Verify results
assert(mockApiClient.last_sent_messages_.size() > 0);
assert(orchestrator.getCurrentState() == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_FEEDBACK_ON_PLAN);
```

## GitHub Repository

The project is available on GitHub at: [https://github.com/your-username/AI-First-TextEditor](https://github.com/your-username/AI-First-TextEditor)

## Project Documentation

- **AI Integration:**
  - [`docs/AI_ARCHITECTURE.md`](docs/AI_ARCHITECTURE.md): Architecture of the AI components.
  - [`docs/TESTING_FRAMEWORK.md`](docs/TESTING_FRAMEWORK.md): Testing approach for AI components.
  - [`docs/MOCK_API_CLIENT.md`](docs/MOCK_API_CLIENT.md): Using the mock API client in tests.
  - [`docs/CPR_IMPLEMENTATION.md`](docs/CPR_IMPLEMENTATION.md): HTTP request implementation using CPR.
- **Core Principles & Practices:**
  - [`docs/STABILITY.md`](docs/STABILITY.md): Editor stability principles.
  - [`docs/THREAD_SAFETY.md`](docs/THREAD_SAFETY.md): Thread safety patterns.
  - [`docs/REFINEMENTS.md`](docs/REFINEMENTS.md): C++ code refinements and best practices.
  - [`docs/SYNTAX_HIGHLIGHTING_INVESTIGATION.md`](docs/SYNTAX_HIGHLIGHTING_INVESTIGATION.md): Technical investigation of the syntax highlighting manager.
  - [`docs/SYNTAX_HIGHLIGHTING_FIXES.md`](docs/SYNTAX_HIGHLIGHTING_FIXES.md): Recent fixes to the syntax highlighting system.
- **Development & Progress:**
  - [`ROADMAP.md`](ROADMAP.md): Project development roadmap.
  - [`docs/FUTURE_IMPROVEMENTS.md`](docs/FUTURE_IMPROVEMENTS.md): Ideas for future enhancements.

## License

[MIT License](LICENSE)