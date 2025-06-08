#pragma once

#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <future>
#include <atomic>
#include <thread>

class TextBuffer; // Forward declaration

/**
 * @class TextBufferOperation
 * @brief Represents a deferred operation on a TextBuffer
 * 
 * This class encapsulates an operation to be performed on a TextBuffer,
 * allowing it to be queued and executed later by the owner thread.
 */
class TextBufferOperation {
public:
    using OperationFunction = std::function<void(TextBuffer&)>;
    using ResultType = std::optional<std::string>;
    
    /**
     * @brief Constructs an operation with a callback function
     * @param operation The function to execute on the TextBuffer
     * @param hasResult Whether this operation produces a result value
     */
    explicit TextBufferOperation(OperationFunction operation, bool hasResult = false);
    
    /**
     * @brief Executes the operation on the given TextBuffer
     * @param buffer Reference to the TextBuffer to operate on
     */
    void execute(TextBuffer& buffer);
    
    /**
     * @brief Sets the result of the operation
     * @param result The string result value
     */
    void setResult(const std::string& result);
    
    /**
     * @brief Sets the operation as completed with no result
     */
    void markCompleted();
    
    /**
     * @brief Waits for the operation to complete and returns the result
     * @return Optional string result (empty if operation has no result)
     */
    ResultType waitForResult();
    
    /**
     * @brief Checks if the operation has completed
     * @return True if completed, false otherwise
     */
    bool isCompleted() const;
    
    /**
     * @brief Returns the future associated with this operation
     */
    std::shared_future<ResultType> getFuture() const;

private:
    OperationFunction operation_;
    std::promise<ResultType> promise_;
    std::shared_future<ResultType> future_;
    bool hasResult_;
    std::atomic<bool> completed_;
};

/**
 * @class TextBufferOperationQueue
 * @brief Thread-safe queue for TextBuffer operations
 * 
 * This class provides a thread-safe queue for TextBuffer operations,
 * allowing operations to be enqueued from any thread and dequeued
 * by the owner thread for execution.
 */
class TextBufferOperationQueue {
public:
    TextBufferOperationQueue();
    ~TextBufferOperationQueue();
    
    /**
     * @brief Enqueues an operation
     * @param operation The operation to enqueue
     * @return A future that will contain the result of the operation
     */
    std::shared_future<TextBufferOperation::ResultType> enqueue(TextBufferOperation::OperationFunction operation, bool hasResult = false);
    
    /**
     * @brief Dequeues an operation
     * @return The next operation in the queue, or nullptr if the queue is empty
     */
    std::shared_ptr<TextBufferOperation> dequeue();
    
    /**
     * @brief Checks if the queue is empty
     * @return True if the queue is empty, false otherwise
     */
    bool isEmpty() const;
    
    /**
     * @brief Returns the number of operations in the queue
     * @return The queue size
     */
    size_t size() const;
    
    /**
     * @brief Signals that no more operations will be added
     * This will allow waiters on dequeue to return nullptr
     */
    void shutdown();
    
    /**
     * @brief Waits for the queue to become empty
     * @param timeout_ms Maximum time to wait in milliseconds (0 = wait indefinitely)
     * @return True if queue became empty, false if timeout occurred
     */
    bool waitUntilEmpty(size_t timeout_ms = 0);

private:
    std::queue<std::shared_ptr<TextBufferOperation>> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_dequeue_;
    std::condition_variable cv_empty_;
    bool shutdown_;
}; 