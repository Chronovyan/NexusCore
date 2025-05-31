#include "../src/ThreadSafeTextBuffer.h"
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <future>
#include <chrono>

class ThreadSafeTextBufferTest : public ::testing::Test {
protected:
    ThreadSafeTextBuffer buffer;
    
    void SetUp() override {
        // Initialize with some content
        buffer.addLine("Line 1");
        buffer.addLine("Line 2");
        buffer.addLine("Line 3");
    }
};

// Basic functionality tests

TEST_F(ThreadSafeTextBufferTest, BasicOperations) {
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 1");
    EXPECT_EQ(buffer.getLine(1), "Line 2");
    EXPECT_EQ(buffer.getLine(2), "Line 3");
    
    buffer.addLine("Line 4");
    EXPECT_EQ(buffer.lineCount(), 4);
    EXPECT_EQ(buffer.getLine(3), "Line 4");
    
    buffer.replaceLine(0, "New Line 1");
    EXPECT_EQ(buffer.getLine(0), "New Line 1");
    
    buffer.deleteLine(1);
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(1), "Line 3");
}

TEST_F(ThreadSafeTextBufferTest, ExplicitLocking) {
    // Test explicit locking for reading
    buffer.lockForReading();
    size_t count = buffer.lineCount();
    std::vector<std::string> lines;
    for (size_t i = 0; i < count; i++) {
        lines.push_back(buffer.getLine(i));
    }
    buffer.unlockReading();
    
    EXPECT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "Line 1");
    EXPECT_EQ(lines[1], "Line 2");
    EXPECT_EQ(lines[2], "Line 3");
    
    // Test explicit locking for writing
    buffer.lockForWriting();
    buffer.clear(true);
    buffer.addLine("New content");
    buffer.unlockWriting();
    
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "New content");
}

// Thread safety tests

TEST_F(ThreadSafeTextBufferTest, ConcurrentReaders) {
    const int numThreads = 10;
    const int numReadsPerThread = 1000;
    
    auto readerFunc = [&]() {
        for (int i = 0; i < numReadsPerThread; i++) {
            size_t count = buffer.lineCount();
            if (count > 0) {
                size_t randomLine = i % count;
                std::string line = buffer.getLine(randomLine);
                // Just verify we can read without crashing
                EXPECT_FALSE(line.empty());
            }
        }
        return true;
    };
    
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < numThreads; i++) {
        futures.push_back(std::async(std::launch::async, readerFunc));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        EXPECT_TRUE(future.get());
    }
}

TEST_F(ThreadSafeTextBufferTest, ConcurrentWriters) {
    const int numThreads = 5;
    const int numWritesPerThread = 100;
    
    auto writerFunc = [&](int threadId) {
        for (int i = 0; i < numWritesPerThread; i++) {
            std::string line = "Line from thread " + std::to_string(threadId) + 
                              " iteration " + std::to_string(i);
            buffer.addLine(line);
            
            // Also test other write operations
            if (i % 10 == 0 && buffer.lineCount() > 1) {
                size_t randomLine = (threadId + i) % buffer.lineCount();
                buffer.replaceLine(randomLine, line + " (replaced)");
            }
            
            if (i % 15 == 0 && buffer.lineCount() > 1) {
                size_t randomLine = (threadId + i) % buffer.lineCount();
                buffer.deleteLine(randomLine);
            }
        }
        return true;
    };
    
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < numThreads; i++) {
        futures.push_back(std::async(std::launch::async, writerFunc, i));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        EXPECT_TRUE(future.get());
    }
    
    // Verify the final state is reasonable
    EXPECT_GT(buffer.lineCount(), 0);
}

