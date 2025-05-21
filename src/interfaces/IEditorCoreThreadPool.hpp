#pragma once

#include <memory>
#include <thread>
#include <functional>

// Forward declarations
class TextBuffer;

/**
 * @interface IEditorCoreThreadPool
 * @brief Interface for the editor core thread pool
 * 
 * This interface defines the contract for the editor core thread pool,
 * which manages worker threads for processing operations on core editor
 * components like TextBuffer.
 */
class IEditorCoreThreadPool {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IEditorCoreThreadPool() = default;
    
    /**
     * @brief Starts all worker threads in the pool
     */
    virtual void start() = 0;
    
    /**
     * @brief Signals all threads to stop and waits for them to finish
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Assigns ownership of a TextBuffer to a dedicated thread in the pool
     * @param buffer Pointer to the TextBuffer to be owned
     * @return ID of the thread that now owns the buffer
     */
    virtual std::thread::id assignTextBufferOwnership(std::shared_ptr<TextBuffer> buffer) = 0;
    
    /**
     * @brief Checks if the current thread is part of the thread pool
     * @return True if the calling thread is part of this pool
     */
    virtual bool isPoolThread() const = 0;
    
    /**
     * @brief Checks if the current thread is the designated TextBuffer owner
     * @return True if the calling thread is the TextBuffer owner
     */
    virtual bool isTextBufferOwnerThread() const = 0;
    
    /**
     * @brief Submits a general task to be executed by any thread in the pool
     * @param task The function to be executed
     */
    virtual void submitTask(std::function<void()> task) = 0;
    
    /**
     * @brief Returns the number of worker threads in the pool
     * @return Number of threads
     */
    virtual size_t threadCount() const = 0;
    
    /**
     * @brief Wakes up the TextBuffer owner thread to process operations
     * This can be called when operations are added to the TextBuffer's queue
     */
    virtual void notifyTextBufferOperationsAvailable() = 0;
}; 