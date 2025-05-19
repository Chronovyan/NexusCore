# Asynchronous Logging System

## Overview

The TextEditor includes a robust asynchronous logging system designed to provide high-performance logging capabilities without impacting the editor's responsiveness. This document details how to use, configure, and troubleshoot the async logging system.

Asynchronous logging works by queuing log messages in memory and processing them in a dedicated background thread. This approach offers several advantages:

- **Improved Performance**: Logging calls return quickly without waiting for I/O operations
- **Reduced Latency**: The main thread and UI remain responsive even during heavy logging
- **Configurable Behavior**: Fine-tune the trade-offs between reliability, memory usage, and performance
- **Memory Safety**: Bounded queue options prevent unchecked memory growth during high-volume logging

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

The first parameter (`maxQueueSize`) determines the maximum number of messages that can be held in the queue:
- When set to a positive number, it creates a bounded queue that limits memory usage
- When set to 0, it creates an unbounded queue that can grow as needed (limited only by available memory)

The second parameter (`overflowPolicy`) determines how the system handles queue overflow situations.

### Overflow Policies

The `QueueOverflowPolicy` enum defines how the system handles situations where the queue reaches its maximum capacity:

| Policy | Description | Use Case | Memory Impact | Performance Impact |
|--------|-------------|----------|---------------|-------------------|
| `DropOldest` | Removes the oldest message in the queue to make room for new ones | When newest messages are more important than history | Bounded | Minimal |
| `DropNewest` | Rejects new messages when the queue is full | When preserving historical context is critical | Bounded | Minimal |
| `BlockProducer` | Blocks the calling thread until space is available | When no messages can be lost, even at the cost of performance | Bounded | Can be significant if queue fills |
| `WarnOnly` | Logs warnings but allows the queue to grow beyond the configured limit | When memory is plentiful and no messages should be lost | Unbounded | Minimal |

#### Policy Behaviors in Detail

**DropOldest**:
```cpp
// When the queue reaches maxQueueSize, oldest messages are removed
// Implementation (simplified):
if (queue_.size() >= maxQueueSize_) {
    queue_.pop_front();  // Remove oldest message
    overflowCount_++;
    // Log warning about dropped message
}
```

**DropNewest**:
```cpp
// When the queue is full, new messages are discarded
// Implementation (simplified):
if (queue_.size() >= maxQueueSize_) {
    overflowCount_++;
    // Log warning about rejected message
    return;  // New message is not added to queue
}
```

**BlockProducer**:
```cpp
// When the queue is full, the thread blocks until space is available
// Implementation (simplified):
while (queue_.size() >= maxQueueSize_) {
    // Wait for queue to have space
    queueNotFullCV_.wait(lock);
}
// When a message is processed, signal waiting producers:
if (queueWasFull && queue_.size() < maxQueueSize_) {
    queueNotFullCV_.notify_all();
}
```

**WarnOnly**:
```cpp
// When the queue exceeds configured size, warnings are logged but all messages are kept
// Implementation (simplified):
if (maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_) {
    overflowCount_++;
    // Log warning about queue size
    // But still add the message to the queue
}
```

### Monitoring Queue Statistics

To monitor the performance and behavior of the async logging queue:

