#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <atomic>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

#include "gtest/gtest.h"
#include "../src/TextBuffer.h"

// Define our own min/max functions to avoid issues with Windows macros
template<typename T>
T our_min(T a, T b) {
    return (a < b) ? a : b;
}

template<typename T>
T our_max(T a, T b) {
    return (a > b) ? a : b;
}

// Define our own syntax styling structures to avoid using the ones from SyntaxHighlighter.h
struct TestSyntaxStyle {
    size_t startCol;
    size_t endCol;
    int color;
    
    TestSyntaxStyle(size_t start, size_t end, int c)
        : startCol(start), endCol(end), color(c) {}
};

// Memory usage tracking utility class with RAII principles
class MemoryTracker {
public:
    MemoryTracker() : startMemoryUsage_(getCurrentMemoryUsageMB()) {}
    
    ~MemoryTracker() = default;
    
    // Get memory usage difference since tracker creation
    double getMemoryDeltaMB() const {
        double currentUsage = getCurrentMemoryUsageMB();
        return currentUsage > startMemoryUsage_ ? 
               currentUsage - startMemoryUsage_ : 0.0;
    }
    
    // Get peak memory usage difference since tracker creation
    double getPeakMemoryDeltaMB() const {
        return peakMemoryDelta_;
    }
    
    // Update peak memory if current usage is higher
    void updatePeakMemory() {
        double currentDelta = getMemoryDeltaMB();
        if (currentDelta > peakMemoryDelta_) {
            peakMemoryDelta_ = currentDelta;
        }
    }
    
private:
    double startMemoryUsage_;
    double peakMemoryDelta_ = 0.0;
    
    // Cross-platform implementation of memory usage tracking
    static double getCurrentMemoryUsageMB() {
        double memoryInMB = 0.0;
        
        #ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                                 reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), 
                                 sizeof(pmc))) {
            // Using WorkingSetSize as it represents current physical memory usage
            memoryInMB = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
        }
        #else
        // For Unix-like systems, prefer getrusage over /proc/self/statm for portability
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            // ru_maxrss is in kilobytes on most systems
            memoryInMB = static_cast<double>(usage.ru_maxrss) / 1024.0;
        } else {
            // Fallback to /proc filesystem on Linux
            #ifdef __linux__
            std::ifstream statm("/proc/self/statm");
            if (statm.is_open()) {
                long size, resident;
                statm >> size >> resident;
                statm.close();
                
                // Convert to MB (resident * page size)
                long page_size = sysconf(_SC_PAGESIZE);
                memoryInMB = static_cast<double>(resident * page_size) / (1024.0 * 1024.0);
            }
            #endif
        }
        #endif
        
        return memoryInMB;
    }
};

// Mock syntax highlighter for testing
class MockSyntaxHighlighter {
public:
    std::vector<TestSyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) const {
        // Simulate syntax highlighting computation with random styles
        std::vector<TestSyntaxStyle> styles;
        
        // Add some delay to simulate real highlighting work
        std::this_thread::sleep_for(std::chrono::microseconds(10 + line.length() / 10));
        
        if (line.empty()) {
            return styles;
        }
        
        // Create some random styles
        std::mt19937 gen(42 + static_cast<unsigned int>(line.length())); // Fixed seed for reproducibility
        std::uniform_int_distribution<> styleDist(0, 5);
        
        int maxLen = static_cast<int>(line.length() / 5);
        if (maxLen < 1) maxLen = 1;
        std::uniform_int_distribution<> lengthDist(1, maxLen);
        
        size_t pos = 0;
        while (pos < line.length()) {
            size_t length = lengthDist(gen);
            if (pos + length > line.length()) {
                length = line.length() - pos;
            }
            
            int color = styleDist(gen);
            
            // Use emplace_back to create the SyntaxStyle directly
            styles.emplace_back(pos, pos + length, color);
            pos += length;
        }
        
        return styles;
    }
    
    std::vector<std::vector<TestSyntaxStyle>> highlightBuffer(const TextBuffer& buffer) const {
        std::vector<std::vector<TestSyntaxStyle>> result;
        result.reserve(buffer.lineCount());
        
        for (size_t i = 0; i < buffer.lineCount(); ++i) {
            result.push_back(highlightLine(buffer.getLine(i), i));
        }
        return result;
    }
    
    std::vector<std::string> getSupportedExtensions() const {
        return {"txt", "mock"};
    }
    
    std::string getLanguageName() const {
        return "MockLanguage";
    }
};

