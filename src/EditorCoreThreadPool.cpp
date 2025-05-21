#include "EditorCoreThreadPool.h"
#include "TextBuffer.h"
#include "AppDebugLog.h"
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

EditorCoreThreadPool::EditorCoreThreadPool(size_t numThreads)
    : running_(false)
    , textBufferOwnerIndex_(0)  // Default to first thread as TextBuffer owner
    , textBufferOperationsAvailable_(false)
{
    // Ensure at least one thread for the pool
    numThreads = std::max(numThreads, static_cast<size_t>(1));
    
    // Reserve space for worker threads
    workerThreads_.reserve(numThreads);
    
    LOG_DEBUG("EditorCoreThreadPool created with " + std::to_string(numThreads) + " threads");
}

EditorCoreThreadPool::~EditorCoreThreadPool() {
    // Ensure threads are properly shut down
    shutdown();
}

void EditorCoreThreadPool::start() {
    std::lock_guard<std::mutex> lock(taskQueueMutex_);
    
    if (running_) {
        LOG_WARNING("EditorCoreThreadPool::start() called when already running");
        return;
    }
    
    running_ = true;
    
    // Create and start worker threads
    for (size_t i = 0; i < workerThreads_.capacity(); ++i) {
        if (i == textBufferOwnerIndex_) {
            // This thread will be dedicated to TextBuffer operations
            workerThreads_.emplace_back(&EditorCoreThreadPool::textBufferWorkerFunction, this, i);
            LOG_DEBUG("Started TextBuffer owner thread (index " + std::to_string(i) + ")");
        } else {
            // General worker thread
            workerThreads_.emplace_back(&EditorCoreThreadPool::generalWorkerFunction, this, i);
            LOG_DEBUG("Started general worker thread (index " + std::to_string(i) + ")");
        }
    }
}

void EditorCoreThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex_);
        
        if (!running_) {
            return;
        }
        
        running_ = false;
        
        // Clear any pending tasks
        std::queue<std::function<void()>> empty;
        std::swap(taskQueue_, empty);
    }
    
    // Notify all waiting threads to check the running_ flag
    taskQueueCondition_.notify_all();
    textBufferCondition_.notify_all();
    
    // Wait for all threads to finish
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Clear the thread vector
    workerThreads_.clear();
    
    LOG_DEBUG("EditorCoreThreadPool shut down successfully");
}

std::thread::id EditorCoreThreadPool::assignTextBufferOwnership(std::shared_ptr<TextBuffer> buffer) {
    std::lock_guard<std::mutex> lock(textBufferMutex_);
    
    if (!buffer) {
        LOG_ERROR("Attempted to assign ownership of null TextBuffer");
        return std::thread::id();
    }
    
    // Store the buffer
    ownedTextBuffer_ = buffer;
    
    // If threads are already running, set the owner thread ID
    if (!workerThreads_.empty() && textBufferOwnerIndex_ < workerThreads_.size()) {
        std::thread::id ownerId = workerThreads_[textBufferOwnerIndex_].get_id();
        buffer->setOwnerThread(ownerId);
        
        LOG_DEBUG("TextBuffer ownership assigned to thread index " + 
                 std::to_string(textBufferOwnerIndex_));
        
        return ownerId;
    } else {
        LOG_WARNING("TextBuffer ownership assignment deferred - thread pool not started");
        return std::thread::id();
    }
}

bool EditorCoreThreadPool::isPoolThread() const {
    std::thread::id currentId = std::this_thread::get_id();
    
    for (const auto& thread : workerThreads_) {
        if (thread.get_id() == currentId) {
            return true;
        }
    }
    
    return false;
}

bool EditorCoreThreadPool::isTextBufferOwnerThread() const {
    if (workerThreads_.empty() || textBufferOwnerIndex_ >= workerThreads_.size()) {
        return false;
    }
    
    return std::this_thread::get_id() == workerThreads_[textBufferOwnerIndex_].get_id();
}

void EditorCoreThreadPool::submitTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex_);
        
        if (!running_) {
            LOG_WARNING("Task submitted to stopped thread pool");
            return;
        }
        
        taskQueue_.push(std::move(task));
    }
    
    // Notify one waiting thread that a new task is available
    taskQueueCondition_.notify_one();
}

size_t EditorCoreThreadPool::threadCount() const {
    return workerThreads_.size();
}

void EditorCoreThreadPool::notifyTextBufferOperationsAvailable() {
    textBufferOperationsAvailable_ = true;
    textBufferCondition_.notify_one();
}

void EditorCoreThreadPool::generalWorkerFunction(size_t threadIndex) {
    LOG_DEBUG("General worker thread " + std::to_string(threadIndex) + " started");
    
    while (running_) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(taskQueueMutex_);
            
            // Wait for a task or shutdown signal
            taskQueueCondition_.wait(lock, [this] {
                return !taskQueue_.empty() || !running_;
            });
            
            // Check if we should exit
            if (!running_ && taskQueue_.empty()) {
                break;
            }
            
            // Get a task from the queue
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }
        
        // Execute the task if we got one
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in worker thread task: " + std::string(e.what()));
            } catch (...) {
                LOG_ERROR("Unknown exception in worker thread task");
            }
        }
    }
    
    LOG_DEBUG("General worker thread " + std::to_string(threadIndex) + " stopped");
}

void EditorCoreThreadPool::textBufferWorkerFunction(size_t threadIndex) {
    LOG_DEBUG("TextBuffer owner thread " + std::to_string(threadIndex) + " started");
    
    // Set thread name for debugging (platform-specific, omitted here)
    
    while (running_) {
        bool processedOperations = false;
        
        // First, process any TextBuffer operations
        {
            std::lock_guard<std::mutex> lock(textBufferMutex_);
            if (ownedTextBuffer_) {
                processTextBufferOperations();
                processedOperations = true;
            }
        }
        
        // If no TextBuffer operations were processed, check for general tasks
        if (!processedOperations) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(taskQueueMutex_);
                
                if (!taskQueue_.empty()) {
                    task = std::move(taskQueue_.front());
                    taskQueue_.pop();
                }
            }
            
            // Execute the task if we got one
            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in TextBuffer thread task: " + std::string(e.what()));
                } catch (...) {
                    LOG_ERROR("Unknown exception in TextBuffer thread task");
                }
                continue;  // Skip the wait if we processed a task
            }
        }
        
        // If no work was done, wait for a notification or check periodically
        {
            std::unique_lock<std::mutex> lock(textBufferMutex_);
            
            // Wait until notified or timeout
            textBufferCondition_.wait_for(lock, 10ms, [this] {
                return textBufferOperationsAvailable_ || !running_;
            });
            
            // Reset the notification flag
            textBufferOperationsAvailable_ = false;
        }
    }
    
    LOG_DEBUG("TextBuffer owner thread " + std::to_string(threadIndex) + " stopped");
}

void EditorCoreThreadPool::processTextBufferOperations() {
    if (!ownedTextBuffer_) {
        return;
    }
    
    try {
        // Process all pending operations in the TextBuffer's queue
        size_t processedCount = ownedTextBuffer_->processOperationQueue();
        
        if (processedCount > 0) {
            LOG_DEBUG("Processed " + std::to_string(processedCount) + 
                     " TextBuffer operations");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while processing TextBuffer operations: " + 
                 std::string(e.what()));
    }
} 