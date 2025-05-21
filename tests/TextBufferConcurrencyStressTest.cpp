#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <future>
#include <algorithm>
#include <cassert>

#include "AppDebugLog.h"
#include "TextBuffer.h"
#include "EditorCoreThreadPool.h"

// Configuration constants for the stress test
namespace StressTestConfig {
    // Number of worker threads to spawn for stress testing
    constexpr int NUM_WORKER_THREADS = 16;
    
    // Number of operations each worker thread will perform
    constexpr int OPERATIONS_PER_THREAD = 1000;
    
    // Maximum line length for generated content
    constexpr int MAX_LINE_LENGTH = 100;
    
    // Probability weights for different operations (must sum to 100)
    constexpr int PROB_ADD_LINE = 30;
    constexpr int PROB_INSERT_LINE = 20;
    constexpr int PROB_DELETE_LINE = 15;
    constexpr int PROB_REPLACE_LINE = 25;
    constexpr int PROB_READ_LINE = 10;
    
    // Minimum number of initial lines in the TextBuffer
    constexpr int MIN_INITIAL_LINES = 10;
    
    // Test duration in seconds (if using time-based testing instead of operation count)
    constexpr int TEST_DURATION_SECONDS = 10;
    
    // Sleep range between operations (milliseconds)
    constexpr int MIN_SLEEP_MS = 0;
    constexpr int MAX_SLEEP_MS = 5;
    
    // Whether to verify the integrity of the TextBuffer after the test
    constexpr bool VERIFY_BUFFER_INTEGRITY = true;
    
    // Whether to track and verify that all operations were processed
    constexpr bool TRACK_OPERATIONS = true;
}

// Operation tracking for verification
struct OperationResult {
    enum class Status {
        PENDING,
        COMPLETED,
        FAILED
    };
    
    Status status = Status::PENDING;
    std::string description;
    std::string errorMessage;
};

// Helper class to generate random text and operations
class RandomGenerator {
public:
    RandomGenerator() : engine_(std::random_device{}()) {}
    
    // Generate a random string of specified length
    std::string generateRandomString(int length) {
        static const char charset[] = 
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789 !@#$%^&*()-=_+[]{}|;:,.<>?";
        
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
        std::string result;
        result.reserve(length);
        
        for (int i = 0; i < length; ++i) {
            result += charset[dist(engine_)];
        }
        
        return result;
    }
    
    // Generate a random line of text
    std::string generateRandomLine() {
        std::uniform_int_distribution<> lengthDist(10, StressTestConfig::MAX_LINE_LENGTH);
        return generateRandomString(lengthDist(engine_));
    }
    
    // Select a random operation type based on probability weights
    enum class OperationType {
        ADD_LINE,
        INSERT_LINE,
        DELETE_LINE,
        REPLACE_LINE,
        READ_LINE
    };
    
    OperationType selectRandomOperation() {
        std::uniform_int_distribution<> dist(1, 100);
        int roll = dist(engine_);
        
        if (roll <= StressTestConfig::PROB_ADD_LINE) {
            return OperationType::ADD_LINE;
        } else if (roll <= StressTestConfig::PROB_ADD_LINE + StressTestConfig::PROB_INSERT_LINE) {
            return OperationType::INSERT_LINE;
        } else if (roll <= StressTestConfig::PROB_ADD_LINE + StressTestConfig::PROB_INSERT_LINE + 
                         StressTestConfig::PROB_DELETE_LINE) {
            return OperationType::DELETE_LINE;
        } else if (roll <= StressTestConfig::PROB_ADD_LINE + StressTestConfig::PROB_INSERT_LINE + 
                         StressTestConfig::PROB_DELETE_LINE + StressTestConfig::PROB_REPLACE_LINE) {
            return OperationType::REPLACE_LINE;
        } else {
            return OperationType::READ_LINE;
        }
    }
    
    // Select a random line index from the buffer
    size_t selectRandomLineIndex(size_t lineCount) {
        if (lineCount == 0) return 0;
        std::uniform_int_distribution<size_t> dist(0, lineCount - 1);
        return dist(engine_);
    }
    
