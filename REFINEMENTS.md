# C++ Code Refinements & Best Practices

This document outlines specific C++17 patterns and practices for enhancing safety, robustness, and code quality.

## 1. Prefer `std::scoped_lock` for Multiple Mutexes

For acquiring multiple locks, `std::scoped_lock` (C++17) offers deadlock-avoidance by enforcing a consistent lock order. For single mutexes, `std::lock_guard` is often sufficient, but `std::scoped_lock` also works and provides a consistent RAII wrapper.

```cpp
// Multiple mutexes - deadlock-safe acquisition
// BEFORE: Manual locking (order-dependent, risk of deadlock)
/*
{
    std::unique_lock<std::mutex> lock1(mutex1, std::defer_lock);
    std::unique_lock<std::mutex> lock2(mutex2, std::defer_lock);
    std::lock(lock1, lock2); // or manual ordered locking
}
*/
// AFTER:
std::scoped_lock locks(mutex1, mutex2); // Atomic, deadlock-free acquisition
```

Identified use cases for review (if multiple locks involved):
- `SyntaxHighlightingManager::setHighlighter`
- `SyntaxHighlightingManager::setBuffer`
- `SyntaxHighlighterRegistry::registerHighlighter`

## 2. Standardized Error Handling Strategy

Implement a consistent error handling pattern using custom exception types and a central reporting mechanism.

```cpp
// Define specific error types
class CriticalError : public std::runtime_error { using std::runtime_error::runtime_error; };
class OperationError : public std::runtime_error { using std::runtime_error::runtime_error; };

// Conceptual central error logging/handler
namespace ErrorHandler {
    void log(const std::string& subsystem, const std::string& message, bool is_critical = false);
}

// Usage example
void doSomethingComplex() {
    try {
        // ... operations that may throw ...
        if (/* condition for critical failure */) {
            throw CriticalError("Critical subsystem failure description");
        }
        // ... more operations ...
    } catch (const CriticalError& ex) {
        ErrorHandler::log("MyModule", std::string("Critical: ") + ex.what(), true);
        // Potentially rethrow or initiate safe shutdown
        throw; 
    } catch (const std::exception& ex) {
        ErrorHandler::log("MyModule", std::string("Operation failed: ") + ex.what());
        // Return error code or default value
    }
}
```

## 3. RAII for All Resource Management

Extend RAII beyond memory and mutexes to all system resources (files, handles, sockets, etc.).

```cpp
// Example: FILE* wrapper (ensure full Rule of 5/0 if used broadly)
class FileHandle {
    FILE* file_ = nullptr;
public:
    explicit FileHandle(const char* filename, const char* mode)
        : file_(fopen(filename, mode)) {
        if (!file_) throw std::runtime_error(std::string("Failed to open: ") + filename);
    }
    ~FileHandle() { if (file_) fclose(file_); }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& other) noexcept : file_(other.file_) { other.file_ = nullptr; }
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (file_) fclose(file_);
            file_ = other.file_; other.file_ = nullptr;
        }
        return *this;
    }
    FILE* get() const { return file_; }
    operator bool() const { return file_ != nullptr; }
};

// Usage
void processFile(const std::string& filename) {
    FileHandle fh(filename.c_str(), "r");
    if (!fh) return; // Or handle error
    // Use fh.get() for file operations
    // fh automatically closes file on scope exit
}
```

## 4. Explicit Ownership with Smart Pointers

Default to smart pointers to clearly express ownership semantics.

```cpp
// For exclusive, unique ownership:
std::unique_ptr<MyType> owned_object = std::make_unique<MyType>();

// For shared ownership (use when multiple objects co-own the resource):
std::shared_ptr<MyType> shared_object = std::make_shared<MyType>();

// For non-owning, potentially dangling references to shared_ptr-managed objects:
std::weak_ptr<MyType> weak_observer = shared_object;

// Raw pointers: Reserve for non-owning, non-lifetime-managing observation of objects 
// whose lifetime is guaranteed by other means (e.g., member of a class, stack variable).
// Document lifetime assumptions clearly.
MyType* non_owning_ptr = owned_object.get(); // Lifetime tied to owned_object
```

Review ownership for:
- `SyntaxHighlightingManager::buffer_` and `highlighter_`

## 5. Enforce `const`-Correctness

Mark all member functions that do not modify the logical state of the object as `const`.
This aids compiler optimizations, improves thread safety (for read-only access), and clarifies API contracts.

```cpp
class ImmutableData {
    std::string data_;
public:
    ImmutableData(std::string d) : data_(std::move(d)) {}
    const std::string& getData() const { return data_; } // Getter is const
    bool isEmpty() const { return data_.empty(); }      // Query is const
};
```

Audit for `const` opportunities in:
- `SyntaxHighlighter` methods (e.g., `getLanguageName`, `getSupportedExtensions`)
- All getter methods throughout the codebase.

## 6. Judicious Use of Lock-Free Programming

For true performance hot-spots identified *by profiling*, consider atomic operations or lock-free data structures. These are complex and error-prone; use with caution.

