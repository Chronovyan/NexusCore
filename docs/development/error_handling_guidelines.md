# Error Handling Guidelines

This document outlines the standardized approach to error handling across the AI-First TextEditor codebase. Following these guidelines will ensure consistent, robust, and maintainable error handling throughout the application.

## 1. Error Handling Principles

1. **Fail Early, Fail Clearly**: Detect and handle errors as close to the source as possible
2. **Exceptions for Exceptional Conditions**: Use exceptions for truly exceptional cases, not for expected control flow
3. **Proper Resource Cleanup**: Ensure resources are properly released even when errors occur (RAII)
4. **Clear Error Messages**: Provide actionable, context-specific error messages
5. **Appropriate Error Propagation**: Choose the right mechanism to communicate errors up the call stack
6. **Recovery When Possible**: Design systems to recover from errors when feasible

## 2. Exception Hierarchy

The application uses a hierarchical exception system to categorize different types of errors:

```
EditorError (base class for all exceptions)
├── FileSystemError
│   ├── FileNotFoundError
│   ├── FileAccessError
│   └── FileWriteError
├── EditorOperationError
│   ├── InvalidPositionError
│   ├── InvalidSelectionError
│   └── CommandExecutionError
├── AIServiceError
│   ├── APIConnectionError
│   ├── AuthenticationError
│   ├── AIResponseError
│   └── RateLimitError
└── ApplicationError
    ├── ConfigurationError
    ├── ResourceAllocationError
    └── ConcurrencyError
```

## 3. When to Use Exceptions vs. Return Codes

### Use Exceptions For:

1. **Unrecoverable errors** that prevent the continuation of the current operation
2. **Resource allocation failures** that cannot be handled locally
3. **Initialization errors** that prevent proper object construction
4. **Invariant violations** that indicate programming errors
5. **I/O errors** such as file not found, permission denied, etc.

### Use Return Codes For:

1. **Expected failure conditions** that are part of normal operation
2. **Validation results** that indicate user input problems
3. **Performance-critical code paths** where exceptions would be expensive
4. **Low-level utility functions** that need to communicate success/failure

## 4. Exception Handling Best Practices

### Throwing Exceptions

```cpp
// Good: Throw with descriptive message and context
if (!file.open(filename)) {
    throw FileNotFoundError("Could not open file at path: " + filename);
}

// Good: Include relevant diagnostic information
if (position >= buffer.size()) {
    throw InvalidPositionError("Cursor position out of bounds", 
                               {{"position", std::to_string(position)}, 
                                {"buffer_size", std::to_string(buffer.size())}});
}
```

### Catching Exceptions

```cpp
// Good: Catch specific exceptions first, more general later
try {
    editor.loadFile(filename);
} catch (const FileNotFoundError& e) {
    // Handle specifically this error
    LOG_ERROR("File not found: " + std::string(e.what()));
    uiModel.displayErrorMessage("Could not find the file: " + filename);
} catch (const FileSystemError& e) {
    // Handle any file system error
    LOG_ERROR("File system error: " + std::string(e.what()));
    uiModel.displayErrorMessage("Could not access the file: " + filename);
} catch (const EditorError& e) {
    // Handle any editor error
    LOG_ERROR("Editor error: " + std::string(e.what()));
    uiModel.displayErrorMessage("Error opening file: " + e.what());
} catch (const std::exception& e) {
    // Handle standard exceptions (unexpected)
    LOG_ERROR("Unexpected error: " + std::string(e.what()));
    uiModel.displayErrorMessage("An unexpected error occurred");
} catch (...) {
    // Last resort fallback
    LOG_ERROR("Unknown error occurred while loading file");
    uiModel.displayErrorMessage("An unknown error occurred");
}
```

## 5. Return Code Pattern

For non-exceptional error reporting, use std::optional, std::variant, or dedicated error types:

