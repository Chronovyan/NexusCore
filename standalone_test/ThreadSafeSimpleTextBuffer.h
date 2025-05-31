#pragma once

#include "SimpleTextBuffer.h"
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <memory>

/**
 * @class ThreadSafeSimpleTextBuffer
 * @brief Thread-safe decorator for SimpleTextBuffer
 * 
 * This class wraps a SimpleTextBuffer instance and provides thread-safe access
 * to its methods using appropriate synchronization mechanisms.
 * 
 * Thread Safety Guarantees:
 * 1. Individual method calls are thread-safe
 * 2. References returned by methods remain valid only until the next modification
 *    of the buffer from any thread. Callers must be careful when storing references.
 * 3. For operations that need to be atomic across multiple method calls,
 *    use the lockForReading() and lockForWriting() methods
 */
class ThreadSafeSimpleTextBuffer : public ISimpleTextBuffer {
public:
    /**
     * @brief Construct a new ThreadSafeSimpleTextBuffer
     * 
     * @param buffer Optional existing SimpleTextBuffer to wrap. If nullptr, creates a new one.
     */
    explicit ThreadSafeSimpleTextBuffer(std::shared_ptr<SimpleTextBuffer> buffer = nullptr)
        : buffer_(buffer ? buffer : std::make_shared<SimpleTextBuffer>()) {
    }
    
    // ISimpleTextBuffer interface implementation - Write Operations
    void addLine(const std::string& line) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->addLine(line);
    }
    
    void insertLine(size_t index, const std::string& line) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->insertLine(index, line);
    }
    
    void deleteLine(size_t index) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->deleteLine(index);
    }
    
    void replaceLine(size_t index, const std::string& newLine) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->replaceLine(index, newLine);
    }
    
    void clear(bool keepEmptyLine) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->clear(keepEmptyLine);
    }
    
    void insertString(size_t lineIndex, size_t colIndex, const std::string& text) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->insertString(lineIndex, colIndex, text);
    }
    
    void insertChar(size_t lineIndex, size_t colIndex, char ch) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->insertChar(lineIndex, colIndex, ch);
    }
    
    void deleteChar(size_t lineIndex, size_t colIndex) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        buffer_->deleteChar(lineIndex, colIndex);
    }
    
    // ISimpleTextBuffer interface implementation - Read Operations
    const std::string& getLine(size_t index) const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return buffer_->getLine(index);
    }
    
    size_t lineCount() const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return buffer_->lineCount();
    }
    
    bool isEmpty() const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return buffer_->isEmpty();
    }
    
    // Non-const getLine is more complex for thread safety
    std::string& getLine(size_t index) override {
        // Note: This implementation is problematic for thread safety since we return a reference
        // In a truly thread-safe implementation, we would need to return a proxy object or
        // require explicit locking by the caller.
        // For now, we use a unique lock to ensure exclusivity
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return buffer_->getLine(index);
    }
    
    /**
     * @brief Get the underlying SimpleTextBuffer
     * 
     * This method is provided for advanced use cases where direct access
     * to the underlying SimpleTextBuffer is needed. Use with caution, as operations
     * on the returned SimpleTextBuffer are not thread-safe.
     * 
     * @return std::shared_ptr<SimpleTextBuffer> The underlying SimpleTextBuffer
     */
    std::shared_ptr<SimpleTextBuffer> getUnderlyingBuffer() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return buffer_;
    }
    
    /**
     * @brief Lock for reading
     * 
     * This method acquires a shared lock on the buffer for reading.
     * Use this when you need to perform multiple read operations atomically.
     * IMPORTANT: You MUST call unlockReading() when done to prevent deadlocks.
     */
    void lockForReading() const {
        mutex_.lock_shared();
    }
    
    /**
     * @brief Unlock after reading
     * 
     * Call this method to release a shared lock acquired with lockForReading().
     */
    void unlockReading() const {
        mutex_.unlock_shared();
    }
    
    /**
     * @brief Lock for writing
     * 
     * This method acquires an exclusive lock on the buffer for writing.
     * Use this when you need to perform multiple write operations atomically.
     * IMPORTANT: You MUST call unlockWriting() when done to prevent deadlocks.
     */
    void lockForWriting() {
        mutex_.lock();
    }
    
    /**
     * @brief Unlock after writing
     * 
     * Call this method to release an exclusive lock acquired with lockForWriting().
     */
    void unlockWriting() {
        mutex_.unlock();
    }
    
private:
    std::shared_ptr<SimpleTextBuffer> buffer_;
    mutable std::shared_mutex mutex_;
}; 