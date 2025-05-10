# C++ Text Editor Refinements

This document outlines specific improvements for enhancing safety, robustness, and code quality in our text editor project.

## 1. Replace `std::unique_lock` with `std::scoped_lock`

`std::scoped_lock` (C++17) is preferred for acquiring multiple locks as it prevents deadlocks by ensuring consistent lock order.

```cpp
// BEFORE:
std::unique_lock<std::shared_mutex> lock(mutex_);

// AFTER:
std::scoped_lock lock(mutex_); // For single mutex

// For multiple mutexes - deadlock-safe acquisition
// BEFORE: 
{
    std::unique_lock<std::mutex> lock1(mutex1);
    std::unique_lock<std::mutex> lock2(mutex2); // Potential deadlock hazard
}

// AFTER:
std::scoped_lock locks(mutex1, mutex2); // Atomic, deadlock-free acquisition
```

Use cases in our codebase:
- `SyntaxHighlightingManager::setHighlighter`
- `SyntaxHighlightingManager::setBuffer`
- `SyntaxHighlighterRegistry::registerHighlighter`

## 2. Standardize Error Handling

Create a consistent error handling pattern with appropriate levels of response.

```cpp
// Define error severity levels
enum class ErrorSeverity {
    Critical, // Program integrity threatened - log and terminate if necessary
    Major,    // Function cannot complete - log and return error value
    Minor     // Function can recover - log and continue
};

// Improved error handling function
template<typename ReturnT>
ReturnT handleError(const char* location, const std::exception& ex, ErrorSeverity severity, ReturnT defaultValue = {}) {
    std::string message = std::string("[") + location + "] " + ex.what();
    
    switch(severity) {
        case ErrorSeverity::Critical:
            std::cerr << "CRITICAL ERROR: " << message << std::endl;
            // Log to file
            // Consider program termination
            break;
        case ErrorSeverity::Major:
            std::cerr << "ERROR: " << message << std::endl;
            // Log to file
            break;
        case ErrorSeverity::Minor:
            std::cerr << "WARNING: " << message << std::endl;
            // Log to file with lower priority
            break;
    }
    
    return defaultValue;
}

// Usage example
try {
    // Operation that may throw
} catch (const std::exception& ex) {
    return handleError("SyntaxHighlighter::parse", ex, ErrorSeverity::Major, fallbackValue);
} catch (...) {
    return handleError("SyntaxHighlighter::parse", std::runtime_error("Unknown error"), 
                      ErrorSeverity::Critical, fallbackValue);
}
```

## 3. Add RAII Wrappers for Resource Management

Apply RAII pattern to all resources beyond mutex locks:

```cpp
// File resource RAII wrapper
class FileResource {
private:
    FILE* file_;
    
public:
    explicit FileResource(const char* filename, const char* mode) 
        : file_(fopen(filename, mode)) {
        if (!file_) {
            throw std::runtime_error("Could not open file");
        }
    }
    
    ~FileResource() {
        if (file_) {
            fclose(file_);
        }
    }
    
    // Deleted copy operations
    FileResource(const FileResource&) = delete;
    FileResource& operator=(const FileResource&) = delete;
    
    // Move operations
    FileResource(FileResource&& other) noexcept : file_(other.file_) {
        other.file_ = nullptr;
    }
    
    FileResource& operator=(FileResource&& other) noexcept {
        if (this != &other) {
            if (file_) fclose(file_);
            file_ = other.file_;
            other.file_ = nullptr;
        }
        return *this;
    }
    
    // Access to underlying resource
    FILE* get() const { return file_; }
    
    operator bool() const { return file_ != nullptr; }
};

// Usage
void processFile(const std::string& filename) {
    FileResource file(filename.c_str(), "r");
    // Use file.get() for operations
    // Automatically closed when function exits
}
```

## 4. Replace Raw Pointers with Smart Pointers

Use appropriate smart pointers based on ownership semantics:

