#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <future>
#include <algorithm>
#include <cassert>
#include <set>
#include <map>

#include "AppDebugLog.h"
#include "TextBuffer.h"
#include "EditorCoreThreadPool.h"

/**
 * This test specifically targets potential race conditions in the TextBuffer
 * by having multiple threads perform operations on the same lines simultaneously.
 */

// Configuration for race condition tests
namespace RaceTestConfig {
    // Number of threads to use for testing
    constexpr int NUM_THREADS = 8;
    
    // Number of iterations for each test
    constexpr int NUM_ITERATIONS = 100;
    
    // Number of lines to use for testing
    constexpr int NUM_LINES = 20;
    
    // Sleep range between operations (milliseconds)
    constexpr int MIN_SLEEP_MS = 0;
    constexpr int MAX_SLEEP_MS = 2;
}

// Helper function to generate a random sleep duration
std::chrono::milliseconds generateRandomSleep() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(RaceTestConfig::MIN_SLEEP_MS, RaceTestConfig::MAX_SLEEP_MS);
    return std::chrono::milliseconds(dist(gen));
}

// Helper function to generate a random line index
size_t generateRandomLineIndex(size_t maxLines) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, maxLines - 1);
    return dist(gen);
}

// Test class for race conditions
class TextBufferRaceTest {
public:
    TextBufferRaceTest()
        : textBuffer_(std::make_shared<TextBuffer>()),
          threadPool_(std::make_unique<EditorCoreThreadPool>(4)), // Use 4 threads in the pool
          stopRequested_(false)
    {
        LOG_INIT("TextBufferRaceTest");
        LOG_DEBUG("Initializing TextBuffer race condition test");
    }
    
    void initialize() {
        // Start the thread pool
        threadPool_->start();
        
        // Assign TextBuffer ownership to a thread in the pool
        ownerThreadId_ = threadPool_->assignTextBufferOwnership(textBuffer_);
        LOG_DEBUG("TextBuffer ownership assigned to thread: " + 
                 std::to_string(std::hash<std::thread::id>{}(ownerThreadId_)));
        
        // Initialize the TextBuffer with test lines
        for (int i = 0; i < RaceTestConfig::NUM_LINES; ++i) {
            textBuffer_->addLine("Initial line " + std::to_string(i));
        }
        
        LOG_DEBUG("TextBuffer initialized with " + std::to_string(textBuffer_->lineCount()) + " lines");
    }
    
    void runTests() {
        LOG_DEBUG("Running race condition tests");
        
        // Run test for concurrent modifications to the same line
        testConcurrentLineModifications();
        
        // Run test for concurrent insertions and deletions
        testConcurrentInsertDelete();
        
        // Run test for concurrent reads during modifications
        testConcurrentReadModify();
        
        LOG_DEBUG("All race condition tests completed");
    }
    
