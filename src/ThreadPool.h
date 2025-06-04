#pragma once

#include <vector>
#include <queue>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <atomic>
#include <stdexcept>
#include <type_traits>
#include "AppDebugLog.h"

/**
 * @class ThreadPool
 * @brief A flexible thread pool implementation with priority support
 * 
 * This thread pool manages a collection of worker threads that process
 * tasks submitted to the pool. Tasks can be submitted with priorities
 * to ensure important tasks are processed first.
 */
class ThreadPool {
public:
    /**
     * @brief Task priority levels
     */
    enum class Priority {
        HIGH = 0,    ///< High priority tasks (processed first)
        NORMAL = 1,  ///< Normal priority tasks
        LOW = 2      ///< Low priority tasks (processed last)
    };

    /**
     * @brief Constructor
     * 
     * @param numThreads Number of worker threads to create (defaults to hardware concurrency)
     */
    explicit ThreadPool(size_t numThreads = 0)
        : stop_(false)
        , activeThreads_(0) 
    {
        // Default to hardware concurrency or fallback to 2 threads
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) {
                numThreads = 2;
            }
        }
        
        LOG_DEBUG("Creating ThreadPool with " + std::to_string(numThreads) + " threads");
        
        // Create worker threads
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                workerThread();
            });
        }
    }
    
    /**
     * @brief Destructor
     * 
     * Stops all worker threads and waits for them to finish.
     */
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        LOG_DEBUG("ThreadPool destroyed");
    }
    
    /**
     * @brief Submit a task to the thread pool with priority
     * 
     * @tparam F Function type
     * @tparam Args Argument types
     * @param priority The task priority
     * @param f The function to execute
     * @param args The function arguments
     * @return A future that will contain the function's return value
     */
    template<class F, class... Args>
    auto submit(Priority priority, F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using ReturnType = typename std::invoke_result<F, Args...>::type;
        
        // Create a packaged task with the function and its arguments
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        // Get future for result
        std::future<ReturnType> result = task->get_future();
        
        // Add task to queue
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Don't allow enqueueing after stopping the pool
            if (stop_) {
                throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");
            }
            
            // Add task to appropriate priority queue
            tasks_[static_cast<int>(priority)].emplace([task]() { (*task)(); });
        }
        
        // Notify a waiting thread
        condition_.notify_one();
        
        return result;
    }
    
    /**
     * @brief Get the current number of active threads
     * 
     * @return The number of threads currently processing tasks
     */
    size_t getActiveThreadCount() const {
        return activeThreads_.load();
    }
    
    /**
     * @brief Get the total number of threads in the pool
     * 
     * @return The total number of worker threads
     */
    size_t getThreadCount() const {
        return workers_.size();
    }
    
    /**
     * @brief Get the number of pending tasks in the queue
     * 
     * @return The total number of queued tasks across all priorities
     */
    size_t getQueueSize() {
        std::unique_lock<std::mutex> lock(queueMutex_);
        return tasks_[0].size() + tasks_[1].size() + tasks_[2].size();
    }
    
    /**
     * @brief Shutdown the thread pool
     * 
     * Stops all worker threads after they complete their current tasks.
     * The destructor will also call this, but this allows for explicit shutdown.
     */
    void shutdown() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        
        condition_.notify_all();
    }

private:
    /**
     * @brief Worker thread function
     * 
     * Continuously pulls tasks from the queue and executes them.
     */
    void workerThread() {
        try {
            while (true) {
                std::function<void()> task;
                
                // Get a task from the queue
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    
                    // Wait for a task or stop signal
                    condition_.wait(lock, [this] {
                        return stop_ || !tasks_[0].empty() || !tasks_[1].empty() || !tasks_[2].empty();
                    });
                    
                    // Exit if stopped and no more tasks
                    if (stop_ && tasks_[0].empty() && tasks_[1].empty() && tasks_[2].empty()) {
                        return;
                    }
                    
                    // Get highest priority task available
                    if (!tasks_[0].empty()) {
                        // High priority
                        task = std::move(tasks_[0].front());
                        tasks_[0].pop();
                    } else if (!tasks_[1].empty()) {
                        // Normal priority
                        task = std::move(tasks_[1].front());
                        tasks_[1].pop();
                    } else if (!tasks_[2].empty()) {
                        // Low priority
                        task = std::move(tasks_[2].front());
                        tasks_[2].pop();
                    }
                }
                
                // Execute the task
                if (task) {
                    activeThreads_++;
                    try {
                        task();
                    } catch (const std::exception& e) {
                        LOG_ERROR("Exception in thread pool task: " + std::string(e.what()));
                    } catch (...) {
                        LOG_ERROR("Unknown exception in thread pool task");
                    }
                    activeThreads_--;
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in worker thread: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in worker thread");
        }
    }

private:
    // Thread control
    std::vector<std::thread> workers_;
    std::atomic<bool> stop_;
    std::atomic<size_t> activeThreads_;
    
    // Task queues (one for each priority level)
    std::array<std::queue<std::function<void()>>, 3> tasks_;
    
    // Synchronization
    std::mutex queueMutex_;
    std::condition_variable condition_;
}; 