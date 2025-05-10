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