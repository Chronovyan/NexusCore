#include "../src/ThreadSafeTextBuffer.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>

// Mutex for console output to prevent garbled text
std::mutex consoleMutex;

// Helper function for thread-safe console output
template<typename... Args>
void printThreadSafe(Args&&... args) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    (std::cout << ... << args) << std::endl;
}

// Function to read from the buffer in a loop
void readerFunction(ThreadSafeTextBuffer& buffer, int readerId, std::atomic<bool>& shouldStop) {
    printThreadSafe("Reader ", readerId, " started");
    
    int readCount = 0;
    while (!shouldStop.load(std::memory_order_acquire) && readCount < 100) {
        // Example 1: Safe individual method calls
        size_t lineCountSafe = buffer.lineCount();
        if (lineCountSafe > 0) {
            size_t randomLine = readCount % lineCountSafe;
            std::string line = buffer.getLine(randomLine);
            printThreadSafe("Reader ", readerId, " read line ", randomLine, ": ", line);
        }
        
        // Example 2: Using explicit locking for multiple operations
        buffer.lockForReading();
        try {
            size_t count = buffer.lineCount();
            std::vector<std::string> allLines = buffer.getAllLines();
            buffer.unlockReading();
            
            printThreadSafe("Reader ", readerId, " read all ", count, " lines using explicit locking");
        } catch (...) {
            // Always unlock in case of exceptions
            buffer.unlockReading();
            throw;
        }
        
        readCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    printThreadSafe("Reader ", readerId, " finished after ", readCount, " reads");
}

// Function to write to the buffer in a loop
void writerFunction(ThreadSafeTextBuffer& buffer, int writerId, std::atomic<bool>& shouldStop) {
    printThreadSafe("Writer ", writerId, " started");
    
    int writeCount = 0;
    while (!shouldStop.load(std::memory_order_acquire) && writeCount < 20) {
        // Example 1: Safe individual method calls
        std::string newLine = "Line written by writer " + std::to_string(writerId) + 
                              " at iteration " + std::to_string(writeCount);
        buffer.addLine(newLine);
        printThreadSafe("Writer ", writerId, " added new line: ", newLine);
        
        // Example 2: Using explicit locking for multiple operations
        buffer.lockForWriting();
        try {
            size_t lineCount = buffer.lineCount();
            if (lineCount > 3) {
                // Make a compound operation (delete line and add modified version)
                std::string line = buffer.getLine(lineCount - 3);
                buffer.deleteLine(lineCount - 3);
                buffer.addLine(line + " (modified by writer " + std::to_string(writerId) + ")");
                printThreadSafe("Writer ", writerId, " performed a compound modify operation");
            }
            buffer.unlockWriting();
        } catch (...) {
            // Always unlock in case of exceptions
            buffer.unlockWriting();
            throw;
        }
        
        writeCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    printThreadSafe("Writer ", writerId, " finished after ", writeCount, " writes");
}

int main() {
    // Create the thread-safe text buffer
    ThreadSafeTextBuffer buffer;
    
    // Add some initial content
    buffer.addLine("Initial line 1");
    buffer.addLine("Initial line 2");
    buffer.addLine("Initial line 3");
    
    std::cout << "Initial buffer state:" << std::endl;
    buffer.printToStream(std::cout);
    std::cout << std::endl;
    
    // Flag to signal threads to stop
    std::atomic<bool> shouldStop(false);
    
    // Create reader and writer threads
    std::vector<std::thread> threads;
    
    // Start 3 reader threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(readerFunction, std::ref(buffer), i, std::ref(shouldStop));
    }
    
    // Start 2 writer threads
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back(writerFunction, std::ref(buffer), i, std::ref(shouldStop));
    }
    
    // Let the threads run for a while
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Signal threads to stop
    shouldStop.store(true, std::memory_order_release);
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Print final buffer state
    std::cout << "\nFinal buffer state:" << std::endl;
    buffer.printToStream(std::cout);
    
    // Save to file
    std::string filename = "thread_safe_buffer_example_output.txt";
    if (buffer.saveToFile(filename)) {
        std::cout << "\nBuffer saved to " << filename << std::endl;
    } else {
        std::cout << "\nFailed to save buffer to " << filename << std::endl;
    }
    
    return 0;
} 