```cpp
// BEFORE:
SyntaxHighlighter* highlighter_ = nullptr;

// AFTER (for owned resources):
std::unique_ptr<SyntaxHighlighter> highlighter_;

// AFTER (for shared resources):
std::shared_ptr<SyntaxHighlighter> highlighter_;

// AFTER (for non-owning references):
SyntaxHighlighter* highlighter_; // Keep raw, but document non-ownership
// OR
std::weak_ptr<SyntaxHighlighter> highlighter_; // If original is a shared_ptr
```

Apply to:
- `SyntaxHighlighterRegistry::highlighters_` (already using unique_ptr)
- Consider for `SyntaxHighlightingManager::buffer_` and `highlighter_`

## 5. Enhance Const-Correctness

Add const qualifiers to all methods that don't modify object state:

```cpp
// BEFORE:
std::vector<SyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) {
    // Implementation
}

// AFTER:
std::vector<SyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) const {
    // Implementation
}
```

Audit all member functions to verify they're marked const when appropriate:
- `SyntaxHighlighter::getLanguageName`
- `SyntaxHighlighter::getSupportedExtensions`
- Various getter methods throughout codebase

## 6. Adopt Lock-Free Approach for Hot Paths

Identify performance-critical sections and replace mutex locks with atomic operations:

```cpp
// BEFORE:
std::unique_lock<std::mutex> lock(cache_mutex_);
if (cache_.contains(key)) {
    return cache_[key];
}
// Compute value
cache_[key] = value;
return value;

// AFTER (example using atomic shared_ptr for cache entries):
struct CacheEntry {
    std::string value;
    std::atomic<bool> valid{true};
};

std::shared_ptr<CacheEntry> entry = cache_ptr_.load(std::memory_order_acquire);
if (entry && entry->valid.load(std::memory_order_acquire)) {
    return entry->value;
}

// Fallback to locked computation if needed
std::unique_lock<std::mutex> lock(cache_mutex_);
// Compute value
auto new_entry = std::make_shared<CacheEntry>();
new_entry->value = computed_value;
cache_ptr_.store(new_entry, std::memory_order_release);
return computed_value;
```

Apply to:
- `SyntaxHighlightingManager::getHighlightingStyles` - Avoid lock for cache hits
- `SyntaxHighlighterRegistry::getHighlighterForExtension` - Consider read optimizations

## 7. Add Thread Identification/Naming

Name threads and include thread IDs in logs for better debugging:

```cpp
// Thread creation with naming
std::thread worker([id = next_thread_id_++]() {
    // Set thread name for debugging
    #ifdef _WIN32
    std::stringstream name;
    name << "Worker" << id;
    SetThreadDescription(GetCurrentThread(), 
                        std::wstring(name.str().begin(), name.str().end()).c_str());
    #elif defined(__linux__)
    std::stringstream name;
    name << "Worker" << id;
    pthread_setname_np(pthread_self(), name.str().c_str());
    #endif
    
    // Thread work here
});

// Enhanced logging with thread IDs
void log(const std::string& message) {
    std::stringstream ss;
    ss << "[Thread " << std::this_thread::get_id() << "] " << message;
    std::cout << ss.str() << std::endl;
}
```

Apply to:
- `DeadlockTest.cpp` - Name test threads
- Enhance logging throughout with thread identifiers

## 8. Add Sanitizer Instrumentation

Update build scripts to include sanitizer flags for development builds:

### CMake Example:
```cmake
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

if(ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENABLE_UBSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
endif()
```

### Visual Studio:
For MSVC builds, update `build_deadlock_test.bat` to include appropriate flags:

```batch
REM Add thread sanitizer options when available
cl.exe /std:c++17 /EHsc /W4 /Zi /fsanitize=thread /O2 ^
    /Febin\DeadlockTest.exe DeadlockTest.cpp ^
    ... other files ...
```

## Implementation Priority

1. Standardize error handling (highest impact on robustness)
2. Replace raw pointers with smart pointers (memory safety)
3. Enhance const-correctness (correctness and optimization)
4. Add sanitizer instrumentation (catch bugs early)
5. Use std::scoped_lock (deadlock prevention)
6. Add thread identification/naming (debugging)
7. Add RAII wrappers (resource safety)
8. Adopt lock-free approach (performance optimization) 