#include "EditorCoreThreadPool.h"
#include "TextBuffer.h"
#include "AppDebugLog.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>

using namespace std::chrono_literals;

// This demo shows how the EditorCoreThreadPool manages TextBuffer ownership
// and processes operations from multiple threads.

int main() {
    // Initialize logging
    LOG_INIT("EditorCoreThreadPoolDemo");
    LOG_DEBUG("Starting EditorCoreThreadPool demo");
    
    // Create a TextBuffer
    auto textBuffer = std::make_shared<TextBuffer>();
    
    // Add some initial content
    textBuffer->addLine("Line 1 - Initial content");
    textBuffer->addLine("Line 2 - Initial content");
    textBuffer->addLine("Line 3 - Initial content");
    
    LOG_DEBUG("Initial buffer content:");
    for (size_t i = 0; i < textBuffer->lineCount(); ++i) {
        LOG_DEBUG("  Line " + std::to_string(i) + ": " + textBuffer->getLine(i));
    }
    
    // Create the thread pool with 3 threads
    EditorCoreThreadPool threadPool(3);
    
    // Start the thread pool
    threadPool.start();
    
    // Assign TextBuffer ownership to a thread in the pool
    std::thread::id ownerThreadId = threadPool.assignTextBufferOwnership(textBuffer);
    LOG_DEBUG("TextBuffer ownership assigned to thread: " + std::to_string(
        std::hash<std::thread::id>{}(ownerThreadId)));
    
    // Create an atomic flag to signal threads to stop
    std::atomic<bool> stopThreads(false);
    
    // Create a thread that simulates a UI thread adding lines
    std::thread uiThread([&textBuffer, &threadPool, &stopThreads]() {
        LOG_DEBUG("UI thread started");
        int counter = 0;
        
        while (!stopThreads) {
            // Request to add a line
            std::string lineText = "Line added by UI thread - " + std::to_string(counter++);
            LOG_DEBUG("UI thread requesting to add: " + lineText);
            
            auto future = textBuffer->requestAddLine(lineText);
            
            // Notify the thread pool that operations are available
            threadPool.notifyTextBufferOperationsAvailable();
            
            // Wait for the operation to complete
            try {
                future.wait();
                LOG_DEBUG("UI thread: Line added successfully");
            }
            catch (const std::exception& e) {
                LOG_ERROR("UI thread: Failed to add line: " + std::string(e.what()));
            }
            
            // Sleep for a short time
            std::this_thread::sleep_for(100ms);
        }
        
        LOG_DEBUG("UI thread stopped");
    });
    
    // Create a thread that simulates a plugin thread modifying lines
    std::thread pluginThread([&textBuffer, &threadPool, &stopThreads]() {
        LOG_DEBUG("Plugin thread started");
        int counter = 0;
        
        while (!stopThreads) {
            // Get the current line count
            size_t lineCount = textBuffer->lineCount();
            
            if (lineCount > 0) {
                // Pick a line to modify
                size_t lineIndex = counter % lineCount;
                
                // Request to replace the line
                std::string lineText = "Line " + std::to_string(lineIndex) + 
                                      " modified by plugin thread - " + std::to_string(counter++);
                LOG_DEBUG("Plugin thread requesting to modify line " + std::to_string(lineIndex) + 
                         " to: " + lineText);
                
                auto future = textBuffer->requestReplaceLine(lineIndex, lineText);
                
                // Notify the thread pool that operations are available
                threadPool.notifyTextBufferOperationsAvailable();
                
                // Wait for the operation to complete
                try {
                    future.wait();
                    LOG_DEBUG("Plugin thread: Line " + std::to_string(lineIndex) + 
                             " modified successfully");
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Plugin thread: Failed to modify line: " + std::string(e.what()));
                }
            }
            
            // Sleep for a short time
            std::this_thread::sleep_for(150ms);
        }
        
        LOG_DEBUG("Plugin thread stopped");
    });
    
    // Create a thread that simulates an I/O thread reading the buffer
    std::thread ioThread([&textBuffer, &stopThreads]() {
        LOG_DEBUG("I/O thread started");
        
        while (!stopThreads) {
            // Get the current line count
            size_t lineCount = textBuffer->lineCount();
            
            // Read all lines (this is thread-safe and doesn't need the owner thread)
            LOG_DEBUG("I/O thread reading buffer content (" + std::to_string(lineCount) + 
                     " lines):");
            
            for (size_t i = 0; i < lineCount; ++i) {
                try {
                    LOG_DEBUG("  Line " + std::to_string(i) + ": " + textBuffer->getLine(i));
                }
                catch (const std::exception& e) {
                    LOG_ERROR("I/O thread: Failed to read line: " + std::string(e.what()));
                }
            }
            
            // Sleep for a short time
            std::this_thread::sleep_for(300ms);
        }
        
        LOG_DEBUG("I/O thread stopped");
    });
    
    // Submit a task to the thread pool
    threadPool.submitTask([]() {
        LOG_DEBUG("Task executed in thread pool");
    });
    
    // Run for a few seconds
    LOG_DEBUG("Main thread running for 5 seconds");
    std::this_thread::sleep_for(5s);
    
    // Signal threads to stop
    LOG_DEBUG("Signaling threads to stop");
    stopThreads = true;
    
    // Wait for threads to finish
    uiThread.join();
    pluginThread.join();
    ioThread.join();
    
    // Shut down the thread pool
    threadPool.shutdown();
    
    // Print the final buffer content
    LOG_DEBUG("Final buffer content:");
    for (size_t i = 0; i < textBuffer->lineCount(); ++i) {
        LOG_DEBUG("  Line " + std::to_string(i) + ": " + textBuffer->getLine(i));
    }
    
    LOG_DEBUG("EditorCoreThreadPool demo completed");
    return 0;
} 