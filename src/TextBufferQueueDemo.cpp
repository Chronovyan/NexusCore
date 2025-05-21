#include "TextBuffer.h"
#include "AppDebugLog.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace std::chrono_literals;

// This demo shows how the TextBuffer can be safely accessed from multiple threads
// using the operation queue.

int main() {
    // Initialize logging
    LOG_INIT("TextBufferQueueDemo");
    LOG_DEBUG("Starting TextBuffer queue demo");
    
    // Create a TextBuffer
    TextBuffer buffer;
    
    // Add some initial content
    buffer.addLine("Line 1 - Initial content");
    buffer.addLine("Line 2 - Initial content");
    buffer.addLine("Line 3 - Initial content");
    
    LOG_DEBUG("Initial buffer content:");
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        LOG_DEBUG("  Line " + std::to_string(i) + ": " + buffer.getLine(i));
    }
    
    // Set the main thread as the owner
    buffer.setOwnerThread(std::this_thread::get_id());
    
    // Create an atomic flag to signal threads to stop
    std::atomic<bool> stopThreads(false);
    
    // Create a thread that simulates a worker thread adding lines
    std::thread adderThread([&buffer, &stopThreads]() {
        LOG_DEBUG("Adder thread started");
        int counter = 0;
        
        while (!stopThreads) {
            // Request to add a line
            auto future = buffer.requestAddLine("Line added by worker thread - " + std::to_string(counter++));
            
            // Wait for the operation to complete
            try {
                future.wait();
                LOG_DEBUG("Adder thread: Line added successfully");
            }
            catch (const std::exception& e) {
                LOG_ERROR("Adder thread: Failed to add line: " + std::string(e.what()));
            }
            
            // Sleep for a short time
            std::this_thread::sleep_for(100ms);
        }
        
        LOG_DEBUG("Adder thread stopped");
    });
    
    // Create a thread that simulates a worker thread modifying lines
    std::thread modifierThread([&buffer, &stopThreads]() {
        LOG_DEBUG("Modifier thread started");
        int counter = 0;
        
        while (!stopThreads) {
            // Get the current line count
            size_t lineCount = buffer.lineCount();
            
            if (lineCount > 0) {
                // Pick a random line to modify
                size_t lineIndex = counter % lineCount;
                
                // Request to replace the line
                auto future = buffer.requestReplaceLine(lineIndex, 
                    "Line " + std::to_string(lineIndex) + " modified by worker thread - " + 
                    std::to_string(counter++));
                
                // Wait for the operation to complete
                try {
                    future.wait();
                    LOG_DEBUG("Modifier thread: Line " + std::to_string(lineIndex) + " modified successfully");
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Modifier thread: Failed to modify line: " + std::string(e.what()));
                }
            }
            
            // Sleep for a short time
            std::this_thread::sleep_for(150ms);
        }
        
        LOG_DEBUG("Modifier thread stopped");
    });
    
    // Create a thread that simulates a worker thread deleting lines
    std::thread deleterThread([&buffer, &stopThreads]() {
        LOG_DEBUG("Deleter thread started");
        
        while (!stopThreads) {
            // Get the current line count
            size_t lineCount = buffer.lineCount();
            
            if (lineCount > 3) {  // Keep at least 3 lines
                // Pick a random line to delete
                size_t lineIndex = rand() % lineCount;
                
                // Request to delete the line
                auto future = buffer.requestDeleteLine(lineIndex);
                
                // Wait for the operation to complete
                try {
                    future.wait();
                    LOG_DEBUG("Deleter thread: Line " + std::to_string(lineIndex) + " deleted successfully");
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Deleter thread: Failed to delete line: " + std::string(e.what()));
                }
            }
            
            // Sleep for a short time
            std::this_thread::sleep_for(200ms);
        }
        
        LOG_DEBUG("Deleter thread stopped");
    });
    
    // Main thread processes the operation queue
    LOG_DEBUG("Main thread processing operation queue");
    
    // Run for a few seconds
    for (int i = 0; i < 50; ++i) {
        // Process the operation queue
        size_t processedCount = buffer.processOperationQueue();
        
        if (processedCount > 0) {
            LOG_DEBUG("Main thread processed " + std::to_string(processedCount) + " operations");
            
            // Print the current buffer content
            LOG_DEBUG("Current buffer content:");
            for (size_t j = 0; j < buffer.lineCount(); ++j) {
                LOG_DEBUG("  Line " + std::to_string(j) + ": " + buffer.getLine(j));
            }
        }
        
        // Sleep for a short time
        std::this_thread::sleep_for(50ms);
    }
    
    // Signal threads to stop
    LOG_DEBUG("Signaling threads to stop");
    stopThreads = true;
    
    // Wait for threads to finish
    adderThread.join();
    modifierThread.join();
    deleterThread.join();
    
    // Process any remaining operations
    size_t processedCount = buffer.processOperationQueue();
    LOG_DEBUG("Processed " + std::to_string(processedCount) + " remaining operations");
    
    // Print the final buffer content
    LOG_DEBUG("Final buffer content:");
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        LOG_DEBUG("  Line " + std::to_string(i) + ": " + buffer.getLine(i));
    }
    
    LOG_DEBUG("TextBuffer queue demo completed");
    return 0;
} 