TEST_F(ThreadSafeTextBufferTest, ConcurrentReadersAndWriters) {
    const int numReaders = 8;
    const int numWriters = 4;
    const int numOperationsPerThread = 200;
    std::atomic<bool> stop{false};
    
    auto readerFunc = [&]() {
        for (int i = 0; i < numOperationsPerThread && !stop.load(); i++) {
            if (i % 5 == 0) {
                // Test compound read operation with explicit locking
                buffer.lockForReading();
                try {
                    size_t count = buffer.lineCount();
                    std::vector<std::string> allLines;
                    for (size_t j = 0; j < count; j++) {
                        allLines.push_back(buffer.getLine(j));
                    }
                    buffer.unlockReading();
                } catch (...) {
                    buffer.unlockReading();
                    throw;
                }
            } else {
                // Test individual read operations
                size_t count = buffer.lineCount();
                if (count > 0) {
                    size_t randomLine = i % count;
                    if (randomLine < count) {
                        std::string line = buffer.getLine(randomLine);
                    }
                }
            }
            
            // Sleep a bit to allow for interleaving
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    };
    
    auto writerFunc = [&](int threadId) {
        for (int i = 0; i < numOperationsPerThread && !stop.load(); i++) {
            if (i % 10 == 0) {
                // Test compound write operation with explicit locking
                buffer.lockForWriting();
                try {
                    std::string line = "Compound from writer " + std::to_string(threadId);
                    buffer.addLine(line);
                    if (buffer.lineCount() > 1) {
                        buffer.deleteLine(0);
                    }
                    buffer.unlockWriting();
                } catch (...) {
                    buffer.unlockWriting();
                    throw;
                }
            } else {
                // Test individual write operations
                std::string line = "Line from writer " + std::to_string(threadId) + 
                                   " iter " + std::to_string(i);
                buffer.addLine(line);
                
                // Mix in other operations occasionally
                if (i % 5 == 0 && buffer.lineCount() > 1) {
                    size_t randomLine = (threadId + i) % buffer.lineCount();
                    if (randomLine < buffer.lineCount()) {
                        buffer.replaceLine(randomLine, line + " (replaced)");
                    }
                }
            }
            
            // Sleep a bit to allow for interleaving
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return true;
    };
    
    // Start reader threads
    std::vector<std::future<bool>> readerFutures;
    for (int i = 0; i < numReaders; i++) {
        readerFutures.push_back(std::async(std::launch::async, readerFunc));
    }
    
    // Start writer threads
    std::vector<std::future<bool>> writerFutures;
    for (int i = 0; i < numWriters; i++) {
        writerFutures.push_back(std::async(std::launch::async, writerFunc, i));
    }
    
    // Wait for a while to let threads run concurrently
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Signal stop and wait for all threads to complete
    stop.store(true);
    
    for (auto& future : readerFutures) {
        EXPECT_TRUE(future.get());
    }
    
    for (auto& future : writerFutures) {
        EXPECT_TRUE(future.get());
    }
}

// Test for race conditions in modified flag
TEST_F(ThreadSafeTextBufferTest, ModifiedFlagThreadSafety) {
    const int numThreads = 8;
    const int numOperationsPerThread = 100;
    
    auto toggleModifiedFunc = [&](int threadId) {
        for (int i = 0; i < numOperationsPerThread; i++) {
            bool expected = (threadId + i) % 2 == 0;
            buffer.setModified(expected);
            EXPECT_EQ(buffer.isModified(), expected);
            
            // Let other threads have a chance
            std::this_thread::yield();
        }
        return true;
    };
    
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < numThreads; i++) {
        futures.push_back(std::async(std::launch::async, toggleModifiedFunc, i));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        EXPECT_TRUE(future.get());
    }
}

// Test for proper handling of references
TEST_F(ThreadSafeTextBufferTest, ReferenceHandling) {
    // This test demonstrates the issue with returning references
    // It's not really a correctness test, but more of a demonstration
    
    // First set up a line we can get a reference to
    buffer.setLine(0, "Original line");
    
    // Get a reference to the line
    const std::string& lineRef = buffer.getLine(0);
    EXPECT_EQ(lineRef, "Original line");
    
    // Modify the buffer in another thread
    std::thread modifierThread([&]() {
        buffer.setLine(0, "Modified line");
    });
    modifierThread.join();
    
    // Now the reference should reflect the new value
    // (This is actually implementation-dependent and potentially unsafe!)
    EXPECT_EQ(buffer.getLine(0), "Modified line");
    
    // The preferred safe approach is to make a copy
    std::string lineCopy = buffer.getLine(0);
    
    // Modify again in another thread
    std::thread modifierThread2([&]() {
        buffer.setLine(0, "Modified again");
    });
    modifierThread2.join();
    
    // The copy remains unchanged
    EXPECT_EQ(lineCopy, "Modified line");
    EXPECT_EQ(buffer.getLine(0), "Modified again");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 