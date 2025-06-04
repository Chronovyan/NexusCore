# AI-First TextEditor: Project Overview

[![C++ Smoke Test](https://github.com/yourusername/AI-First-TextEditor/actions/workflows/smoke-test.yml/badge.svg)](https://github.com/yourusername/AI-First-TextEditor/actions/workflows/smoke-test.yml)

## Vision and Mission

The AI-First TextEditor is designed from the ground up with AI capabilities integrated as first-class citizens, not merely add-ons. Our mission is to create a modern, responsive, and intelligent text editing experience that leverages AI to enhance developer productivity.

We aim to create a text editor that:

1. **Makes AI a First-Class Citizen** - AI assistance is deeply integrated into the editor's core functionality
2. **Reduces Context Switching** - Developers can stay in their flow by keeping AI interactions within the editor
3. **Optimizes for Real-World Development** - Practical features that solve actual development challenges
4. **Maintains High Performance** - AI features do not compromise the core editing experience

## Key Features

### Core Editor Capabilities
- Fast, responsive text editing with syntax highlighting
- Modern UI with customizable themes and layouts
- Robust command system with comprehensive undo/redo support
- Efficient file I/O with support for large files

### AI Integration
- Deep integration with the OpenAI API
- Conversational interface for code generation and explanation
- Tool execution for real-world development tasks
- Context-aware assistance that understands your codebase
- Multi-model support for different AI capabilities (planned)

### Development Features
- Comprehensive asynchronous logging system for high performance
- Command-based architecture with undo/redo support
- Modern C++ (17) codebase with robust testing
- Plugin architecture for extensibility (planned)
- Collaborative editing capabilities (planned)

## Project Structure

The project is organized into the following main components:

- **Core Editor** - Text buffer, editing commands, cursor management
- **UI Layer** - User interface components built with Dear ImGui
- **AI Integration** - OpenAI API client, AI agent orchestration
- **File System** - Workspace management, file operations
- **Utilities** - Logging, error handling, performance monitoring

## Getting Started

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake 3.15+
- OpenAI API key (for AI features)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/AI-First-TextEditor.git
cd AI-First-TextEditor

# Initialize submodules
git submodule update --init --recursive

# Configure with CMake
mkdir build && cd build
cmake ..

# Build
cmake --build .

# Run the editor
./src/TextEditor
```

### Setting Up Git Hooks

We use git hooks to ensure code quality. To install the pre-commit hook that runs tests automatically:

```bash
# For Linux/macOS users
./hooks/install-hooks.sh

# For Windows users
powershell -ExecutionPolicy Bypass -File hooks/install-hooks.ps1
```

The pre-commit hook will build the project and run critical tests before allowing commits, ensuring code quality is maintained.

## Documentation Map

Navigate the project documentation using the following guide:

### Project Planning & Status
- [Development Roadmap](project/ROADMAP.md) - Phased development plan
- [Implementation Progress](project/PROGRESS.md) - Current status and achievements

### Developer Documentation
- [Architecture Overview](development/ARCHITECTURE.md) - System design and component interactions
- [Codebase Structure](development/CODEBASE_STRUCTURE.md) - Directory organization and key files
- [Contributing Guide](development/CONTRIBUTING.md) - How to contribute to the project

### Feature Documentation
- [AI Integration](features/ai_integration.md) - OpenAI API integration details
- [Async Logging](features/async_logging.md) - Asynchronous logging system

### Testing Documentation
- [Test Plan](testing/TEST_PLAN.md) - Test strategy and methodology
- [Test Implementation](testing/IMPLEMENTATION.md) - Test coverage details
- [Stress Tests](testing/stress_tests/file_logging.md) - File logging stress testing

## Current Status

The project is currently in **Phase 2: Comprehensive Testing & Validation**. We're focused on ensuring code quality and reliability through extensive testing before moving on to architectural refinements in Phase 3.

## Contributing

We welcome contributions! Please see our [Contributing Guide](development/CONTRIBUTING.md) for more information on how to get involved.

### Important Code Quality Requirements

- **All code must pass tests** before being committed or pushed
- Pre-commit hooks will automatically verify this for you
- CI checks must pass for pull requests to be merged
- If you need to bypass the pre-commit hook in rare circumstances, use `git commit --no-verify` (use sparingly and with good reason)

## License

This project is licensed under the MIT License - see the LICENSE file for details. 