```cpp
// Example: Atomic flag for simple state
std::atomic<bool> is_ready_flag{false};

void setDataReady() {
    is_ready_flag.store(true, std::memory_order_release);
}

bool checkDataReady() {
    return is_ready_flag.load(std::memory_order_acquire);
}

// Complex lock-free structures (e.g., queues, hash maps) are typically sourced 
// from specialized libraries (e.g., TBB, Boost.Lockfree) or implemented with extreme care.
```

Consider for (after profiling):
- `SyntaxHighlightingManager::getHighlightingStyles` cache access.
- `SyntaxHighlighterRegistry::getHighlighterForExtension` if high contention.

## 7. Debugging: Thread Identification & Naming

For easier debugging of multi-threaded applications, name your threads and include thread IDs in logs.

```cpp
#include <thread>
#include <sstream>
// Platform-specific headers for thread naming (Windows.h, pthread.h)

void set_thread_name(std::thread::native_handle_type thread_handle, const char* name) {
    // Platform-specific implementation
    #ifdef _WIN32
    // SetThreadDescription requires Windows 10, version 1607
    // Consider older alternatives or wide char conversion if needed
    #elif defined(__linux__) || defined(__APPLE__)
    // pthread_setname_np varies by OS (e.g., on Linux it takes pthread_t, not native_handle_type directly without cast)
    // pthread_setname_np(thread_handle, name); // May need casting or std::thread::id conversion
    #endif
}

std::thread create_worker_thread(int id) {
    return std::thread([id]() {
        std::stringstream ss_name;
        ss_name << "Worker_" << id;
        // set_thread_name(pthread_self(), ss_name.str().c_str()); // Example for pthreads
        // Thread work...
    });
}

// Enhanced logging (conceptual)
void log_message(const std::string& msg) {
    std::cout << "[TID:" << std::this_thread::get_id() << "] " << msg << std::endl;
}
```
Ensure this is active in debug builds for facilities like `DeadlockTest.cpp`.

## 8. Codebase-Specific Architectural Refinements

This section details architectural and API improvements specific to this text editor project, derived from code review.

### 8.1. Command Pattern Enhancements
- **Consolidate Redundant Commands:**
    - Review `DeleteTextCommand`, `DeleteForwardCommand`, and `DeleteCharCommand`. Consolidate into a single, robust `DeleteCharCommand(bool isBackspace)` and remove older/redundant versions from both `.h` and `.cpp` files to avoid confusion and simplify maintenance.
- **Clarify Command Responsibilities:**
    - `AddLineCommand`: Currently handles both appending a new line with text and splitting a line (like Enter key). This dual role is complex.
        - Recommendation: Retain `AddLineCommand` solely for appending a line with optional text.
        - Utilize the existing `NewLineCommand` for all line-splitting (Enter key) operations. This simplifies the logic for both commands.
- **Improve Command Implementations:**
    - `InsertTextCommand::undo()`: Currently deletes text character by character in a loop. For efficiency, especially with large pastes, consider adding a `TextBuffer::deleteRange(line, col, length)` or similar method and use it here.
    - `CutCommand` Test Logic: **Critically, remove all hardcoded test-specific logic** from `CutCommand::execute` and `CutCommand::undo` in `EditorCommands.h`. Commands should implement generic logic, and unit tests should verify this generic behavior with appropriate test data.
- **Configuration:**
    - `CommandManager::maxHistory_`: Consider making this `static constexpr` or configurable at runtime if dynamic history limits are desired.

### 8.2. Syntax Highlighting Architecture
- **Simplify `SyntaxHighlightingManager::highlighter_` Member:**
    - The current type `std::shared_ptr<std::atomic<SyntaxHighlighter*>> highlighter_` is unconventional for its role.
    - If `SyntaxHighlightingManager` is intended to share ownership of the `SyntaxHighlighter` object (which `setHighlighter` taking a `std::shared_ptr` suggests):
        - Change member to: `std::shared_ptr<SyntaxHighlighter> highlighter_;`
        - `setHighlighter` would then store this `shared_ptr`.
        - `getHighlighter` would return a copy of this `shared_ptr` (or `const &` if appropriate).
    - If `SyntaxHighlightingManager` is only an observer of an externally-managed highlighter:
        - Change member to: `std::atomic<SyntaxHighlighter*> highlighter_{nullptr};`
        - `setHighlighter` stores the raw pointer.
        - `getHighlighter` can return the raw `SyntaxHighlighter*` (documenting lifetime expectations) or continue using the `std::shared_ptr` with a no-op deleter, but explicitly document this non-owning nature.
- **Integrate Editor's Highlighting Cache:**
    - `Editor.cpp` contains `highlightingStylesCache_` and related logic. This appears redundant with `SyntaxHighlightingManager`.
    - Recommendation: `Editor` should delegate all highlighting requests and cache management to an instance of `SyntaxHighlightingManager`. Remove the direct caching fields and logic from `Editor.cpp`.