```cpp
// Using std::optional for operations that might fail
std::optional<Document> tryLoadDocument(const std::string& path) {
    if (!fileExists(path)) {
        return std::nullopt;
    }
    // Load the document
    return Document(path);
}

// Using result type for more detailed error information
struct ErrorInfo {
    std::string errorCode;
    std::string message;
    std::map<std::string, std::string> details;
};

template<typename T>
class Result {
public:
    // Success constructor
    explicit Result(const T& value) : value_(value), success_(true) {}
    
    // Error constructor
    explicit Result(const ErrorInfo& error) : error_(error), success_(false) {}
    
    bool isSuccess() const { return success_; }
    const T& value() const { /* with appropriate error checking */ return value_; }
    const ErrorInfo& error() const { /* with appropriate error checking */ return error_; }
    
private:
    T value_;
    ErrorInfo error_;
    bool success_;
};

// Usage example
Result<int> validateUserInput(const std::string& input) {
    if (input.empty()) {
        return Result<int>(ErrorInfo{"EMPTY_INPUT", "Input cannot be empty", {}});
    }
    
    try {
        int value = std::stoi(input);
        if (value < 0) {
            return Result<int>(ErrorInfo{"NEGATIVE_VALUE", "Input must be positive", {{"value", std::to_string(value)}}});
        }
        return Result<int>(value);
    } catch (const std::exception& e) {
        return Result<int>(ErrorInfo{"INVALID_NUMBER", "Input must be a valid number", {{"error", e.what()}}});
    }
}
```

## 6. Resource Management and RAII

Always use RAII principles to ensure resources are properly cleaned up, even in the face of exceptions:

```cpp
// Good: Use smart pointers for memory management
std::unique_ptr<Buffer> createBuffer(size_t size) {
    return std::make_unique<Buffer>(size);
}

// Good: Use RAII wrapper classes for system resources
class FileHandle {
public:
    explicit FileHandle(const std::string& path, const std::string& mode) {
        file_ = std::fopen(path.c_str(), mode.c_str());
        if (!file_) {
            throw FileAccessError("Could not open file: " + path);
        }
    }
    
    ~FileHandle() {
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }
    
    // Delete copy operations
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    
    // Move operations
    FileHandle(FileHandle&& other) noexcept : file_(other.file_) {
        other.file_ = nullptr;
    }
    
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (file_) {
                std::fclose(file_);
            }
            file_ = other.file_;
            other.file_ = nullptr;
        }
        return *this;
    }
    
    // Accessor
    FILE* get() const { return file_; }
    
private:
    FILE* file_ = nullptr;
};
```

## 7. Logging and Error Reporting

Use our logging system consistently to record errors:

```cpp
// Error levels
LOG_DEBUG("Detailed information for debugging");
LOG_INFO("Informational messages about normal operation");
LOG_WARNING("Warning about potential issues");
LOG_ERROR("Error that prevents an operation but not the whole application");
LOG_CRITICAL("Critical error that may require application shutdown");

// Include context in logs
LOG_ERROR("Failed to save file: " + filename + ", error: " + e.what());
```

## 8. User-Facing Error Messages

Guidelines for displaying errors to users:

1. **Be Specific**: Tell the user exactly what went wrong
2. **Be Actionable**: Provide guidance on how to resolve the issue
3. **Be Concise**: Keep messages brief and to the point
4. **Be User-Focused**: Use non-technical language when possible
5. **Provide Error Codes**: Include error codes for support reference

Examples:

```cpp
// Good user error message
uiModel.displayErrorMessage(
    "Unable to save file: " + filename + "\n" +
    "Please check if you have write permissions or if the disk is full.\n" +
    "Error code: FS-103"
);

// Bad user error message
uiModel.displayErrorMessage(
    "Error 0x8007045D occurred while writing to " + filename
);
```

## 9. Thread Safety Considerations

When handling errors in multi-threaded contexts:

1. Ensure exception safety across thread boundaries
2. Use appropriate synchronization for error reporting
3. Consider using `std::future`/`std::promise` for propagating exceptions between threads
4. Implement thread-specific error handling where appropriate

## 10. Implementation Checklist

When implementing error handling in a component:

- [ ] Identify possible error conditions
- [ ] Choose appropriate error handling mechanism (exceptions or return codes)
- [ ] Define meaningful error types/codes
- [ ] Ensure proper resource cleanup (RAII)
- [ ] Add appropriate logging
- [ ] Create user-friendly error messages
- [ ] Write tests for error cases

## 11. Migration Strategy

For existing code:

1. Start with the most critical components
2. Update one component at a time
3. Add tests before making changes
4. Update all callers to handle the new error reporting mechanism
5. Document changes clearly in commit messages 