```cpp
// Get current statistics about the queue
AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();

// Access individual metrics
std::cout << "Current queue size: " << stats.currentQueueSize << std::endl;
std::cout << "High water mark: " << stats.highWaterMark << std::endl;
std::cout << "Overflow count: " << stats.overflowCount << std::endl;
std::cout << "Configuration: " << stats.maxQueueSizeConfigured 
          << " messages, policy: " << getPolicyName(stats.policy) << std::endl;

// Helper function to convert policy to string
std::string getPolicyName(QueueOverflowPolicy policy) {
    switch (policy) {
        case QueueOverflowPolicy::DropOldest: return "DropOldest";
        case QueueOverflowPolicy::DropNewest: return "DropNewest";
        case QueueOverflowPolicy::BlockProducer: return "BlockProducer";
        case QueueOverflowPolicy::WarnOnly: return "WarnOnly";
        default: return "Unknown";
    }
}
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

```cpp
// Thread synchronization primitives used:
static std::mutex queueMutex_;                   // Protects the message queue
static std::condition_variable queueCV_;         // Signals when messages are available
static std::condition_variable queueNotFullCV_;  // Signals when queue has space (for BlockProducer)
```

### Memory Usage

Async logging consumes memory proportional to the number of queued messages. Each message includes:
- The log message text
- Severity level
- Timestamp information

For high-volume logging scenarios, consider:
1. Setting an appropriate queue size limit
2. Choosing the right overflow policy based on your application's needs
3. Monitoring queue statistics to detect potential issues

#### Memory Usage Estimation

A rough estimation for queue memory usage:

```
Memory usage ≈ maxQueueSize * (avg_message_length + overhead)
```

Where:
- `maxQueueSize` is your configured queue limit
- `avg_message_length` is the average size of your log messages 
- `overhead` is approximately 30-50 bytes per message for metadata

### Performance Characteristics

Performance depends on several factors:

- **Queue Size**: Larger queues allow for more buffering but use more memory
- **Overflow Policy**: `BlockProducer` can impact performance if the queue fills up
- **Log Message Size**: Large messages require more memory in the queue
- **Log Destination Performance**: Slow destinations (e.g., network) may cause the queue to grow

#### Measuring Performance Impact

A simple benchmark to evaluate performance:

```cpp
// Setup
const int numMessages = 10000;
auto start = std::chrono::high_resolution_clock::now();

// Log with async enabled
ErrorReporter::enableAsyncLogging(true);
for (int i = 0; i < numMessages; i++) {
    ErrorReporter::logDebug("Test message " + std::to_string(i));
}
ErrorReporter::flushLogs();  // Wait for all messages to be processed

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
std::cout << "Async logging: " << duration << "ms for " << numMessages << " messages" << std::endl;
```

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
   // Select a queue size based on expected burst logging volume
   ErrorReporter::configureAsyncQueue(5000, QueueOverflowPolicy::DropOldest);
   ```

3. **Select the Right Policy for Critical Logs**:
   ```cpp
   // For critical error logging that must not be lost
   ErrorReporter::configureAsyncQueue(1000, QueueOverflowPolicy::BlockProducer);
   
   // For debugging with large volumes of data
   ErrorReporter::configureAsyncQueue(10000, QueueOverflowPolicy::DropOldest);
   ```

4. **Flush Logs at Important Points**:
   ```cpp
   // After significant operations or before application exit
   performImportantOperation();
   ErrorReporter::flushLogs();
   ```

5. **Monitor Queue Statistics During Development**:
   ```cpp
   // Periodically check queue statistics during testing
   AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();
   if (stats.highWaterMark > stats.maxQueueSizeConfigured * 0.8) {
       std::cout << "Warning: Queue approaching capacity" << std::endl;
   }
   if (stats.overflowCount > 0) {
       std::cout << "Warning: " << stats.overflowCount << " messages were dropped" << std::endl;
   }
   ```

6. **Adaptive Policy Selection**:
   ```cpp
   // Change policies based on application state
   void configureLoggingForPhase(ApplicationPhase phase) {
       switch (phase) {
           case ApplicationPhase::Startup:
               // Critical phase, don't lose messages
               ErrorReporter::configureAsyncQueue(5000, QueueOverflowPolicy::BlockProducer);
               break;
           case ApplicationPhase::NormalOperation:
               // Balance performance and reliability
               ErrorReporter::configureAsyncQueue(2000, QueueOverflowPolicy::DropOldest);
               break;
           case ApplicationPhase::Shutdown:
               // Ensure all messages are processed
               ErrorReporter::enableAsyncLogging(false);
               break;
       }
   }
   ```

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

4. **Thread Blocking**:
   - If using `BlockProducer` policy, be aware that threads may block
   - Consider using `DropOldest` for UI or performance-critical threads
   - Add timeout handling for situations where blocking would be problematic

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

