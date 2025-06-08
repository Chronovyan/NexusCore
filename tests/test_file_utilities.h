#pragma once

#include <string>
#include <fstream>
#include <random>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <functional>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <mach/task.h>
    #include <mach/task_info.h>
#else
    // Linux
    #include <unistd.h>
    #include <cstdio>
    #include <sys/resource.h>
#endif

namespace TestFileGenerator {

    /**
     * Defines different content patterns for generated test files
     */
    enum class ContentPattern {
        SEQUENTIAL_NUMBERS,   // Lines with sequential numbers (1, 2, 3...)
        REPEATED_TEXT,        // Repeating standard text snippets
        RANDOM_TEXT,          // Random ASCII text with varying lengths
        CODE_LIKE,            // C++-like code patterns
        MIXED_LINE_LENGTHS,   // Lines with different lengths (5, 10, 20, 50, 100...)
        MIXED_LINE_ENDINGS    // Mix of CR, LF, and CRLF line endings
    };
    
    /**
     * Defines line ending types for generated test files
     */
    enum class LineEnding {
        LF,      // Unix-style \n
        CR,      // MacOS 9 style \r
        CRLF,    // Windows-style \r\n
        MIXED    // Mix of all types
    };
    
    /**
     * Generate a file of specified size with given pattern and line ending
     * 
     * @param sizeInBytes Desired approximate file size in bytes
     * @param filename Path where the file will be created
     * @param pattern Content pattern type to generate
     * @param lineEnding Line ending type to use
     * @return The filename if successful
     * @throws std::runtime_error if file creation fails
     */
    std::string generateFile(
        size_t sizeInBytes, 
        const std::string& filename,
        ContentPattern pattern = ContentPattern::REPEATED_TEXT,
        LineEnding lineEnding = LineEnding::LF
    );

} // namespace TestFileGenerator

namespace MemoryTracker {
    
    /**
     * Get the current memory usage of the process in bytes
     * 
     * @return Memory usage in bytes or 0 if measurement fails
     */
    size_t getCurrentMemoryUsage();
    
    /**
     * Track peak memory usage during execution of an operation
     * 
     * @param operation Function to execute and monitor
     * @return Peak memory usage in bytes observed during operation
     */
    template<typename Func>
    size_t trackPeakMemoryDuring(Func&& operation) {
        size_t peakMemory = getCurrentMemoryUsage();
        
        // Start a background thread to monitor memory
        std::atomic<bool> keepRunning(true);
        std::thread memoryMonitor([&peakMemory, &keepRunning]() {
            while (keepRunning) {
                size_t currentMemory = getCurrentMemoryUsage();
                if (currentMemory > peakMemory) {
                    peakMemory = currentMemory;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Execute the operation
        operation();
        
        // Stop the monitoring thread
        keepRunning = false;
        if (memoryMonitor.joinable()) {
            memoryMonitor.join();
        }
        
        return peakMemory;
    }
    
} // namespace MemoryTracker 