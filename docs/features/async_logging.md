# Asynchronous Logging System

## Overview

The TextEditor includes a robust asynchronous logging system designed to provide high-performance logging capabilities without impacting the editor's responsiveness. This document details how to use, configure, and troubleshoot the async logging system.

Asynchronous logging works by queuing log messages in memory and processing them in a dedicated background thread. This approach offers several advantages:

- **Improved Performance**: Logging calls return quickly without waiting for I/O operations
- **Reduced Latency**: The main thread and UI remain responsive even during heavy logging
- **Configurable Behavior**: Fine-tune the trade-offs between reliability, memory usage, and performance

## Basic Usage

### Enabling Asynchronous Logging

To enable or disable asynchronous logging:

```cpp
// Enable async logging (logs will be processed in background)
ErrorReporter::enableAsyncLogging(true);

// Disable async logging (logs will be processed synchronously)
ErrorReporter::enableAsyncLogging(false);
```

When enabled, all log messages are queued and processed by a dedicated worker thread. When disabled, log messages are written directly to their destinations.

### Logging Messages

The API for logging messages remains unchanged whether async logging is enabled or not:

```cpp
// Log messages at various severity levels
ErrorReporter::logDebug("Debug information");
ErrorReporter::logWarning("Warning message");
ErrorReporter::logError("Error message");

// Or log an exception
try {
    // Some operation
} catch (const EditorException& ex) {
    ErrorReporter::logException(ex);
}
```

### Flushing Logs

To ensure all queued log messages are processed:

```cpp
// Flush all pending log messages
ErrorReporter::flushLogs();
```

This is particularly important before the application exits to ensure no log messages are lost.

## Queue Configuration

### Setting Queue Size and Overflow Policy

The async logging queue can be configured to handle different scenarios:

```cpp
// Configure queue with size limit of 1000 messages and default policy (DropOldest)
ErrorReporter::configureAsyncQueue(1000);

// Configure queue with specific overflow policy
ErrorReporter::configureAsyncQueue(1000, QueueOverflowPolicy::BlockProducer);

// Configure unlimited queue (may use more memory but won't lose messages)
ErrorReporter::configureAsyncQueue(0, QueueOverflowPolicy::WarnOnly);
```

### Overflow Policies

The `QueueOverflowPolicy` enum defines how the system handles situations where the queue reaches its maximum capacity:

| Policy | Description | Use Case |
|--------|-------------|----------|
| `DropOldest` | Removes the oldest message in the queue to make room for new ones | When newest messages are more important than history |
| `DropNewest` | Rejects new messages when the queue is full | When preserving historical context is critical |
| `BlockProducer` | Blocks the calling thread until space is available | When no messages can be lost, even at the cost of performance |
| `WarnOnly` | Logs warnings but allows the queue to grow beyond the configured limit | When memory is plentiful and no messages should be lost |

### Monitoring Queue Statistics

To monitor the performance and behavior of the async logging queue:

```cpp
// Get current statistics about the queue
AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();

// Access individual metrics
std::cout << "Current queue size: " << stats.currentQueueSize << std::endl;
std::cout << "High water mark: " << stats.highWaterMark << std::endl;
std::cout << "Overflow count: " << stats.overflowCount << std::endl;
```

The `AsyncQueueStats` struct provides the following information:

| Field | Description |
|-------|-------------|
| `currentQueueSize` | Current number of messages in the queue |
| `maxQueueSizeConfigured` | Maximum queue size as configured (0 = unbounded) |
| `highWaterMark` | Maximum queue size ever reached (peak memory usage) |
| `overflowCount` | Number of messages dropped or rejected due to overflow |
| `policy` | Current overflow policy in use |

## Design Considerations

### Thread Safety

The asynchronous logging system is designed to be thread-safe. All public methods can be called safely from any thread, and appropriate synchronization is used internally to protect shared state.

### Memory Usage

Async logging consumes memory proportional to the number of queued messages. Each message includes:
- The log message text
- Severity level
- Timestamp information

For high-volume logging scenarios, consider:
1. Setting an appropriate queue size limit
2. Choosing the right overflow policy based on your application's needs
3. Monitoring queue statistics to detect potential issues

### Performance Characteristics

Performance depends on several factors:

- **Queue Size**: Larger queues allow for more buffering but use more memory
- **Overflow Policy**: `BlockProducer` can impact performance if the queue fills up
- **Log Message Size**: Large messages require more memory in the queue
- **Log Destination Performance**: Slow destinations (e.g., network) may cause the queue to grow

## Best Practices

1. **Enable Async Logging for Performance-Critical Code Paths**:
   ```cpp
   // Enable async logging before performance-critical operations
   ErrorReporter::enableAsyncLogging(true);
   performCriticalOperation();
   ```

2. **Configure Appropriate Queue Size**:
   ```cpp
   // For most applications, a bounded queue prevents unbounded memory growth
   ErrorReporter::configureAsyncQueue(5000, QueueOverflowPolicy::DropOldest);
   ```

3. **Flush Logs at Important Points**:
   ```cpp
   // After significant operations or before application exit
   performImportantOperation();
   ErrorReporter::flushLogs();
   ```

4. **Monitor Queue Statistics During Development**:
   ```cpp
   // Periodically check queue statistics during testing
   AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();
   if (stats.highWaterMark > stats.maxQueueSizeConfigured * 0.8) {
       std::cout << "Warning: Queue approaching capacity" << std::endl;
   }
   ```

5. **Choose the Right Overflow Policy**:
   - Use `DropOldest` for general logging (default)
   - Use `BlockProducer` for critical logs that cannot be lost
   - Use `WarnOnly` during debugging or when memory is plentiful

## Troubleshooting

### Common Issues

1. **Lost Log Messages**:
   - Check if your queue size is sufficient
   - Verify that the chosen overflow policy is appropriate
   - Call `flushLogs()` before application exit

2. **Performance Problems**:
   - If using `BlockProducer`, consider if threads are being blocked
   - Check if log destinations are slow, causing queue buildup
   - Consider reducing logging verbosity

3. **High Memory Usage**:
   - Monitor `highWaterMark` to see maximum queue usage
   - Consider setting a lower `maxQueueSize`
   - Check if log messages are excessively large

### Debugging Tips

To help diagnose issues with async logging:

```cpp
// Get current queue statistics
AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();
std::cout << "Queue size: " << stats.currentQueueSize 
          << " / " << stats.maxQueueSizeConfigured << std::endl;
std::cout << "High water mark: " << stats.highWaterMark << std::endl;
std::cout << "Overflow count: " << stats.overflowCount << std::endl;

// Temporarily disable async logging to see immediate results
ErrorReporter::enableAsyncLogging(false);
ErrorReporter::logDebug("Test message (sync)");
```

## Implementation Details

The async logging system consists of several key components:

1. **Message Queue**: A thread-safe queue that holds pending log messages
2. **Worker Thread**: A dedicated thread that processes messages from the queue
3. **Synchronization Primitives**: Mutexes and condition variables that ensure thread safety
4. **Overflow Handling**: Logic to manage queue overflow according to the configured policy
5. **Statistics Tracking**: Counters for monitoring queue behavior and performance

When a log message is submitted:
1. If async logging is disabled, the message is written directly to log destinations
2. If async logging is enabled, the message is added to the queue according to the overflow policy
3. The worker thread is notified that a new message is available
4. The worker thread processes messages from the queue and writes them to all configured destinations

## Additional Resources

- `src/EditorError.h`: API documentation for the logging system
- `tests/AsyncLoggingTest.cpp`: Examples of async logging usage and test cases 