// Test different overflow policies
void testOverflowPolicies() {
    // Test each policy with a small queue for demonstration
    const size_t smallQueueSize = 10;
    const int messagesToSend = 20; // Will overflow
    
    for (auto policy : {QueueOverflowPolicy::DropOldest, 
                        QueueOverflowPolicy::DropNewest,
                        QueueOverflowPolicy::BlockProducer,
                        QueueOverflowPolicy::WarnOnly}) {
        // Configure a small queue with the test policy
        ErrorReporter::configureAsyncQueue(smallQueueSize, policy);
        ErrorReporter::clearLogDestinations(); // For clean test
        ErrorReporter::enableFileLogging("logs/policy_test.log");
        
        // Send more messages than the queue can hold
        for (int i = 0; i < messagesToSend; i++) {
            ErrorReporter::logDebug("Test message " + std::to_string(i));
        }
        
        // Check queue stats after test
        AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();
        std::cout << "Policy: " << static_cast<int>(policy) 
                  << " - Queue size: " << stats.currentQueueSize 
                  << " - Overflow count: " << stats.overflowCount << std::endl;
        
        // Flush and reset for next test
        ErrorReporter::flushLogs();
    }
}
```

## Implementation Details

The async logging system consists of several key components:

1. **Message Queue**: A thread-safe queue that holds pending log messages
2. **Worker Thread**: A dedicated thread that processes messages from the queue
3. **Synchronization Primitives**: Mutexes and condition variables that ensure thread safety
4. **Overflow Handling**: Logic to manage queue overflow according to the configured policy
5. **Statistics Tracking**: Counters for monitoring queue behavior and performance

### Queue Manager Implementation

A simplified view of the bounded queue implementation:

```cpp
// Key class members
static std::deque<LogMessage> queue_;           // The message queue
static size_t maxQueueSize_;                    // Maximum allowed queue size (0 = unbounded)
static QueueOverflowPolicy queueOverflowPolicy_; // Policy for handling queue overflow
static size_t queueOverflowCount_;              // Count of messages dropped/rejected
static size_t queueHighWaterMark_;              // Maximum size the queue has reached

// Enqueue implementation (simplified)
void ErrorReporter::enqueueMessage(Severity severity, const std::string& message) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    // Check for queue overflow if bounded
    if (maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_) {
        // Handle overflow based on policy
        switch (queueOverflowPolicy_) {
            case QueueOverflowPolicy::DropOldest:
                queue_.pop_front();  // Remove oldest message
                queueOverflowCount_++;
                break;
                
            case QueueOverflowPolicy::DropNewest:
                queueOverflowCount_++;
                return;  // Discard new message
                
            case QueueOverflowPolicy::BlockProducer:
                // Wait until queue has space
                while (queue_.size() >= maxQueueSize_) {
                    queueNotFullCV_.wait(lock);
                }
                break;
                
            case QueueOverflowPolicy::WarnOnly:
                // Just count overflow but allow queue to grow
                queueOverflowCount_++;
                break;
        }
    }
    
    // Add message to queue
    queue_.push_back({severity, message, std::chrono::system_clock::now()});
    
    // Update high water mark if needed
    if (queue_.size() > queueHighWaterMark_) {
        queueHighWaterMark_ = queue_.size();
    }
    
    // Notify worker thread
    queueCV_.notify_one();
}
```

### Worker Thread Implementation

The worker thread processes messages from the queue:

```cpp
// Worker thread function (simplified)
void ErrorReporter::workerThreadFunction() {
    while (!shutdownRequested_) {
        LogMessage message;
        bool queueWasFull = false;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Wait for messages or shutdown signal
            queueCV_.wait(lock, [&]() {
                return !queue_.empty() || shutdownRequested_;
            });
            
            if (shutdownRequested_ && queue_.empty()) {
                break;
            }
            
            // Get next message
            if (!queue_.empty()) {
                queueWasFull = (maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_);
                message = queue_.front();
                queue_.pop_front();
            } else {
                continue;
            }
            
            // Signal producers waiting for space (if using BlockProducer)
            if (queueWasFull && queueOverflowPolicy_ == QueueOverflowPolicy::BlockProducer &&
                maxQueueSize_ > 0 && queue_.size() < maxQueueSize_) {
                queueNotFullCV_.notify_all();
            }
        }
        
        // Process message outside lock to reduce contention
        processLogMessage(message.severity, message.message, message.timestamp);
    }
}
```

## Additional Resources

- `src/EditorError.h`: API documentation for the logging system
- `tests/AsyncLoggingTest.cpp`: Examples of async logging usage and test cases
- `tests/FileLogStressTest.cpp`: Stress testing for file logging performance 