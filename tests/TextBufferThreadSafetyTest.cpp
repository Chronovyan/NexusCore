#include "gtest/gtest.h"
#include "../src/TextBuffer.h"
#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>

class TextBufferThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<TextBuffer>();
        // Initialize with some content
        buffer->addLine("Line 1");
        buffer->addLine("Line 2");
        buffer->addLine("Line 3");
    }

    std::unique_ptr<TextBuffer> buffer;
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
};

// Test basic thread ownership functionality
TEST_F(TextBufferThreadSafetyTest, SetOwnerThreadWorks) {
    std::thread::id main_thread_id = std::this_thread::get_id();
    buffer->setOwnerThread(main_thread_id);
    
    // Should be able to modify from owner thread
    EXPECT_NO_THROW(buffer->addLine("Added from owner thread"));
    
    // Should not be able to modify from another thread
    std::thread other_thread([&]() {
        EXPECT_THROW(buffer->addLine("Should fail"), std::runtime_error);
    });
    other_thread.join();
}

// Test concurrent reads from multiple threads
TEST_F(TextBufferThreadSafetyTest, ConcurrentReads) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // Create multiple reader threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            // All threads try to read simultaneously
            auto lines = buffer->getAllLines();
            if (!lines.empty() && lines[0] == "Line 1") {
                success_count++;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count, num_threads);
}

// Test operation queue processing
TEST_F(TextBufferThreadSafetyTest, ProcessOperationQueue) {
    // Set up a non-owner thread to queue operations
    std::thread worker_thread([&]() {
        // These operations should be queued, not executed immediately
        buffer->addLine("Queued line 1");
        buffer->addLine("Queued line 2");
    });
    
    // Let the worker thread queue some operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify operations haven't been applied yet
    EXPECT_EQ(buffer->lineCount(), 3); // Original 3 lines
    
    // Process the operation queue from the owner thread
    size_t processed = buffer->processOperationQueue();
    
    // Verify operations were processed
    EXPECT_EQ(processed, 2);
    EXPECT_EQ(buffer->lineCount(), 5);
    EXPECT_EQ(buffer->getLine(3), "Queued line 1");
    EXPECT_EQ(buffer->getLine(4), "Queued line 2");
    
    worker_thread.join();
}

// Test concurrent modifications from multiple threads with operation queue
TEST_F(TextBufferThreadSafetyTest, ConcurrentModifications) {
    const int num_threads = 5;
    std::vector<std::thread> threads;
    
    // Create multiple writer threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i]() {
            // Each thread adds a line with its ID
            buffer->addLine("Thread " + std::to_string(i) + " line");
        });
    }
    
    // Let threads queue their operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Process all queued operations
    size_t processed = buffer->processOperationQueue();
    
    // Verify all operations were processed
    EXPECT_EQ(processed, num_threads);
    EXPECT_EQ(buffer->lineCount(), 3 + num_threads);
    
    // Clean up
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

// Test that operations are processed in order
TEST_F(TextBufferThreadSafetyTest, OperationOrdering) {
    // Queue operations from a non-owner thread
    std::thread worker_thread([&]() {
        buffer->insertText(0, 0, "START ");
        buffer->addLine("MIDDLE");
        buffer->insertText(0, 0, "BEGIN ");
    });
    
    // Process the operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    size_t processed = buffer->processOperationQueue();
    EXPECT_EQ(processed, 3);
    
    // Verify operations were applied in the correct order
    EXPECT_EQ(buffer->getLine(0), "BEGIN START Line 1");
    EXPECT_EQ(buffer->getLine(3), "MIDDLE");
    
    worker_thread.join();
}

// Test error handling for invalid thread access
TEST_F(TextBufferThreadSafetyTest, ThreadSafetyViolationThrows) {
    buffer->setOwnerThread(std::this_thread::get_id());
    
    // Try to modify from a different thread
    std::thread other_thread([&]() {
        EXPECT_THROW({
            try {
                buffer->addLine("Should throw");
            } catch (const std::runtime_error& e) {
                EXPECT_NE(std::string(e.what()).find("thread"), std::string::npos);
                throw;
            }
        }, std::runtime_error);
    });
    
    other_thread.join();
}

// Test that read operations don't require thread ownership
TEST_F(TextBufferThreadSafetyTest, ReadOperationsDontRequireOwnership) {
    buffer->setOwnerThread(std::this_thread::get_id());
    
    std::thread other_thread([&]() {
        // Read operations should work from any thread
        EXPECT_NO_THROW({
            auto line = buffer->getLine(0);
            auto count = buffer->lineCount();
            EXPECT_GT(count, 0);
        });
    });
    
    other_thread.join();
}

// Test concurrent reads during modification
TEST_F(TextBufferThreadSafetyTest, ConcurrentReadsDuringModification) {
    const int num_readers = 5;
    std::vector<std::thread> readers;
    std::atomic<bool> stop_readers{false};
    std::atomic<int> read_count{0};
    
    // Start reader threads
    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&]() {
            while (!stop_readers) {
                try {
                    auto lines = buffer->getAllLines();
                    if (!lines.empty()) {
                        read_count++;
                    }
                } catch (...) {
                    // Ignore any exceptions during concurrent access
                }
                std::this_thread::yield();
            }
        });
    }
    
    // Perform modifications from owner thread
    for (int i = 0; i < 10; ++i) {
        buffer->addLine("Modification " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Stop readers
    stop_readers = true;
    for (auto& t : readers) {
        if (t.joinable()) t.join();
    }
    
    // Verify we got at least some reads through
    EXPECT_GT(read_count, 0);
}