// Simple syntax highlighting manager for tests
class TestSyntaxHighlightingManager {
public:
    void setBuffer(std::shared_ptr<TextBuffer> buffer) {
        buffer_ = buffer;
    }
    
    void setHighlighter(std::shared_ptr<MockSyntaxHighlighter> highlighter) {
        highlighter_ = highlighter;
    }
    
    void setEnabled(bool enabled) {
        enabled_ = enabled;
    }
    
    void setVisibleRange(size_t startLine, size_t endLine) {
        visibleStartLine_ = startLine;
        visibleEndLine_ = endLine;
    }
    
    void invalidateLine(size_t line) {
        // Simulate invalidating a line for reprocessing
    }
    
    std::vector<std::vector<TestSyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const {
        if (!buffer_ || !highlighter_ || !enabled_) {
            return {};
        }
        
        std::vector<std::vector<TestSyntaxStyle>> result;
        for (size_t i = startLine; i <= endLine && i < buffer_->lineCount(); ++i) {
            result.push_back(highlighter_->highlightLine(buffer_->getLine(i), i));
        }
        return result;
    }
    
private:
    std::shared_ptr<TextBuffer> buffer_;
    std::shared_ptr<MockSyntaxHighlighter> highlighter_;
    bool enabled_ = false;
    size_t visibleStartLine_ = 0;
    size_t visibleEndLine_ = 0;
};

// Helper class for benchmarking
class HighlightingBenchmark {
public:
    struct BenchmarkResult {
        double totalTimeMs;
        double avgTimePerLineMs;
        double peakMemoryUsageMB;
        size_t totalLines;
        size_t totalStyles;
        
        void print(std::ostream& os) const {
            os << "Benchmark Results:\n";
            os << "  Total time: " << std::fixed << std::setprecision(2) << totalTimeMs << " ms\n";
            os << "  Avg time per line: " << std::fixed << std::setprecision(3) << avgTimePerLineMs << " ms\n";
            os << "  Peak memory usage: " << std::fixed << std::setprecision(2) << peakMemoryUsageMB << " MB\n";
            os << "  Total lines: " << totalLines << "\n";
            os << "  Total styles: " << totalStyles << "\n";
        }
    };
    