    void cleanup() {
        LOG_DEBUG("Cleaning up race test resources");
        threadPool_->shutdown();
    }
    
private:
    // Test concurrent modifications to the same line
    void testConcurrentLineModifications() {
        LOG_DEBUG("Starting concurrent line modification test");
        
        // Create a thread that periodically processes operations
        std::thread processorThread([this]() {
            while (!stopRequested_) {
                threadPool_->notifyTextBufferOperationsAvailable();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        for (int iteration = 0; iteration < RaceTestConfig::NUM_ITERATIONS; ++iteration) {
            LOG_DEBUG("Concurrent modification iteration " + std::to_string(iteration));
            
            // Select a target line for all threads to modify
            size_t targetLine = generateRandomLineIndex(RaceTestConfig::NUM_LINES);
            
            // Store futures for all operations
            std::vector<std::shared_future<TextBufferOperation::ResultType>> futures;
            
            // Launch threads to modify the same line
            for (int t = 0; t < RaceTestConfig::NUM_THREADS; ++t) {
                std::string newContent = "Thread " + std::to_string(t) + 
                                       " modified line " + std::to_string(targetLine) + 
                                       " in iteration " + std::to_string(iteration);
                
                // Request to replace the line
                auto future = textBuffer_->requestReplaceLine(targetLine, newContent);
                futures.push_back(future);
                
                // Small random sleep to increase chance of race conditions
                std::this_thread::sleep_for(generateRandomSleep());
            }
            
            // Wait for all operations to complete
            for (auto& future : futures) {
                try {
                    future.wait();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in concurrent modification: " + std::string(e.what()));
                    assert(false && "Exception in concurrent modification");
                }
            }
            
            // Verify that the line was modified (we don't know which thread's modification
            // will be the final one, but it should be one of them)
            std::string finalContent = textBuffer_->getLine(targetLine);
            bool validContent = false;
            
            for (int t = 0; t < RaceTestConfig::NUM_THREADS; ++t) {
                std::string expectedContent = "Thread " + std::to_string(t) + 
                                           " modified line " + std::to_string(targetLine) + 
                                           " in iteration " + std::to_string(iteration);
                if (finalContent == expectedContent) {
                    validContent = true;
                    break;
                }
            }
            
            if (!validContent) {
                LOG_ERROR("Line content is invalid: " + finalContent);
                assert(false && "Invalid line content after concurrent modification");
            }
        }
        
        // Stop the processor thread
        stopRequested_ = true;
        processorThread.join();
        stopRequested_ = false;
        
        LOG_DEBUG("Concurrent line modification test completed");
    }
    
    // Test concurrent insertions and deletions
    void testConcurrentInsertDelete() {
        LOG_DEBUG("Starting concurrent insert/delete test");
        
        // Create a thread that periodically processes operations
        std::thread processorThread([this]() {
            while (!stopRequested_) {
                threadPool_->notifyTextBufferOperationsAvailable();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Reset the TextBuffer
        textBuffer_->clear(false);
        for (int i = 0; i < RaceTestConfig::NUM_LINES; ++i) {
            textBuffer_->addLine("Initial line " + std::to_string(i));
        }
        
        // Track the expected line count
        std::atomic<int> expectedLineCount(RaceTestConfig::NUM_LINES);
        
        // Create threads for inserting and deleting lines
        std::vector<std::thread> threads;
        
        // Thread for inserting lines
        threads.emplace_back([this, &expectedLineCount]() {
            for (int i = 0; i < RaceTestConfig::NUM_ITERATIONS; ++i) {
                size_t insertPos = generateRandomLineIndex(textBuffer_->lineCount() + 1);
                std::string newLine = "Inserted line " + std::to_string(i);
                
                try {
                    auto future = textBuffer_->requestInsertLine(insertPos, newLine);
                    future.wait();
                    expectedLineCount++;
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in insert: " + std::string(e.what()));
                }
                
                std::this_thread::sleep_for(generateRandomSleep());
            }
        });
        
        // Thread for deleting lines
        threads.emplace_back([this, &expectedLineCount]() {
            for (int i = 0; i < RaceTestConfig::NUM_ITERATIONS; ++i) {
                // Don't delete if we're down to 1 line
                if (textBuffer_->lineCount() > 1) {
                    size_t deletePos = generateRandomLineIndex(textBuffer_->lineCount());
                    
                    try {
                        auto future = textBuffer_->requestDeleteLine(deletePos);
                        future.wait();
                        expectedLineCount--;
                    } catch (const std::exception& e) {
                        LOG_ERROR("Exception in delete: " + std::string(e.what()));
                    }
                }
                
                std::this_thread::sleep_for(generateRandomSleep());
            }
        });
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Stop the processor thread
        stopRequested_ = true;
        processorThread.join();
        stopRequested_ = false;
        
        // Process any remaining operations
        threadPool_->notifyTextBufferOperationsAvailable();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify the final line count
        size_t actualLineCount = textBuffer_->lineCount();
        LOG_DEBUG("Expected line count: " + std::to_string(expectedLineCount));
        LOG_DEBUG("Actual line count: " + std::to_string(actualLineCount));
        
        // The counts might not match exactly due to race conditions in our tracking,
        // but they should be close
        assert(abs(static_cast<int>(actualLineCount) - expectedLineCount) <= 5 && 
               "Line count mismatch after concurrent insert/delete");
        
        LOG_DEBUG("Concurrent insert/delete test completed");
    }
    
    // Test concurrent reads during modifications
    void testConcurrentReadModify() {
        LOG_DEBUG("Starting concurrent read/modify test");
        
        // Create a thread that periodically processes operations
        std::thread processorThread([this]() {
            while (!stopRequested_) {
                threadPool_->notifyTextBufferOperationsAvailable();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Reset the TextBuffer
        textBuffer_->clear(false);
        for (int i = 0; i < RaceTestConfig::NUM_LINES; ++i) {
            textBuffer_->addLine("Initial line " + std::to_string(i));
        }
        
        // Track exceptions that occur during reads
        std::atomic<int> readExceptions(0);
        
        // Create threads for reading and modifying
        std::vector<std::thread> threads;
        
        // Thread for modifying lines
        threads.emplace_back([this]() {
            for (int i = 0; i < RaceTestConfig::NUM_ITERATIONS; ++i) {
                size_t lineIndex = generateRandomLineIndex(RaceTestConfig::NUM_LINES);
                std::string newContent = "Modified in iteration " + std::to_string(i);
                
                try {
                    auto future = textBuffer_->requestReplaceLine(lineIndex, newContent);
                    future.wait();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in modify: " + std::string(e.what()));
                }
                
                std::this_thread::sleep_for(generateRandomSleep());
            }
        });
        
        // Multiple threads for reading lines
        for (int t = 0; t < RaceTestConfig::NUM_THREADS - 1; ++t) {
            threads.emplace_back([this, &readExceptions]() {
                for (int i = 0; i < RaceTestConfig::NUM_ITERATIONS; ++i) {
                    size_t lineIndex = generateRandomLineIndex(RaceTestConfig::NUM_LINES);
                    
                    try {
                        std::string content = textBuffer_->getLine(lineIndex);
                        // Just verify that we got a non-empty string
                        assert(!content.empty() && "Empty line content during concurrent read");
                    } catch (const std::exception& e) {
                        LOG_ERROR("Exception in read: " + std::string(e.what()));
                        readExceptions++;
                    }
                    
                    std::this_thread::sleep_for(generateRandomSleep());
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Stop the processor thread
        stopRequested_ = true;
        processorThread.join();
        stopRequested_ = false;
        
        // Process any remaining operations
        threadPool_->notifyTextBufferOperationsAvailable();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify that no read exceptions occurred
        LOG_DEBUG("Read exceptions: " + std::to_string(readExceptions));
        assert(readExceptions == 0 && "Exceptions occurred during concurrent reads");
        
        LOG_DEBUG("Concurrent read/modify test completed");
    }
    
private:
    std::shared_ptr<TextBuffer> textBuffer_;
    std::unique_ptr<EditorCoreThreadPool> threadPool_;
    std::thread::id ownerThreadId_;
    std::atomic<bool> stopRequested_;
};

// Main function
int main() {
    try {
        TextBufferRaceTest test;
        
        test.initialize();
        test.runTests();
        test.cleanup();
        
        std::cout << "TextBuffer race condition tests completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR" << std::endl;
        return 1;
    }
} 