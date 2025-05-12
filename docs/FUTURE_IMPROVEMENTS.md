# Potential Future Enhancements

This document outlines potential high-level directions for future development of the text editor, focusing on significant enhancements to its capabilities, performance, and maintainability.

## I. Core Editor & Feature Development

1.  **Advanced Text Editing Features:**
    *   Sophisticated cursor behaviors (e.g., multiple cursors, rectangular selection).
    *   Code-aware navigation (e.g., jump to definition, find references - potentially via LSP).
    *   Advanced search/replace (e.g., regex lookahead/lookbehind, multi-file search).

2.  **Plugin Architecture for Extensibility:**
    *   Design and implement a robust plugin system to allow third-party extensions (e.g., new language support, linters, formatters, custom tools).
    *   Requires a stable plugin API, discovery mechanism, and potentially sandboxing.

3.  **Enhanced Syntax Highlighting Engine:**
    *   Support for a wider range of programming languages (potentially via TextMate grammars or similar).
    *   Semantic highlighting (leveraging code analysis for more accurate tokenization).
    *   User-customizable themes and highlighting rules.

4.  **Basic Graphical User Interface (GUI):**
    *   Develop a simple, cross-platform GUI wrapper for the editor core.
    *   Focus on essential UI elements: text area, menus, file dialogs, status bar.

5.  **Workspace & Project Management:**
    *   Ability to open and manage folders as projects.
    *   Basic project-wide operations (e.g., search in project files).

## II. Performance & Stability

1.  **Advanced Performance Optimizations:**
    *   Explore lock-free algorithms or RCU (Read-Copy-Update) for critical data structures if profiling indicates contention.
    *   Memory optimization: custom allocators, object pooling for high-frequency objects, `std::string_view` adoption.
    *   Parallelize suitable operations (e.g., syntax highlighting of different regions, file loading/parsing).

2.  **Comprehensive Testing Strategies:**
    *   Implement property-based testing for core algorithms.
    *   Introduce fuzz testing to discover edge cases in parsing and input handling.
    *   Develop automated memory leak detection for long-running sessions.

3.  **Robust Error Recovery & Persistence:**
    *   Implement state recovery mechanisms for critical operations.
    *   Automatic backups during editing sessions and crash recovery for unsaved changes.

4.  **Integrated Performance Profiling & Diagnostics:**
    *   Embed instrumentation for performance measurement of key operations.
    *   Develop tools or visualizations for performance data and runtime diagnostics.
    *   Establish performance regression tests and configurable performance budgets.

## III. Code Quality, Build System, & Developer Experience

1.  **Continuous Code Quality & Modernization:**
    *   Proactively address compiler warnings and static analysis issues.
    *   Regularly review and refactor for `const`-correctness, smart pointer usage, and modern C++ idioms.
    *   Maintain comprehensive API documentation (Doxygen or similar) and architectural diagrams.