    // Generate a text file with random content
    static void generateRandomFile(TextBuffer& buffer, size_t lineCount, size_t avgLineLength) {
        buffer.clear(false);
        
        std::mt19937 gen(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<> lineLenDist(
            static_cast<int>(avgLineLength / 2), 
            static_cast<int>(avgLineLength * 3 / 2)
        );
        std::uniform_int_distribution<> charDist(32, 126); // ASCII printable characters
        
        for (size_t i = 0; i < lineCount; ++i) {
            std::string line;
            size_t lineLen = lineLenDist(gen);
            line.reserve(lineLen);
            
            for (size_t j = 0; j < lineLen; ++j) {
                line.push_back(static_cast<char>(charDist(gen)));
            }
            
            buffer.addLine(line);
        }
    }
    
    // Benchmark highlighting a large file
    static BenchmarkResult benchmarkHighlighting(
        TestSyntaxHighlightingManager& manager, 
        const TextBuffer& buffer,
        size_t iterationCount = 10) {
        
        BenchmarkResult result;
        result.totalLines = buffer.lineCount();
        result.totalStyles = 0;
        
        // Create memory tracker for this benchmark
        MemoryTracker memTracker;
        
        // Run benchmark
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (size_t iter = 0; iter < iterationCount; ++iter) {
            // Simulate scrolling through the file
            size_t visibleStart = (iter * buffer.lineCount() / iterationCount);
            size_t visibleEnd = visibleStart + 30;
            if (visibleEnd >= buffer.lineCount()) {
                visibleEnd = buffer.lineCount() - 1;
            }
            
            manager.setVisibleRange(visibleStart, visibleEnd);
            
            // Get highlighting for visible range
            auto styles = manager.getHighlightingStyles(visibleStart, visibleEnd);
            
            // Count total styles
            for (const auto& lineStyles : styles) {
                result.totalStyles += lineStyles.size();
            }
            
            // Invalidate some lines to simulate editing
            for (size_t i = 0; i < 5; ++i) {
                size_t line = visibleStart + i;
                if (line < buffer.lineCount()) {
                    manager.invalidateLine(line);
                }
            }
            
            // Update memory tracking
            memTracker.updatePeakMemory();
            
            // Simulate waiting for a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Calculate results
        result.totalTimeMs = duration.count();
        result.avgTimePerLineMs = duration.count() / static_cast<double>(result.totalLines);
        result.peakMemoryUsageMB = memTracker.getPeakMemoryDeltaMB();
        
        return result;
    }
};

// Test fixture for syntax highlighting benchmarks
class SyntaxHighlightingBenchmarkTest : public ::testing::Test {
protected:
    std::shared_ptr<TextBuffer> buffer_;
    std::unique_ptr<TestSyntaxHighlightingManager> manager_;
    std::shared_ptr<MockSyntaxHighlighter> highlighter_;
    
    void SetUp() override {
        buffer_ = std::make_shared<TextBuffer>();
        manager_ = std::make_unique<TestSyntaxHighlightingManager>();
        highlighter_ = std::make_shared<MockSyntaxHighlighter>();
        
        manager_->setHighlighter(highlighter_);
        manager_->setBuffer(buffer_);
        manager_->setEnabled(true);
    }
    
    void TearDown() override {
        manager_->setBuffer(nullptr);
        manager_->setHighlighter(nullptr);
        highlighter_.reset();
        manager_.reset();
        buffer_.reset();
    }
};

TEST_F(SyntaxHighlightingBenchmarkTest, BenchmarkSmallFile) {
    const size_t lineCount = 500;
    const size_t avgLineLength = 80;
    
    HighlightingBenchmark::generateRandomFile(*buffer_, lineCount, avgLineLength);
    
    std::cout << "Running benchmark on small file (" << lineCount << " lines)..." << std::endl;
    auto result = HighlightingBenchmark::benchmarkHighlighting(*manager_, *buffer_);
    
    result.print(std::cout);
    
    // Ensure results are reasonable
    ASSERT_GT(result.totalTimeMs, 0);
    ASSERT_GT(result.totalStyles, 0);
}

TEST_F(SyntaxHighlightingBenchmarkTest, BenchmarkMediumFile) {
    const size_t lineCount = 5000;
    const size_t avgLineLength = 100;
    
    HighlightingBenchmark::generateRandomFile(*buffer_, lineCount, avgLineLength);
    
    std::cout << "Running benchmark on medium file (" << lineCount << " lines)..." << std::endl;
    auto result = HighlightingBenchmark::benchmarkHighlighting(*manager_, *buffer_);
    
    result.print(std::cout);
    
    // Ensure results are reasonable
    ASSERT_GT(result.totalTimeMs, 0);
    ASSERT_GT(result.totalStyles, 0);
}

TEST_F(SyntaxHighlightingBenchmarkTest, BenchmarkLargeFile) {
    const size_t lineCount = 20000;
    const size_t avgLineLength = 120;
    
    HighlightingBenchmark::generateRandomFile(*buffer_, lineCount, avgLineLength);
    
    std::cout << "Running benchmark on large file (" << lineCount << " lines)..." << std::endl;
    auto result = HighlightingBenchmark::benchmarkHighlighting(*manager_, *buffer_);
    
    result.print(std::cout);
    
    // Ensure results are reasonable
    ASSERT_GT(result.totalTimeMs, 0);
    ASSERT_GT(result.totalStyles, 0);
}

TEST_F(SyntaxHighlightingBenchmarkTest, BenchmarkConcurrentAccess) {
    const size_t lineCount = 5000;
    const size_t avgLineLength = 100;
    const size_t threadCount = 4;
    
    HighlightingBenchmark::generateRandomFile(*buffer_, lineCount, avgLineLength);
    
    std::cout << "Running concurrent benchmark with " << threadCount << " threads..." << std::endl;
    
    std::vector<HighlightingBenchmark::BenchmarkResult> results(threadCount);
    std::vector<std::thread> threads;
    
    // Create memory tracker for this benchmark
    MemoryTracker memTracker;
    
    // Thread function that performs benchmark operations
    auto threadFunc = [this, &results](size_t threadId, size_t startLine, size_t endLine) {
        // Create a thread-local TestSyntaxHighlightingManager
        TestSyntaxHighlightingManager threadManager;
        threadManager.setHighlighter(highlighter_);
        threadManager.setBuffer(buffer_);
        threadManager.setEnabled(true);
        
        // Set a visible range specific to this thread
        threadManager.setVisibleRange(startLine, endLine);
        
        // Run a mini-benchmark
        auto startTime = std::chrono::high_resolution_clock::now();
        
        size_t totalStyles = 0;
        const size_t iterations = 20;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            // Randomly select a range within our assigned section
            std::mt19937 gen(static_cast<unsigned int>(threadId * 1000 + iter));
            std::uniform_int_distribution<> rangeDist(0, static_cast<int>(endLine - startLine - 10));
            
            size_t rangeStart = startLine + rangeDist(gen);
            size_t rangeEnd = rangeStart + 10; // Process 10 lines at a time
            
            // Get highlighting
            auto styles = threadManager.getHighlightingStyles(rangeStart, rangeEnd);
            
            // Count styles
            for (const auto& lineStyles : styles) {
                totalStyles += lineStyles.size();
            }
            
            // Invalidate a line
            threadManager.invalidateLine(rangeStart + iter % 10);
            
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Record results
        results[threadId].totalTimeMs = duration.count();
        results[threadId].totalStyles = totalStyles;
        results[threadId].totalLines = endLine - startLine;
        results[threadId].avgTimePerLineMs = duration.count() / static_cast<double>(endLine - startLine);
        // We don't set memory usage per thread as it's not reliable
    };
    
    // Launch threads
    const size_t linesPerThread = lineCount / threadCount;
    
    for (size_t i = 0; i < threadCount; ++i) {
        size_t startLine = i * linesPerThread;
        size_t endLine = (i == threadCount - 1) ? lineCount : (i + 1) * linesPerThread;
        
        threads.emplace_back(threadFunc, i, startLine, endLine);
    }
    
    // Monitor memory usage during thread execution
    std::atomic<bool> threadsRunning{true};
    std::thread memoryMonitor([&threadsRunning, &memTracker]() {
        while (threadsRunning) {
            memTracker.updatePeakMemory();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Stop the memory monitor
    threadsRunning = false;
    memoryMonitor.join();
    
    // Aggregate and print results
    HighlightingBenchmark::BenchmarkResult combinedResult;
    combinedResult.totalTimeMs = 0;
    combinedResult.totalStyles = 0;
    combinedResult.totalLines = lineCount;
    combinedResult.peakMemoryUsageMB = memTracker.getPeakMemoryDeltaMB();
    
    for (const auto& result : results) {
        combinedResult.totalTimeMs += result.totalTimeMs;
        combinedResult.totalStyles += result.totalStyles;
        std::cout << "Thread result: " << result.totalTimeMs << "ms, " 
                  << result.totalStyles << " styles" << std::endl;
    }
    
    combinedResult.totalTimeMs /= threadCount; // Average time across threads
    combinedResult.avgTimePerLineMs = combinedResult.totalTimeMs / static_cast<double>(lineCount);
    
    std::cout << "Combined results:" << std::endl;
    combinedResult.print(std::cout);
    
    // Ensure results are reasonable
    ASSERT_GT(combinedResult.totalTimeMs, 0);
    ASSERT_GT(combinedResult.totalStyles, 0);
} 