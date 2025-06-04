#include "gtest/gtest.h"
#include "ThreadPool.h"
#include <vector>
#include <atomic>
#include <numeric>
#include <chrono>
#include <future>

// Test basic functionality of the thread pool
TEST(ThreadPoolTest, BasicFunctionality) {
    // Create a thread pool with 4 threads
    ThreadPool pool(4);
    
    // Submit a simple task
    auto future = pool.submit(ThreadPool::Priority::NORMAL, []() {
        return 42;
    });
    
    // Wait for the task to complete and check the result
    EXPECT_EQ(future.get(), 42);
}

// Test that tasks are executed concurrently
TEST(ThreadPoolTest, ConcurrentExecution) {
    // Create a thread pool with 4 threads
    ThreadPool pool(4);
    
    // Counter for completed tasks
    std::atomic<int> counter(0);
    
    // Submit multiple tasks
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit(ThreadPool::Priority::NORMAL, [&counter]() {
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter++;
        }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    // Check that all tasks were completed
    EXPECT_EQ(counter, 100);
}

// Test task priorities
TEST(ThreadPoolTest, TaskPriorities) {
    // Create a thread pool with 1 thread to ensure sequential execution
    ThreadPool pool(1);
    
    // Use an atomic vector to store the order of execution
    std::vector<int> executionOrder;
    std::mutex orderMutex;
    
    // Submit tasks with different priorities
    auto lowPriorityTask = pool.submit(ThreadPool::Priority::LOW, [&]() {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(3);
        return 3;
    });
    
    auto normalPriorityTask = pool.submit(ThreadPool::Priority::NORMAL, [&]() {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(2);
        return 2;
    });
    
    auto highPriorityTask = pool.submit(ThreadPool::Priority::HIGH, [&]() {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(1);
        return 1;
    });
    
    // Wait for all tasks to complete
    highPriorityTask.wait();
    normalPriorityTask.wait();
    lowPriorityTask.wait();
    
    // Check execution order
    ASSERT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 1); // High priority should execute first
    EXPECT_EQ(executionOrder[1], 2); // Normal priority should execute second
    EXPECT_EQ(executionOrder[2], 3); // Low priority should execute last
}

// Test thread pool statistics
TEST(ThreadPoolTest, Statistics) {
    // Create a thread pool with 2 threads
    ThreadPool pool(2);
    
    // Check initial state
    EXPECT_EQ(pool.getThreadCount(), 2);
    EXPECT_EQ(pool.getActiveThreadCount(), 0);
    EXPECT_EQ(pool.getQueueSize(), 0);
    
    // Submit a long-running task
    auto future = pool.submit(ThreadPool::Priority::NORMAL, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });
    
    // Give some time for the task to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Check statistics during execution
    EXPECT_EQ(pool.getThreadCount(), 2);
    EXPECT_GT(pool.getActiveThreadCount(), 0);
    
    // Wait for the task to complete
    future.wait();
    
    // Give some time for the thread to return to idle
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Check final state
    EXPECT_EQ(pool.getActiveThreadCount(), 0);
    EXPECT_EQ(pool.getQueueSize(), 0);
}

// Test exception handling in tasks
TEST(ThreadPoolTest, ExceptionHandling) {
    // Create a thread pool
    ThreadPool pool(2);
    
    // Submit a task that throws an exception
    auto future = pool.submit(ThreadPool::Priority::NORMAL, []() -> int {
        throw std::runtime_error("Test exception");
    });
    
    // Check that the exception is propagated
    EXPECT_THROW(future.get(), std::runtime_error);
    
    // Submit another task to verify the pool is still functional
    auto future2 = pool.submit(ThreadPool::Priority::NORMAL, []() {
        return 42;
    });
    
    // Check that the pool is still working
    EXPECT_EQ(future2.get(), 42);
}

// Test shutdown behavior
TEST(ThreadPoolTest, Shutdown) {
    // Create a thread pool
    ThreadPool* pool = new ThreadPool(2);
    
    // Submit a bunch of tasks
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool->submit(ThreadPool::Priority::NORMAL, [i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return i;
        }));
    }
    
    // Destroy the pool (should trigger a shutdown)
    delete pool;
    
    // Tasks may not complete if the pool is shutdown abruptly
    // This is a design decision - we're testing that shutdown doesn't crash
    // We don't check the futures since they might be invalid after shutdown
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 