2.  **Build System & Dependency Management Modernization:**
    *   Ensure CMake practices remain current (e.g., target-based properties, modern find_package usage).
    *   Streamline dependency management (e.g., using CMake's FetchContent or a package manager like Conan/vcpkg if external dependencies grow).
    *   Full sanitizer support (ASan, TSan, UBSan, MSan) across all relevant build configurations and platforms.

3.  **Enhanced CI/CD Pipeline:**
    *   Automated performance benchmarking within CI.
    *   Integration of more advanced static analysis tools (e.g., Clang-Tidy, SonarQube) in the CI pipeline.
    *   Automated release packaging and deployment processes.

4.  **Structured Logging & Monitoring:**
    *   Implement a comprehensive structured logging framework throughout the codebase.
    *   Collect runtime performance metrics for monitoring and diagnostics.

## IV. Current Codebase Assessment & Refinement Focus

This section reflects a general assessment based on recent codebase review. It complements the future enhancements by highlighting current strengths to build upon and areas for ongoing refinement.

**Strengths:**

*   **Modular Design:** The project exhibits a good separation of concerns, with distinct components like `TextBuffer`, `Editor`, `CommandManager`, `SyntaxHighlighter`, and various command objects. This modularity is crucial for maintainability and scalability.
*   **Command Pattern for Undo/Redo:** The use of the command pattern for implementing editor operations and managing undo/redo functionality is a robust and effective approach.
*   **Commitment to Testing:** The presence of a `tests/` directory with a mix of Google Test and a custom test framework (`TestFramework.h`) demonstrates a commitment to ensuring code correctness. `RunAllTests.cpp` provides a centralized test runner.
*   **Modern C++ Practices:** The codebase generally aims to use C++17 features and practices.
*   **Custom Exception Handling:** The definition and use of custom exceptions (e.g., `EditorException`, `TextBufferException`) allow for more specific error reporting and handling.
*   **Basic Thread Safety Considerations:** The existence of `ThreadSafetyConfig.h` suggests an awareness of potential multi-threading, even if not extensively used yet.

**Areas for Ongoing Refinement & Focus:**

*   **Test Framework Consistency:** While comprehensive, standardizing on a single testing framework (likely Google Test, given its partial adoption) could simplify test development and maintenance. Ensure all test files are integrated into `RunAllTests.cpp` or the GTest main.
*   **Eliminate Remaining Commented-Out Code:** Continue efforts to remove or reintegrate any lingering commented-out blocks, especially old `main()` functions in test files.
*   **Replace `std::cout` for Debugging:** Transition from `std::cout` based debugging/logging to a more structured logging framework (as mentioned in III.4). This applies to debug messages in `src` and verbose test outputs.
*   **Address Large Files/Methods:** Some files (e.g., `src/Editor.cpp`, `src/EditorCommands.cpp`) and methods have grown quite large. Consider opportunities for further decomposition to improve readability and maintainability.
*   **Systematic Review of `TODO`s and `FIXME`s:** Proactively address any `TODO` or `FIXME` comments scattered throughout the codebase.
*   **Enhance `const`-Correctness:** Conduct a thorough review to ensure `const`-correctness is applied consistently, which can improve code safety and clarity.
*   **API Documentation (Doxygen):** Expand Doxygen comments for all public classes and methods to ensure comprehensive API documentation (linking to III.1).
*   **Integrated Performance Profiling:** Build upon `tests/PerformanceBenchmark.cpp` by integrating more robust and regular performance profiling (linking to II.4).
*   **Review Magic Numbers/Strings:** Identify and replace magic numbers and string literals with named constants or enums where appropriate for better readability and maintainability.
*   **Build System & Dependency Management:** Continuously review and modernize CMake usage. If external dependencies are expected to grow, formalize a strategy (e.g., FetchContent, Conan, vcpkg) (linking to III.2).
*   **Code Style Consistency:** While generally good, a pass with an automated formatter (e.g., ClangFormat) using a defined project style could ensure uniform code style.
*   **Completeness of Syntax Highlighting:** Ensure the `SyntaxHighlightingManager` and specific highlighters (e.g., for C++) are robust and cover edge cases.
*   **Error Reporting Granularity:** Review `EditorError.h` and its usage to ensure exceptions provide sufficient context for debugging.

#### Specific Architectural Refinements
(Details derived from C++ Code Refinements & Best Practices review)

##### Command Pattern Enhancements
*   **Consolidate Redundant Commands:**
    *   Review `DeleteTextCommand`, `DeleteForwardCommand`, and `DeleteCharCommand`. Consolidate into a single, robust `DeleteCharCommand(bool isBackspace)` and remove older/redundant versions from both `.h` and `.cpp` files to avoid confusion and simplify maintenance.
*   **Clarify Command Responsibilities:**
    *   `AddLineCommand`: Currently handles both appending a new line with text and splitting a line (like Enter key). This dual role is complex.
        *   Recommendation: Retain `AddLineCommand` solely for appending a line with optional text.
        *   Utilize the existing `NewLineCommand` for all line-splitting (Enter key) operations. This simplifies the logic for both commands.
*   **Improve Command Implementations:**
    *   `InsertTextCommand::undo()`: Currently deletes text character by character in a loop. For efficiency, especially with large pastes, consider adding a `TextBuffer::deleteRange(line, col, length)` or similar method and use it here.
    *   `CutCommand` Test Logic: **Critically, remove all hardcoded test-specific logic** from `CutCommand::execute` and `CutCommand::undo` in `EditorCommands.h`. Commands should implement generic logic, and unit tests should verify this generic behavior with appropriate test data.
*   **Configuration:**
    *   `CommandManager::maxHistory_`: Consider making this `static constexpr` or configurable at runtime if dynamic history limits are desired.

##### Syntax Highlighting Architecture
*   **Simplify `SyntaxHighlightingManager::highlighter_` Member:**
    *   The current type `std::shared_ptr<std::atomic<SyntaxHighlighter*>> highlighter_` is unconventional for its role.
    *   If `SyntaxHighlightingManager` is intended to share ownership of the `SyntaxHighlighter` object (which `setHighlighter` taking a `std::shared_ptr` suggests):
        *   Change member to: `std::shared_ptr<SyntaxHighlighter> highlighter_;`
        *   `setHighlighter` would then store this `shared_ptr`.
        *   `getHighlighter` would return a copy of this `shared_ptr` (or `const &` if appropriate).
    *   If `SyntaxHighlightingManager` is only an observer of an externally-managed highlighter:
        *   Change member to: `std::atomic<SyntaxHighlighter*> highlighter_{nullptr};`
        *   `setHighlighter` stores the raw pointer.
        *   `getHighlighter` can return the raw `SyntaxHighlighter*` (documenting lifetime expectations) or continue using the `std::shared_ptr` with a no-op deleter, but explicitly document this non-owning nature.
*   **Integrate Editor's Highlighting Cache:**
    *   `Editor.cpp` contains `highlightingStylesCache_` and related logic. This appears redundant with `SyntaxHighlightingManager`.
    *   Recommendation: `Editor` should delegate all highlighting requests and cache management to an instance of `SyntaxHighlightingManager`. Remove the direct caching fields and logic from `Editor.cpp`.
*   **Cache Eviction in `SyntaxHighlightingManager::cleanupCache()`:**
    *   The current `cleanupCache` marks entries as invalid (`valid.store(false)`). For significant memory savings with large files or long sessions, consider also `reset()`-ing the `std::shared_ptr<CacheEntry>` in `cachedStyles_` for evicted lines to allow the style data to be deallocated. This might require ensuring no other thread is actively reading that specific entry, or adjusting the `cachedStyles_` vector structure (e.g., removing nullptrs periodically).

##### API Clarity and Consistency
*   **Constants and Magic Numbers:**
    *   Replace magic numbers related to UI layout in `main.cpp` and `Editor.cpp` (e.g., `commandLineYOffset`, default display dimensions) with named `static constexpr` constants or configurable variables.
*   **Error Reporting Consolidation:**
    *   The error utilities in `EditorError.h` (`ErrorReporter`), `SyntaxHighlighter.h` (`handleError`, `logError`), and `SyntaxHighlightingManager.cpp` (`logManager`) have overlapping goals.
    *   Recommendation: Consolidate all error/exception logging through `ErrorReporter` from `EditorError.h`. The other utilities (`handleError`, `logManager`) can be refactored to use `ErrorReporter` internally. This provides a single, consistent logging style and point of control.

##### Build and Development Practices
*   **Reduce Header Includes:** Minimize includes in header files. For example, if `iostream` is used in `EditorCommands.h` only for debugging macros that are not always enabled, consider if it can be moved or if debugging output can be conditional.
*   **Implementation Hiding:** Move implementations of complex commands and non-trivial functions from header files (`EditorCommands.h`, potentially parts of `SyntaxHighlighter.h`) into their corresponding `.cpp` files. This improves encapsulation, reduces compilation dependencies, and can speed up build times.

## V. General C++ Refinements & Best Practices

This section outlines specific C++17 patterns and practices for enhancing safety, robustness, and code quality, derived from the "C++ Code Refinements & Best Practices" document.

### 1. Prefer `std::scoped_lock` for Multiple Mutexes

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

### 2. Standardized Error Handling Strategy

Implement a consistent error handling pattern using custom exception types and a central reporting mechanism. Our existing `EditorError.h` provides a good foundation.

```cpp
// Define specific error types (as in EditorError.h)
// class CriticalError : public std::runtime_error { using std::runtime_error::runtime_error; };
// class OperationError : public std::runtime_error { using std::runtime_error::runtime_error; };

// Conceptual central error logging/handler (e.g., ErrorReporter in EditorError.h)
// namespace ErrorHandler {
//     void log(const std::string& subsystem, const std::string& message, bool is_critical = false);
// }

// Usage example
void doSomethingComplex() {
    try {
        // ... operations that may throw ...
        if (/* condition for critical failure */) {
            // throw CriticalError("Critical subsystem failure description");
            ErrorReporter::reportError("MyModule", "Critical subsystem failure description", EditorException::Severity::Critical);
            // Potentially rethrow or initiate safe shutdown if ErrorReporter doesn't handle this
        }
        // ... more operations ...
    } catch (const EditorException& ex) { // Catch our specific exceptions
        ErrorReporter::reportError("MyModule", std::string("Critical: ") + ex.what(), ex.getSeverity());
        // Potentially rethrow or initiate safe shutdown based on severity
        if (ex.getSeverity() >= EditorException::Severity::Critical) throw; 
    } catch (const std::exception& ex) { // Catch standard exceptions as a fallback
        ErrorReporter::reportError("MyModule", std::string("Operation failed (std::exception): ") + ex.what(), EditorException::Severity::Error);
        // Return error code or default value
    }
}
```
This strategy should be consistently applied, leveraging `ErrorReporter` from `EditorError.h`.

### 3. RAII for All Resource Management

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
    try {
        FileHandle fh(filename.c_str(), "r");
        // Use fh.get() for file operations
        // fh automatically closes file on scope exit
    } catch (const std::runtime_error& e) {
        ErrorReporter::reportError("FileProcessing", e.what(), EditorException::Severity::Error);
    }
}
```

### 4. Explicit Ownership with Smart Pointers

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
- `SyntaxHighlightingManager::buffer_` and `highlighter_` (see also Specific Architectural Refinements for `highlighter_`).

### 5. Enforce `const`-Correctness

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

### 6. Judicious Use of Lock-Free Programming

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

### 7. Debugging: Thread Identification & Naming

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
        // Example for pthreads, adapt for actual native handle type if needed:
        // set_thread_name(pthread_self(), ss_name.str().c_str()); 
        
        // Thread work...
        // Log with thread ID
        // log_message_with_tid("Doing work in " + ss_name.str());
    });
}

// Enhanced logging (conceptual, integrate with ErrorReporter or dedicated logger)
// void log_message_with_tid(const std::string& msg) {
//     std::cout << "[TID:" << std::this_thread::get_id() << "] " << msg << std::endl;
// }
```
Ensure this is active in debug builds for facilities like `DeadlockTest.cpp` and integrated with the main logging mechanism.

### 9. Robustness and Edge Case Handling (Project Specific)

Apply to relevant modules like `Editor.cpp`, `TextBuffer.cpp`, and Commands.
- **Input Validation:**
    - Always check for empty input after `std::getline` before processing (e.g., in `main.cpp`, `Editor::run()`). Review all input points for similar checks.
    - Validate parameters to public API functions, especially indices, pointers, and string content where feasible.
- **Boundary Conditions:**
    - Thoroughly test operations at the beginning/end of lines, beginning/end of buffer, empty buffer, buffer with one line/char, etc.
    - Ensure cursor and selection logic correctly handles these boundary cases (e.g., `Editor::moveCursorUp` at first line).
- **Resource Limits:**
    - Consider behavior with extremely long lines or large number of lines. While not requiring arbitrary limits, understand potential performance cliffs.
    - Test undo/redo stack with maximum history.
- **File I/O:**
    - Handle cases like file not found, permission denied, disk full during save, empty files, files with unusual encodings (if broader support is added).
    - Ensure `Editor::saveFile` and `Editor::loadFile` report errors clearly. 