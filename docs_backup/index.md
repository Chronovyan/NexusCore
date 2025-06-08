# AI-First TextEditor Documentation

Welcome to the AI-First TextEditor documentation. This guide will help you get started with building, using, and contributing to the project.

## Main Documentation

- [Project Overview](project_overview.md) - Comprehensive introduction to the project
- [Getting Started Guide](getting_started.md) - First-time setup and basic usage
- [Troubleshooting Guide](troubleshooting.md) - Common issues and solutions

## Architecture and Design

- [Architecture Overview](development/ARCHITECTURE.md) - System design and component interactions
- [Codebase Structure](development/CODEBASE_STRUCTURE.md) - Overview of code organization
- [OpenAI Integration](features/ai_integration.md) - How AI capabilities are implemented
- [Async Logging](features/async_logging.md) - High-performance logging system

## Project Planning & Status

- [Development Roadmap](project/ROADMAP.md) - Phased development plan
- [Implementation Progress](project/PROGRESS.md) - Current status and achievements

## Development

- [Contributing Guide](development/CONTRIBUTING.md) - How to contribute to the project
- [Testing Guide](testing_guide.md) - Running and writing tests

## Command Reference

Build commands:

```bash
# Windows (Visual Studio)
mkdir build_vs && cd build_vs
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release

# Linux/macOS
mkdir build && cd build
cmake ..
make
```

Run commands:

```bash
# Run the application
./TextEditor

# Run all tests
./tests/RunAllTests

# Run specific tests
./tests/RunAllTests --gtest_filter=OpenAIApiEndpointsTest*
```

## Quick Links

- [GitHub Repository](https://github.com/your-org/AI-First-TextEditor) - Source code
- [Issue Tracker](https://github.com/your-org/AI-First-TextEditor/issues) - Report bugs and feature requests 