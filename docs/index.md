# AI-First TextEditor Documentation

Welcome to the AI-First TextEditor documentation. This guide will help you get started with building, using, and contributing to the project.

## Getting Started

- [Project Overview](project/project_overview.md) - Comprehensive introduction to the project
- [Getting Started Guide](project/getting_started.md) - First-time setup and basic usage
- [Troubleshooting](development/troubleshooting.md) - Common issues and solutions

## Development

- [Development Guide](development/) - Comprehensive development documentation
  - [Architecture](development/ARCHITECTURE.md) - System design and components
  - [Codebase Structure](development/CODEBASE_STRUCTURE.md) - Project organization
  - [Contributing](development/CONTRIBUTING.md) - How to contribute
  - [Build Optimization](development/BUILD_OPTIMIZATION.md) - Optimizing build performance
  - [Debugging](development/RAPID_DEBUGGING.md) - Debugging techniques

## Features

- [AI Integration](features/ai_integration.md) - AI capabilities implementation
- [Async Logging](features/async_logging.md) - High-performance logging system
- [Context Gathering](features/ContextGathering.md) - How the editor gathers context
- [Llama Provider](features/LlamaProvider.md) - Local model integration

## Testing

- [Test Plan](testing/TEST_PLAN.md) - Testing strategy and approach
- [Test Implementation](testing/IMPLEMENTATION.md) - How tests are implemented
- [Performance Testing](testing/performance_testing.md) - Performance benchmarks
- [Test Automation](testing/test_automation_summary.md) - Automated testing infrastructure

## Project Management

- [Roadmap](project/ROADMAP.md) - Development phases and milestones
- [Progress](project/PROGRESS.md) - Current status and achievements
- [Branch Protection](project/branch_protection.md) - Branch management policies

## Command Reference

### Build Commands

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

### Run Commands

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
- [Documentation Source](https://github.com/your-org/AI-First-TextEditor/tree/main/docs) - Edit these docs