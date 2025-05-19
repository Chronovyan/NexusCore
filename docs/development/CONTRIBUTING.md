# Contributing to AI-First TextEditor

Thank you for your interest in contributing to the AI-First TextEditor project! This document provides guidelines and instructions for contributing.

## Getting Started

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake 3.15+
- Git
- OpenAI API key (for AI features)

### Setting Up the Development Environment

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/AI-First-TextEditor.git
   cd AI-First-TextEditor
   ```

2. **Initialize submodules**
   ```bash
   git submodule update --init --recursive
   ```

3. **Configure with CMake**
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

4. **Build the project**
   ```bash
   cmake --build .
   ```

5. **Run the tests**
   ```bash
   ctest
   ```

## Development Workflow

### Branching Strategy

We use a modified Git Flow approach:

- `master` - Production-ready code
- `develop` - Integration branch for feature development
- `feature/*` - New features
- `bugfix/*` - Bug fixes
- `refactor/*` - Code refactoring without changing functionality
- `test/*` - Adding or updating tests

### Creating a Feature

1. Create a new branch from `develop`:
   ```bash
   git checkout develop
   git pull
   git checkout -b feature/your-feature-name
   ```

2. Implement your changes, following the coding standards.

3. Write tests for your changes:
   ```bash
   # Create a new test file if needed
   touch tests/your_feature_test.cpp
   ```

4. Make sure all tests pass:
   ```bash
   cd build
   cmake --build . --target RunAllTests
   ./tests/RunAllTests
   ```

5. Submit a pull request to the `develop` branch.

### Pull Request Process

1. Ensure your code follows the coding standards
2. Update documentation if necessary
3. Make sure all tests pass
4. Request review from at least one maintainer
5. Address review comments and update your PR
6. Once approved, your PR will be merged

## Coding Standards

### C++ Guidelines

- Follow C++17 standards
- Use modern C++ features appropriately
- Use smart pointers instead of raw pointers
- Follow RAII principles
- Make use of const correctness

### Naming Conventions

- **Classes and Structs**: PascalCase (e.g., `TextBuffer`, `EditorCommand`)
- **Functions and Methods**: camelCase (e.g., `insertText()`, `getLineCount()`)
- **Variables**: camelCase (e.g., `lineCount`, `currentPosition`)
- **Constants and Enums**: UPPER_CASE (e.g., `MAX_LINE_LENGTH`, `DEFAULT_BUFFER_SIZE`)
- **Private Class Members**: camelCase with trailing underscore (e.g., `buffer_`, `lineCount_`)
- **File Names**: snake_case (e.g., `text_buffer.cpp`, `editor_commands.h`)

### Code Organization

- One class per file (with exceptions for small, closely related classes)
- Use namespaces to organize code
- Group related functions and classes together
- Separate interface from implementation

### Documentation

- Add Doxygen-style comments for all public interfaces
- Include usage examples for complex functionality
- Keep comments up-to-date with code changes
- Document non-obvious behavior and design decisions

Example:
```cpp
/**
 * @brief Inserts a line of text at the specified position
 *
 * This method inserts a new line at the given index and shifts
 * all subsequent lines down by one position.
 *
 * @param index The position to insert the line (0-based)
 * @param text The text content to insert
 * @throw TextBufferException if index is out of range
 *
 * @example
 * TextBuffer buffer;
 * buffer.insertLine(0, "First line");
 * buffer.insertLine(1, "Second line");
 */
void insertLine(size_t index, const std::string& text);
```

## Testing Guidelines

### Test Coverage

- Aim for at least 85% code coverage
- Test both normal operation and error conditions
- Include edge cases in your tests
- Test public interfaces thoroughly

### Test Organization

- Group tests by component
- Use descriptive test names
- Use test fixtures for common setup
- Follow the Arrange-Act-Assert pattern

Example:
```cpp
TEST_F(TextBufferTest, InsertLineInsertsTextAtCorrectPosition) {
    // Arrange
    TextBuffer buffer;
    buffer.insertLine(0, "First line");
    
    // Act
    buffer.insertLine(1, "Second line");
    
    // Assert
    EXPECT_EQ(buffer.getLine(0), "First line");
    EXPECT_EQ(buffer.getLine(1), "Second line");
    EXPECT_EQ(buffer.getLineCount(), 2);
}
```

## License

By contributing to this project, you agree that your contributions will be licensed under the project's license (see LICENSE file).

## Communication

- **Issues**: Use GitHub issues for bug reports and feature requests
- **Pull Requests**: Use GitHub pull requests for submitting changes
- **Discussions**: Use GitHub discussions for general questions and ideas

## Code Review

All submissions require review. We use GitHub pull requests for this purpose.

During code review, maintainers will check:
- Code quality and style
- Test coverage
- Documentation
- Performance considerations
- Design consistency with the project

## Recognition

Contributors will be recognized in the project's README and CONTRIBUTORS file.

Thank you for your contributions! 