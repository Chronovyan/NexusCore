# AI-First TextEditor

A modern code editor with deep AI integration as a core principle.

## About

The AI-First TextEditor is designed from the ground up with AI capabilities integrated as first-class citizens, not merely add-ons. The editor leverages powerful language models to assist with code generation, refactoring, understanding, and more.

## Key Features

- Fast, responsive text editing with syntax highlighting
- Deep AI integration with the OpenAI API
- Asynchronous logging system for high performance
- Command-based architecture with undo/redo support
- Modern C++ (17) codebase with robust testing

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

## Documentation

We've organized our documentation to make it easy to find what you need:

### Project Overview

- [Project Introduction](docs/project/README.md) - Overview of the project goals and structure
- [Development Roadmap](docs/project/ROADMAP.md) - Our phased development plan
- [Implementation Progress](docs/project/PROGRESS.md) - Current status and achievements

### Developer Documentation

- [Architecture Overview](docs/development/ARCHITECTURE.md) - System design and component interactions
- [Codebase Structure](docs/development/CODEBASE_STRUCTURE.md) - Directory organization and key files
- [Contributing Guide](docs/development/CONTRIBUTING.md) - How to contribute to the project

### Feature Documentation

- [AI Integration](docs/features/ai_integration.md) - OpenAI API integration details
- [Async Logging](docs/features/async_logging.md) - Asynchronous logging system

### Testing Documentation

- [Test Plan](docs/testing/TEST_PLAN.md) - Test strategy and methodology
- [Test Implementation](docs/testing/IMPLEMENTATION.md) - Test coverage details
- [Stress Tests](docs/testing/stress_tests/file_logging.md) - File logging stress testing

## Current Status

The project is currently in **Phase 2: Comprehensive Testing & Validation**. We're focused on ensuring code quality and reliability through extensive testing before moving on to architectural refinements in Phase 3.

## Contributing

We welcome contributions! Please see our [Contributing Guide](docs/development/CONTRIBUTING.md) for more information on how to get involved.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 