    // Generate a random sleep duration
    std::chrono::milliseconds generateSleepDuration() {
        std::uniform_int_distribution<> dist(StressTestConfig::MIN_SLEEP_MS, StressTestConfig::MAX_SLEEP_MS);
        return std::chrono::milliseconds(dist(engine_));
    }
    
private:
    std::mt19937 engine_;
};

// Stress test class
class TextBufferStressTest {
public:
    TextBufferStressTest()
        : textBuffer_(std::make_shared<TextBuffer>()),
          threadPool_(std::make_unique<EditorCoreThreadPool>(4)), // Use 4 threads in the pool
          stopRequested_(false),
          operationsCompleted_(0),
          operationsFailed_(0)
    {
        LOG_INIT("TextBufferStressTest");
        LOG_DEBUG("Initializing TextBuffer stress test");
    }
    
    // Initialize the test
    void initialize() {
        // Start the thread pool
        threadPool_->start();
        
        // Assign TextBuffer ownership to a thread in the pool
        ownerThreadId_ = threadPool_->assignTextBufferOwnership(textBuffer_);
        LOG_DEBUG("TextBuffer ownership assigned to thread: " + 
                 std::to_string(std::hash<std::thread::id>{}(ownerThreadId_)));
        
        // Add some initial content to the TextBuffer
        for (int i = 0; i < StressTestConfig::MIN_INITIAL_LINES; ++i) {
            std::string line = "Initial line " + std::to_string(i) + ": " + randomGen_.generateRandomLine();
            textBuffer_->addLine(line);
        }
        
        LOG_DEBUG("TextBuffer initialized with " + std::to_string(textBuffer_->lineCount()) + " lines");
    }
    
