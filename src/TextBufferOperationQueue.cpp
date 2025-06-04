#include "TextBufferOperationQueue.h"
#include "TextBuffer.h"
#include "EditorError.h"
#include <chrono>

// TextBufferOperation implementation
TextBufferOperation::TextBufferOperation(OperationFunction operation, bool hasResult)
    : operation_(std::move(operation))
    , future_(promise_.get_future().share())
    , hasResult_(hasResult)
    , completed_(false)
{
}

void TextBufferOperation::execute(TextBuffer& buffer) {
    try {
        operation_(buffer);
        
        // If this operation doesn't return a result, mark it completed immediately
        if (!hasResult_) {
            markCompleted();
        }
    }
    catch (const std::exception&) {
        // Set the promise as an exception
        promise_.set_exception(std::current_exception());
        completed_ = true;
    }
}

void TextBufferOperation::setResult(const std::string& result) {
    if (!completed_) {
        ResultType resultValue = result;
        promise_.set_value(resultValue);
        completed_ = true;
    }
}

void TextBufferOperation::markCompleted() {
    if (!completed_) {
        ResultType empty;
        promise_.set_value(empty);
        completed_ = true;
    }
}

TextBufferOperation::ResultType TextBufferOperation::waitForResult() {
    try {
        return future_.get();
    }
    catch (...) {
        // Re-throw any exceptions that occurred during execution
        throw;
    }
}

bool TextBufferOperation::isCompleted() const {
    return completed_;
}

std::shared_future<TextBufferOperation::ResultType> TextBufferOperation::getFuture() const {
    return future_;
}

// TextBufferOperationQueue implementation
TextBufferOperationQueue::TextBufferOperationQueue()
    : shutdown_(false)
{
}

TextBufferOperationQueue::~TextBufferOperationQueue() {
    shutdown();
}

std::shared_future<TextBufferOperation::ResultType> TextBufferOperationQueue::enqueue(
    TextBufferOperation::OperationFunction operation, bool hasResult) 
{
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (shutdown_) {
        throw TextBufferException("Cannot enqueue to a shutdown operation queue", 
                                  EditorException::Severity::Error);
    }
    
    auto op = std::make_shared<TextBufferOperation>(std::move(operation), hasResult);
    queue_.push(op);
    
    // Notify one waiting thread that a new operation is available
    cv_dequeue_.notify_one();
    
    return op->getFuture();
}

std::shared_ptr<TextBufferOperation> TextBufferOperationQueue::dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until the queue is not empty or shutdown is requested
    cv_dequeue_.wait(lock, [this] { 
        return !queue_.empty() || shutdown_;
    });
    
    // If the queue is empty after shutdown, return nullptr
    if (queue_.empty()) {
        return nullptr;
    }
    
    // Get the front operation
    auto op = queue_.front();
    queue_.pop();
    
    // If queue becomes empty, notify any threads waiting for that condition
    if (queue_.empty()) {
        cv_empty_.notify_all();
    }
    
    return op;
}

bool TextBufferOperationQueue::isEmpty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t TextBufferOperationQueue::size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
}

void TextBufferOperationQueue::shutdown() {
    std::unique_lock<std::mutex> lock(mutex_);
    shutdown_ = true;
    
    // Notify all waiting threads
    cv_dequeue_.notify_all();
    cv_empty_.notify_all();
}

bool TextBufferOperationQueue::waitUntilEmpty(size_t timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return true;
    }
    
    if (timeout_ms == 0) {
        // Wait indefinitely
        cv_empty_.wait(lock, [this] { return queue_.empty(); });
        return true;
    } else {
        // Wait with timeout
        return cv_empty_.wait_for(lock, std::chrono::milliseconds(timeout_ms), 
                                [this] { return queue_.empty(); });
    }
} 