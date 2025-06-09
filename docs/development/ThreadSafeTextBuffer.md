# ThreadSafeTextBuffer

A thread-safe wrapper for TextBuffer that provides synchronized access to text buffer operations.

## Overview

`ThreadSafeTextBuffer` is a decorator that implements the `ITextBuffer` interface and wraps a standard `TextBuffer` instance. It ensures thread safety by using appropriate synchronization mechanisms (shared mutex) for all operations.

The class follows the Decorator design pattern, which allows it to:
1. Maintain full compatibility with the `ITextBuffer` interface
2. Add thread safety without modifying the original `TextBuffer` class
3. Allow gradual migration from non-thread-safe to thread-safe implementation

## Thread Safety Guarantees

`ThreadSafeTextBuffer` provides the following thread safety guarantees:

1. **Method-level thread safety**: Each individual method call is thread-safe and atomic.
2. **Reader-writer synchronization**: Multiple readers can access the buffer concurrently, while writers have exclusive access.
3. **Atomic operations**: For operations that involve multiple method calls, explicit locking methods are provided.

## Usage

### Basic Usage

```cpp
// Create a thread-safe text buffer
ThreadSafeTextBuffer buffer;

// Use like a regular text buffer
buffer.addLine("Hello, world!");
buffer.insertLine(0, "First line");
std::string line = buffer.getLine(0);
```

### Thread-Safe Compound Operations

For operations that need to span multiple method calls atomically, use the explicit locking methods:

```cpp
// Thread-safe compound read operation
buffer.lockForReading();
try {
    size_t count = buffer.lineCount();
    for (size_t i = 0; i < count; i++) {
        // Process each line...
        const std::string& line = buffer.getLine(i);
        // ...
    }
    buffer.unlockReading();
} catch (...) {
    buffer.unlockReading();
    throw;
}

// Thread-safe compound write operation
buffer.lockForWriting();
try {
    buffer.clear(true);
    buffer.addLine("New line 1");
    buffer.addLine("New line 2");
    buffer.unlockWriting();
} catch (...) {
    buffer.unlockWriting();
    throw;
}
```

## Warning: References from getLine()

The `getLine()` method returns a reference to a line in the buffer. This reference is only guaranteed to be valid until the next modification of the buffer from any thread. Storing these references beyond a single operation is not thread-safe.

```cpp
// UNSAFE: Don't do this
const std::string& line = buffer.getLine(0);
// ... later in code, possibly in another thread
buffer.addLine("New line");
// line reference may now be invalid!

// SAFE: Use a copy if you need to store the line
std::string lineCopy = buffer.getLine(0);
```

## Performance Considerations

Using `ThreadSafeTextBuffer` adds some overhead due to mutex operations. Consider the following guidelines:

1. For read-heavy workloads, the performance impact is minimal due to the use of shared locks.
2. For write-heavy workloads, consider using explicit locking to batch multiple write operations.
3. If maximum performance is critical and thread safety is not needed in specific sections, you can access the underlying buffer directly using `getUnderlyingBuffer()`, but this bypasses all thread safety protections.

## Integration with Dependency Injection

To use `ThreadSafeTextBuffer` throughout your application, register it as the implementation of `ITextBuffer` in your dependency injection container:

```cpp
// Example with a typical DI container
container.registerType<ITextBuffer, ThreadSafeTextBuffer>();
```

This ensures that all components requesting an `ITextBuffer` will receive the thread-safe implementation.

## Thread-Safety Best Practices

When using `ThreadSafeTextBuffer`:

1. **Prefer individual method calls** for simple operations.
2. **Use explicit locking** for compound operations.
3. **Always use try/catch with explicit locks** to ensure the mutex is released even if an exception occurs.
4. **Be careful with references** returned from methods like `getLine()`.
5. **Minimize lock duration** to avoid contention.
6. **Consider using `getAllLines()`** to get a copy of all lines at once rather than accessing them individually. 