    // Run the stress test
    void runTest() {
        LOG_DEBUG("Starting stress test with " + 
                 std::to_string(StressTestConfig::NUM_WORKER_THREADS) + " worker threads");
        
        // Create worker threads
        std::vector<std::thread> workerThreads;
        std::vector<std::vector<OperationResult>> operationResults;
        
        operationResults.resize(StressTestConfig::NUM_WORKER_THREADS);
        for (int i = 0; i < StressTestConfig::NUM_WORKER_THREADS; ++i) {
            operationResults[i].resize(StressTestConfig::OPERATIONS_PER_THREAD);
        }
        
        // Start time measurement
        auto startTime = std::chrono::steady_clock::now();
        
        // Launch worker threads
        for (int i = 0; i < StressTestConfig::NUM_WORKER_THREADS; ++i) {
            workerThreads.emplace_back(&TextBufferStressTest::workerThreadFunction, 
                                      this, i, std::ref(operationResults[i]));
        }
        
        // Periodically notify the thread pool to process operations
        std::thread notifierThread([this]() {
            while (!stopRequested_) {
                threadPool_->notifyTextBufferOperationsAvailable();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Wait for all worker threads to complete
        for (auto& thread : workerThreads) {
            thread.join();
        }
        
        // Signal the notifier thread to stop
        stopRequested_ = true;
        notifierThread.join();
        
        // End time measurement
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Process any remaining operations
        threadPool_->notifyTextBufferOperationsAvailable();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify results
        if (StressTestConfig::TRACK_OPERATIONS) {
            verifyOperationResults(operationResults);
        }
        
        // Verify TextBuffer integrity
        if (StressTestConfig::VERIFY_BUFFER_INTEGRITY) {
            verifyBufferIntegrity();
        }
        
        // Log test summary
        LOG_DEBUG("Stress test completed in " + std::to_string(duration.count()) + " ms");
        LOG_DEBUG("Operations completed: " + std::to_string(operationsCompleted_));
        LOG_DEBUG("Operations failed: " + std::to_string(operationsFailed_));
        LOG_DEBUG("Final TextBuffer line count: " + std::to_string(textBuffer_->lineCount()));
    }
    
    // Clean up resources
    void cleanup() {
        LOG_DEBUG("Cleaning up stress test resources");
        threadPool_->shutdown();
    }
    
private:
    // Worker thread function
    void workerThreadFunction(int threadId, std::vector<OperationResult>& results) {
        LOG_DEBUG("Worker thread " + std::to_string(threadId) + " started");
        
        for (int i = 0; i < StressTestConfig::OPERATIONS_PER_THREAD; ++i) {
            // Select a random operation
            auto opType = randomGen_.selectRandomOperation();
            
            try {
                switch (opType) {
                    case RandomGenerator::OperationType::ADD_LINE:
                        performAddLine(threadId, i, results[i]);
                        break;
                    case RandomGenerator::OperationType::INSERT_LINE:
                        performInsertLine(threadId, i, results[i]);
                        break;
                    case RandomGenerator::OperationType::DELETE_LINE:
                        performDeleteLine(threadId, i, results[i]);
                        break;
                    case RandomGenerator::OperationType::REPLACE_LINE:
                        performReplaceLine(threadId, i, results[i]);
                        break;
                    case RandomGenerator::OperationType::READ_LINE:
                        performReadLine(threadId, i, results[i]);
                        break;
                }
            } catch (const std::exception& e) {
                results[i].status = OperationResult::Status::FAILED;
                results[i].errorMessage = e.what();
                operationsFailed_++;
                LOG_ERROR("Worker " + std::to_string(threadId) + " operation " + 
                         std::to_string(i) + " failed: " + e.what());
            }
            
            // Random sleep between operations
            std::this_thread::sleep_for(randomGen_.generateSleepDuration());
        }
        
        LOG_DEBUG("Worker thread " + std::to_string(threadId) + " completed");
    }
    
    // Add a line to the TextBuffer
    void performAddLine(int threadId, int opIndex, OperationResult& result) {
        std::string line = "Thread " + std::to_string(threadId) + 
                          " Op " + std::to_string(opIndex) + ": " + 
                          randomGen_.generateRandomLine();
        
        result.description = "ADD_LINE: " + line;
        
        auto future = textBuffer_->requestAddLine(line);
        
        try {
            future.wait();
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
        } catch (const std::exception& e) {
            result.status = OperationResult::Status::FAILED;
            result.errorMessage = e.what();
            operationsFailed_++;
        }
    }
    
    // Insert a line at a random position
    void performInsertLine(int threadId, int opIndex, OperationResult& result) {
        size_t lineCount = textBuffer_->lineCount();
        size_t index = randomGen_.selectRandomLineIndex(lineCount + 1); // +1 to allow insertion at end
        
        std::string line = "Thread " + std::to_string(threadId) + 
                          " Op " + std::to_string(opIndex) + 
                          " INSERT: " + randomGen_.generateRandomLine();
        
        result.description = "INSERT_LINE at " + std::to_string(index) + ": " + line;
        
        auto future = textBuffer_->requestInsertLine(index, line);
        
        try {
            future.wait();
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
        } catch (const std::exception& e) {
            result.status = OperationResult::Status::FAILED;
            result.errorMessage = e.what();
            operationsFailed_++;
        }
    }
    
    // Delete a random line
    void performDeleteLine(int threadId, int opIndex, OperationResult& result) {
        size_t lineCount = textBuffer_->lineCount();
        if (lineCount <= 1) {
            // Don't delete the last line
            result.description = "DELETE_LINE skipped (buffer too small)";
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
            return;
        }
        
        size_t index = randomGen_.selectRandomLineIndex(lineCount);
        
        result.description = "DELETE_LINE at " + std::to_string(index);
        
        auto future = textBuffer_->requestDeleteLine(index);
        
        try {
            future.wait();
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
        } catch (const std::exception& e) {
            result.status = OperationResult::Status::FAILED;
            result.errorMessage = e.what();
            operationsFailed_++;
        }
    }
    
    // Replace a random line
    void performReplaceLine(int threadId, int opIndex, OperationResult& result) {
        size_t lineCount = textBuffer_->lineCount();
        if (lineCount == 0) {
            // Can't replace if there are no lines
            result.description = "REPLACE_LINE skipped (buffer empty)";
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
            return;
        }
        
        size_t index = randomGen_.selectRandomLineIndex(lineCount);
        
        std::string line = "Thread " + std::to_string(threadId) + 
                          " Op " + std::to_string(opIndex) + 
                          " REPLACE: " + randomGen_.generateRandomLine();
        
        result.description = "REPLACE_LINE at " + std::to_string(index) + ": " + line;
        
        auto future = textBuffer_->requestReplaceLine(index, line);
        
        try {
            future.wait();
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
        } catch (const std::exception& e) {
            result.status = OperationResult::Status::FAILED;
            result.errorMessage = e.what();
            operationsFailed_++;
        }
    }
    
    // Read a random line (doesn't modify the buffer)
    void performReadLine(int threadId, int opIndex, OperationResult& result) {
        size_t lineCount = textBuffer_->lineCount();
        if (lineCount == 0) {
            // Can't read if there are no lines
            result.description = "READ_LINE skipped (buffer empty)";
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
            return;
        }
        
        size_t index = randomGen_.selectRandomLineIndex(lineCount);
        
        result.description = "READ_LINE at " + std::to_string(index);
        
        try {
            std::string line = textBuffer_->getLine(index);
            result.status = OperationResult::Status::COMPLETED;
            operationsCompleted_++;
        } catch (const std::exception& e) {
            result.status = OperationResult::Status::FAILED;
            result.errorMessage = e.what();
            operationsFailed_++;
        }
    }
    
    // Verify that all operations completed successfully
    void verifyOperationResults(const std::vector<std::vector<OperationResult>>& results) {
        LOG_DEBUG("Verifying operation results");
        
        size_t totalOps = 0;
        size_t completedOps = 0;
        size_t failedOps = 0;
        size_t pendingOps = 0;
        
        for (const auto& threadResults : results) {
            for (const auto& result : threadResults) {
                totalOps++;
                
                switch (result.status) {
                    case OperationResult::Status::COMPLETED:
                        completedOps++;
                        break;
                    case OperationResult::Status::FAILED:
                        failedOps++;
                        LOG_ERROR("Failed operation: " + result.description + " - " + result.errorMessage);
                        break;
                    case OperationResult::Status::PENDING:
                        pendingOps++;
                        LOG_ERROR("Pending operation: " + result.description);
                        break;
                }
            }
        }
        
        LOG_DEBUG("Operation verification results:");
        LOG_DEBUG("  Total operations: " + std::to_string(totalOps));
        LOG_DEBUG("  Completed operations: " + std::to_string(completedOps));
        LOG_DEBUG("  Failed operations: " + std::to_string(failedOps));
        LOG_DEBUG("  Pending operations: " + std::to_string(pendingOps));
        
        // Assert that all operations completed or failed (none should be pending)
        assert(pendingOps == 0 && "Some operations are still pending");
        
        // Assert that the number of completed operations matches our counter
        assert(completedOps == operationsCompleted_ && 
               "Completed operations counter mismatch");
        
        // Assert that the number of failed operations matches our counter
        assert(failedOps == operationsFailed_ && 
               "Failed operations counter mismatch");
    }
    
    // Verify the integrity of the TextBuffer
    void verifyBufferIntegrity() {
        LOG_DEBUG("Verifying TextBuffer integrity");
        
        // Check that the buffer is not empty
        assert(textBuffer_->lineCount() > 0 && "TextBuffer should not be empty");
        
        // Check that we can read all lines without exceptions
        for (size_t i = 0; i < textBuffer_->lineCount(); ++i) {
            try {
                std::string line = textBuffer_->getLine(i);
                // Verify that the line is not empty (our test never adds empty lines)
                assert(!line.empty() && "Line should not be empty");
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to read line " + std::to_string(i) + ": " + e.what());
                assert(false && "Exception while reading line");
            }
        }
        
        LOG_DEBUG("TextBuffer integrity verified successfully");
    }
    
private:
    std::shared_ptr<TextBuffer> textBuffer_;
    std::unique_ptr<EditorCoreThreadPool> threadPool_;
    std::thread::id ownerThreadId_;
    std::atomic<bool> stopRequested_;
    std::atomic<size_t> operationsCompleted_;
    std::atomic<size_t> operationsFailed_;
    RandomGenerator randomGen_;
    std::mutex mutex_;
};

// Main function
int main() {
    try {
        TextBufferStressTest test;
        
        test.initialize();
        test.runTest();
        test.cleanup();
        
        std::cout << "TextBuffer stress test completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR" << std::endl;
        return 1;
    }
} 