- **Cache Eviction in `SyntaxHighlightingManager::cleanupCache()`:**
    - The current `cleanupCache` marks entries as invalid (`valid.store(false)`). For significant memory savings with large files or long sessions, consider also `reset()`-ing the `std::shared_ptr<CacheEntry>` in `cachedStyles_` for evicted lines to allow the style data to be deallocated. This might require ensuring no other thread is actively reading that specific entry, or adjusting the `cachedStyles_` vector structure (e.g., removing nullptrs periodically).

### 8.3. API Clarity and Consistency
- **Constants and Magic Numbers:**
    - Replace magic numbers related to UI layout in `main.cpp` and `Editor.cpp` (e.g., `commandLineYOffset`, default display dimensions) with named `static constexpr` constants or configurable variables.
- **Error Reporting Consolidation:**
    - The error utilities in `EditorError.h` (`ErrorReporter`), `SyntaxHighlighter.h` (`handleError`, `logError`), and `SyntaxHighlightingManager.cpp` (`logManager`) have overlapping goals.
    - Recommendation: Consolidate all error/exception logging through `ErrorReporter` from `EditorError.h`. The other utilities (`handleError`, `logManager`) can be refactored to use `ErrorReporter` internally. This provides a single, consistent logging style and point of control.

### 8.4. Build and Development Practices
- **Reduce Header Includes:** Minimize includes in header files. For example, if `iostream` is used in `EditorCommands.h` only for debugging macros that are not always enabled, consider if it can be moved or if debugging output can be conditional.
- **Implementation Hiding:** Move implementations of complex commands and non-trivial functions from header files (`EditorCommands.h`, potentially parts of `SyntaxHighlighter.h`) into their corresponding `.cpp` files. This improves encapsulation, reduces compilation dependencies, and can speed up build times.

## 9. Robustness and Edge Case Handling (Project Specific)

Apply to relevant modules like `Editor.cpp`, `TextBuffer.cpp`, and Commands.
- **Input Validation:**
    - Always check for empty input after `std::getline` before processing (e.g., in `main.cpp`, `Editor::run()`).
- **Arithmetic Safety:**
    - Prevent potential division by zero in `Editor::Editor()` when calculating `displayWidth_` and `displayHeight_` if `getTerminalWidth()`/`Height()` can return 0 or 1. Add checks and provide safe defaults.
- **`TextBuffer` Operations:**
    - Ensure all `TextBuffer` methods (`deleteLine`, `insertLine`, `joinLines`, `splitLine`, etc.) are robust against edge cases (empty buffer, invalid indices, operations at buffer/line boundaries).
    - Consistently use `TextBufferException` (or other appropriate `EditorException` derivatives) for reporting errors from `TextBuffer` operations.
- **Command Logic Safety:**
    - `InsertTextCommand::undo()`: As mentioned in 8.1, improve efficiency to avoid issues with very large undo operations.

## 10. Testing Strategy (Project Specific)

The current `src/EditorTest.cpp` is a basic driver program and should be expanded into a formal testing suite.

- **Adopt a Unit Testing Framework:**
    - Integrate a standard C++ unit testing framework such as GoogleTest or Catch2. This provides structure, assertions, test discovery, and clear reporting.
- **Comprehensive Test Coverage:**
    - **`TextBuffer`:** Test all methods with various inputs, including empty buffer, single/multiple lines, operations at start/middle/end of lines and buffer. Verify line content, line count, and cursor/selection state (if applicable via an `Editor` wrapper).
    - **Commands:** For each command:
        - Test `execute()`: Verify correct buffer modification, cursor positioning, selection changes, and clipboard state (for copy/cut).
        - Test `undo()`: Verify that `undo()` perfectly reverses the state changes made by `execute()`.
        - Test with edge cases (e.g., command on empty buffer, at boundaries).
    - **`CommandManager`:** Test undo/redo stack limits, correct sequencing of undo/redo for single and compound commands.
    - **`Editor` Facade:** Test `Editor` methods, potentially mocking or using test doubles for dependencies like file system or complex UI interactions if necessary. Test command dispatching.
    - **Syntax Highlighting:** Test `SyntaxHighlighter` implementations with various code snippets. Test `SyntaxHighlightingManager` caching, invalidation, and timeout logic.
    - **File I/O:** Test `Editor::openFile` and `Editor::saveFile` with valid and invalid paths, empty files, and different content. (May require file system interaction, plan accordingly).
- **Remove Test-Specific Code from Production Logic:**
    - As highlighted for `CutCommand`, ensure no production code paths are conditional on specific test string literals or hardcoded states. Tests must verify the generic, production behavior.
- **Use Assertions for Verification:**
    - Utilize the chosen framework's assertion macros (e.g., `ASSERT_EQ`, `EXPECT_TRUE`, `ASSERT_THROWS`) to automatically verify expected outcomes rather than relying on manual `std::cout` inspection.
- **Error Handling in Tests:**
    - Correct the `try-catch` block structure in the existing `EditorTest.cpp` (or its replacement test files) to properly catch and report exceptions.