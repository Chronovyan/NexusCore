# AI-First TextEditor Documentation

Welcome to the AI-First TextEditor documentation. This guide will help you get started with building, using, and contributing to the project.

## Getting Started

- [Getting Started Guide](getting_started.md) - First-time setup and basic usage
- [Troubleshooting Guide](troubleshooting.md) - Common issues and solutions

## Architecture and Design

- [Codebase Structure](codebase_structure.md) - Overview of code organization
- [OpenAI Integration](openai_integration.md) - How AI capabilities are implemented

## Development

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

- [README](../README.md) - Project overview
- [GitHub Repository](https://github.com/your-org/AI-First-TextEditor) - Source code
- [Issue Tracker](https://github.com/your-org/AI-First-TextEditor/issues) - Report bugs and feature requests 