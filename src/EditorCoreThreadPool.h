#pragma once

#include "interfaces/IEditorCoreThreadPool.hpp"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <queue>
#include <memory>
#include <unordered_map>
#include <string>

class TextBuffer;

/**
 * @class EditorCoreThreadPool
 * @brief Manages a pool of worker threads for the editor core components
 * 
 * This class implements a specialized thread pool for the editor core,
 * with particular focus on managing TextBuffer ownership and operation
 * processing. It follows the Thread Ownership Model where specific
 * editor components are owned by specific threads in this pool.
 */
class EditorCoreThreadPool : public IEditorCoreThreadPool {
public:
    /**
     * @brief Constructs the thread pool with the specified number of worker threads
     * @param numThreads Number of worker threads to create (default: 2)
     */
    explicit EditorCoreThreadPool(size_t numThreads = 2);
    
    /**
     * @brief Destructor ensures proper shutdown of all threads
     */
    ~EditorCoreThreadPool() override;
    
    // IEditorCoreThreadPool interface implementation
    void start() override;
    void shutdown() override;
    std::thread::id assignTextBufferOwnership(std::shared_ptr<TextBuffer> buffer) override;
    bool isPoolThread() const override;
    bool isTextBufferOwnerThread() const override;
    void submitTask(std::function<void()> task) override;
    size_t threadCount() const override;
    void notifyTextBufferOperationsAvailable() override;

private:
    // Thread pool state
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_;
    size_t textBufferOwnerIndex_; // Index of the thread that owns TextBuffer
    
    // Task queue for general tasks
    std::queue<std::function<void()>> taskQueue_;
    std::mutex taskQueueMutex_;
    std::condition_variable taskQueueCondition_;
    
    // TextBuffer management
    std::shared_ptr<TextBuffer> ownedTextBuffer_;
    std::mutex textBufferMutex_;
    std::condition_variable textBufferCondition_;
    std::atomic<bool> textBufferOperationsAvailable_;
    
    // Thread worker functions
    void generalWorkerFunction(size_t threadIndex);
    void textBufferWorkerFunction(size_t threadIndex);
    
    // Helper methods
    void processTextBufferOperations();
}; 