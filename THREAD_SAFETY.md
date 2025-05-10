# Thread Safety Patterns & Practices (C++17)

This document outlines key thread safety patterns and best practices implemented in the project, targeting an expert C++ audience.

## Core Thread Safety Patterns

### 1. Reader-Writer Locks (`std::shared_mutex`)

Utilize `std::shared_mutex` for resources where read operations are frequent and write operations are less common, allowing multiple concurrent readers but exclusive writer access.

```cpp
// Example: Protecting a shared resource
class SharedResource {
    mutable std::shared_mutex m_mutex; 
    Data m_data;
public:
    Data read() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_data;
    }
    void write(const Data& new_data) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_data = new_data;
    }
};
```

### 2. Atomic Operations & Memory Ordering

Use `std::atomic` for simple data types requiring thread-safe modifications without mutexes. Specify memory ordering explicitly where necessary (default is `std::memory_order_seq_cst`).

```cpp
std::atomic<bool> data_ready{false};
std::atomic<int> counter{0};

// Example: Signaling data readiness
data_ready.store(true, std::memory_order_release); // Ensure preceding writes are visible

// Example: Checking data readiness
if (data_ready.load(std::memory_order_acquire)) { // Ensure subsequent reads see the data
    // ... process data ...
}

// Key memory orders:
// - std::memory_order_relaxed: No synchronization or ordering constraints.
// - std::memory_order_acquire: Prevents reads/writes from being reordered before this load.
// - std::memory_order_release: Prevents reads/writes from being reordered after this store.
// - std::memory_order_acq_rel: Combines acquire and release semantics (for RMW operations).
// - std::memory_order_seq_cst: Strongest, guarantees sequential consistency.
```

### 3. Thread-Safe Singleton Initialization (Meyer's Singleton)

C++11 and later guarantee thread-safe initialization of function-local static variables.

```cpp
class SingletonService {
public:
    static SingletonService& instance() {
        static SingletonService service_instance; // Thread-safe initialization
        return service_instance;
    }
private:
    SingletonService() = default;
    // ... other members ...
};
```

### 4. Non-Recursive Locking Pattern

To prevent deadlocks from re-entrant calls on the same mutex, use private `_nolock` helper methods that assume the lock is already held by the public caller.

```cpp
class TaskManager {
    std::mutex m_task_mutex;
    void performTask_nolock(Task& task);
public:
    void processTask(Task& task) {
        std::lock_guard<std::mutex> lock(m_task_mutex);
        // ... preparation ...
        performTask_nolock(task);
        // ... cleanup ...
    }
};
```

### 5. Thread-Local Storage (`thread_local`)

For data that must be unique to each thread (e.g., per-thread caches, random number generators).

```cpp
// Example: Thread-local buffer or RNG
thread_local std::vector<char> thread_buffer(1024);
thread_local std::mt19937 thread_rng{std::random_device{}()};
```

### 6. Exception Safety in Threads

Ensure exceptions do not propagate out of a thread's top-level function. Catch exceptions, log them, and handle them appropriately (e.g., terminate thread, signal error).

```cpp
void thread_worker_function() {
    try {
        // ... thread operations ...
    } catch (const std::exception& e) {
        // Log error: e.what(), thread ID, etc.
        // Perform any necessary cleanup or signaling
    } catch (...) {
        // Log unknown error
    }
}
```

## Testing Thread Safety

- **Stress Testing**: High iteration counts, concurrent operations on shared resources.
- **Deadlock Detection**: Specific tests designed to provoke deadlocks (e.g., `DeadlockTest.cpp`).
- **Sanitizers**: Utilize ThreadSanitizer (TSan) in builds.
- **Resource Limiting Tests**: Ensure behavior under constrained resources.
- **Coordination Primitive Tests**: Verify correctness of atomics, condition variables, etc.

## Common Pitfalls & Considerations

- **Race Conditions**: Always protect shared, mutable data.
- **Deadlocks**: Consistent lock acquisition order; `std::scoped_lock` for multiple mutexes.
- **Livelocks**: Threads active but not making progress.
- **Data Races vs. Atomicity**: `std::atomic` guarantees freedom from data races for the atomic variable itself, not for larger operations.
- **False Sharing**: Cache line contention with unrelated atomic variables.
- **Memory Ordering**: Default to `seq_cst` if unsure, but understand costs. Profile for `relaxed` or `acq/rel` benefits.
- **Minimize Critical Sections**: Keep code under locks brief.

## Further Reading

1.  C++ Memory Model and Atomics: [cppreference.com/w/cpp/atomic](https://en.cppreference.com/w/cpp/atomic)
2.  C++ Thread Support Library: [cppreference.com/w/cpp/thread](https://en.cppreference.com/w/cpp/thread)
3.  "C++ Concurrency in Action" by Anthony Williams 