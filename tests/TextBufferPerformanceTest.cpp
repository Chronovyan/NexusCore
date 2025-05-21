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
#include <numeric>
#include <iomanip>
#include <fstream>

#include "AppDebugLog.h"
#include "TextBuffer.h"
#include "EditorCoreThreadPool.h"

/**
 * This test measures the performance characteristics of TextBuffer operations
 * when processed by the EditorCoreThreadPool.
 */

// Configuration for performance tests
namespace PerfTestConfig {
    // Number of threads to use for testing
    constexpr int NUM_THREADS = 8;
    
    // Number of operations per thread
    constexpr int OPERATIONS_PER_THREAD = 10000;
    
    // Initial number of lines in the buffer
    constexpr int INITIAL_LINES = 1000;
    
    // Maximum line length
    constexpr int MAX_LINE_LENGTH = 100;
    
    // Probability weights for different operations (must sum to 100)
    constexpr int PROB_ADD_LINE = 20;
    constexpr int PROB_INSERT_LINE = 20;
    constexpr int PROB_DELETE_LINE = 10;
    constexpr int PROB_REPLACE_LINE = 20;
    constexpr int PROB_READ_LINE = 30;
    
    // Whether to output detailed timing information to a CSV file
    constexpr bool OUTPUT_TIMING_CSV = true;
    constexpr const char* CSV_FILENAME = "textbuffer_performance.csv";
    
    // Number of worker threads in the EditorCoreThreadPool
    constexpr int THREAD_POOL_SIZE = 4;
}

// Operation type enum
enum class OperationType {
    ADD_LINE,
    INSERT_LINE,
    DELETE_LINE,
    REPLACE_LINE,
    READ_LINE
};

// Timing information for an operation
struct OperationTiming {
    OperationType type;
    std::chrono::nanoseconds duration;
    bool success;
    
    static std::string getTypeString(OperationType type) {
        switch (type) {
            case OperationType::ADD_LINE: return "ADD_LINE";
            case OperationType::INSERT_LINE: return "INSERT_LINE";
            case OperationType::DELETE_LINE: return "DELETE_LINE";
            case OperationType::REPLACE_LINE: return "REPLACE_LINE";
            case OperationType::READ_LINE: return "READ_LINE";
            default: return "UNKNOWN";
        }
    }
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
            "0123456789";
        
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
        std::uniform_int_distribution<> lengthDist(10, PerfTestConfig::MAX_LINE_LENGTH);
        return generateRandomString(lengthDist(engine_));
    }
    
    // Select a random operation type based on probability weights
    OperationType selectRandomOperation() {
        std::uniform_int_distribution<> dist(1, 100);
        int roll = dist(engine_);
        
        if (roll <= PerfTestConfig::PROB_ADD_LINE) {
            return OperationType::ADD_LINE;
        } else if (roll <= PerfTestConfig::PROB_ADD_LINE + PerfTestConfig::PROB_INSERT_LINE) {
            return OperationType::INSERT_LINE;
        } else if (roll <= PerfTestConfig::PROB_ADD_LINE + PerfTestConfig::PROB_INSERT_LINE + 
                         PerfTestConfig::PROB_DELETE_LINE) {
            return OperationType::DELETE_LINE;
        } else if (roll <= PerfTestConfig::PROB_ADD_LINE + PerfTestConfig::PROB_INSERT_LINE + 
                         PerfTestConfig::PROB_DELETE_LINE + PerfTestConfig::PROB_REPLACE_LINE) {
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
    
private:
    std::mt19937 engine_;
};

// Performance test class
class TextBufferPerformanceTest {
public:
    TextBufferPerformanceTest()
        : textBuffer_(std::make_shared<TextBuffer>()),
          threadPool_(std::make_unique<EditorCoreThreadPool>(PerfTestConfig::THREAD_POOL_SIZE)),
          stopProcessorThread_(false)
    {
        LOG_INIT("TextBufferPerformanceTest");
        LOG_DEBUG("Initializing TextBuffer performance test");
    }
    
    void initialize() {
        // Start the thread pool
        threadPool_->start();
        
        // Assign TextBuffer ownership to a thread in the pool
        ownerThreadId_ = threadPool_->assignTextBufferOwnership(textBuffer_);
        LOG_DEBUG("TextBuffer ownership assigned to thread: " + 
                 std::to_string(std::hash<std::thread::id>{}(ownerThreadId_)));
        
        // Initialize the TextBuffer with test lines
        for (int i = 0; i < PerfTestConfig::INITIAL_LINES; ++i) {
            textBuffer_->addLine("Initial line " + std::to_string(i) + ": " + randomGen_.generateRandomLine());
        }
        
        LOG_DEBUG("TextBuffer initialized with " + std::to_string(textBuffer_->lineCount()) + " lines");
        
        // Start the processor thread
        processorThread_ = std::thread([this]() {
            while (!stopProcessorThread_) {
                size_t processed = threadPool_->notifyTextBufferOperationsAvailable();
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
        
        LOG_DEBUG("Processor thread started");
    }
    
    void runTest() {
        LOG_DEBUG("Starting performance test with " + 
                 std::to_string(PerfTestConfig::NUM_THREADS) + " worker threads");
        
        // Create worker threads
        std::vector<std::thread> workerThreads;
        std::vector<std::vector<OperationTiming>> threadTimings(PerfTestConfig::NUM_THREADS);
        
        // Start time measurement
        auto startTime = std::chrono::steady_clock::now();
        
        // Launch worker threads
        for (int i = 0; i < PerfTestConfig::NUM_THREADS; ++i) {
            workerThreads.emplace_back(&TextBufferPerformanceTest::workerThreadFunction, 
                                      this, i, std::ref(threadTimings[i]));
        }
        
        // Wait for all worker threads to complete
        for (auto& thread : workerThreads) {
            thread.join();
        }
        
        // End time measurement
        auto endTime = std::chrono::steady_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Calculate and report statistics
        reportStatistics(threadTimings, totalDuration);
        
        // Output detailed timing information to CSV if enabled
        if (PerfTestConfig::OUTPUT_TIMING_CSV) {
            outputTimingsToCsv(threadTimings);
        }
    }
    
    void cleanup() {
        LOG_DEBUG("Cleaning up performance test resources");
        
        // Stop the processor thread
        stopProcessorThread_ = true;
        if (processorThread_.joinable()) {
            processorThread_.join();
        }
        
        // Shut down the thread pool
        threadPool_->shutdown();
    }
    
private:
    // Worker thread function
    void workerThreadFunction(int threadId, std::vector<OperationTiming>& timings) {
        LOG_DEBUG("Worker thread " + std::to_string(threadId) + " started");
        
        timings.reserve(PerfTestConfig::OPERATIONS_PER_THREAD);
        
        for (int i = 0; i < PerfTestConfig::OPERATIONS_PER_THREAD; ++i) {
            // Select a random operation
            auto opType = randomGen_.selectRandomOperation();
            
            // Perform the operation and measure its duration
            auto startTime = std::chrono::high_resolution_clock::now();
            bool success = true;
            
            try {
                switch (opType) {
                    case OperationType::ADD_LINE:
                        performAddLine();
                        break;
                    case OperationType::INSERT_LINE:
                        performInsertLine();
                        break;
                    case OperationType::DELETE_LINE:
                        performDeleteLine();
                        break;
                    case OperationType::REPLACE_LINE:
                        performReplaceLine();
                        break;
                    case OperationType::READ_LINE:
                        performReadLine();
                        break;
                }
            } catch (const std::exception& e) {
                success = false;
                LOG_ERROR("Worker " + std::to_string(threadId) + " operation failed: " + e.what());
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
            
            // Record timing information
            timings.push_back({opType, duration, success});
        }
        
        LOG_DEBUG("Worker thread " + std::to_string(threadId) + " completed");
    }
    
    // Perform operations
    void performAddLine() {
        std::string line = "Added line: " + randomGen_.generateRandomLine();
        auto future = textBuffer_->requestAddLine(line);
        future.wait();
    }
    
    void performInsertLine() {
        size_t lineCount = textBuffer_->lineCount();
        size_t index = randomGen_.selectRandomLineIndex(lineCount + 1); // +1 to allow insertion at end
        std::string line = "Inserted line: " + randomGen_.generateRandomLine();
        auto future = textBuffer_->requestInsertLine(index, line);
        future.wait();
    }
    
    void performDeleteLine() {
        size_t lineCount = textBuffer_->lineCount();
        if (lineCount <= 1) {
            // Don't delete the last line
            return;
        }
        
        size_t index = randomGen_.selectRandomLineIndex(lineCount);
        auto future = textBuffer_->requestDeleteLine(index);
        future.wait();
    }
    
    void performReplaceLine() {
        size_t lineCount = textBuffer_->lineCount();
        if (lineCount == 0) {
            // Can't replace if there are no lines
            return;
        }
        
        size_t index = randomGen_.selectRandomLineIndex(lineCount);
        std::string line = "Replaced line: " + randomGen_.generateRandomLine();
        auto future = textBuffer_->requestReplaceLine(index, line);
        future.wait();
    }
    
    void performReadLine() {
        size_t lineCount = textBuffer_->lineCount();
        if (lineCount == 0) {
            // Can't read if there are no lines
            return;
        }
        
        size_t index = randomGen_.selectRandomLineIndex(lineCount);
        std::string line = textBuffer_->getLine(index);
    }
    
    // Calculate and report statistics
    void reportStatistics(const std::vector<std::vector<OperationTiming>>& threadTimings,
                         std::chrono::milliseconds totalDuration) {
        LOG_DEBUG("Calculating performance statistics");
        
        // Count operations by type
        std::map<OperationType, int> opCounts;
        std::map<OperationType, std::vector<std::chrono::nanoseconds>> opDurations;
        int totalOps = 0;
        int successfulOps = 0;
        
        for (const auto& threadTiming : threadTimings) {
            for (const auto& timing : threadTiming) {
                totalOps++;
                opCounts[timing.type]++;
                opDurations[timing.type].push_back(timing.duration);
                if (timing.success) {
                    successfulOps++;
                }
            }
        }
        
        // Calculate throughput
        double opsPerSecond = static_cast<double>(totalOps) / (totalDuration.count() / 1000.0);
        
        // Calculate average, min, max, and percentile latencies for each operation type
        std::map<OperationType, double> avgLatencies;
        std::map<OperationType, std::chrono::nanoseconds> minLatencies;
        std::map<OperationType, std::chrono::nanoseconds> maxLatencies;
        std::map<OperationType, std::chrono::nanoseconds> p95Latencies;
        std::map<OperationType, std::chrono::nanoseconds> p99Latencies;
        
        for (const auto& entry : opDurations) {
            auto opType = entry.first;
            const auto& durations = entry.second;
            
            // Skip if no operations of this type
            if (durations.empty()) {
                continue;
            }
            
            // Calculate average
            double sum = 0;
            for (const auto& duration : durations) {
                sum += duration.count();
            }
            avgLatencies[opType] = sum / durations.size();
            
            // Calculate min and max
            auto minmax = std::minmax_element(durations.begin(), durations.end());
            minLatencies[opType] = *minmax.first;
            maxLatencies[opType] = *minmax.second;
            
            // Calculate percentiles
            std::vector<std::chrono::nanoseconds> sortedDurations = durations;
            std::sort(sortedDurations.begin(), sortedDurations.end());
            
            size_t p95Index = static_cast<size_t>(sortedDurations.size() * 0.95);
            size_t p99Index = static_cast<size_t>(sortedDurations.size() * 0.99);
            
            p95Latencies[opType] = sortedDurations[p95Index];
            p99Latencies[opType] = sortedDurations[p99Index];
        }
        
        // Report statistics
        std::cout << "====== TextBuffer Performance Test Results ======" << std::endl;
        std::cout << "Total operations: " << totalOps << std::endl;
        std::cout << "Successful operations: " << successfulOps << " (" 
                  << (static_cast<double>(successfulOps) / totalOps * 100.0) << "%)" << std::endl;
        std::cout << "Total duration: " << totalDuration.count() << " ms" << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) 
                  << opsPerSecond << " operations/second" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Operation counts:" << std::endl;
        for (const auto& entry : opCounts) {
            std::cout << "  " << OperationTiming::getTypeString(entry.first) << ": " 
                      << entry.second << " (" 
                      << (static_cast<double>(entry.second) / totalOps * 100.0) << "%)" << std::endl;
        }
        std::cout << std::endl;
        
        std::cout << "Latency statistics (microseconds):" << std::endl;
        std::cout << std::setw(15) << "Operation" << std::setw(10) << "Average" 
                  << std::setw(10) << "Min" << std::setw(10) << "Max" 
                  << std::setw(10) << "P95" << std::setw(10) << "P99" << std::endl;
        std::cout << std::string(65, '-') << std::endl;
        
        for (const auto& entry : avgLatencies) {
            auto opType = entry.first;
            std::cout << std::setw(15) << OperationTiming::getTypeString(opType)
                      << std::setw(10) << std::fixed << std::setprecision(2) << (entry.second / 1000.0)
                      << std::setw(10) << std::fixed << std::setprecision(2) << (minLatencies[opType].count() / 1000.0)
                      << std::setw(10) << std::fixed << std::setprecision(2) << (maxLatencies[opType].count() / 1000.0)
                      << std::setw(10) << std::fixed << std::setprecision(2) << (p95Latencies[opType].count() / 1000.0)
                      << std::setw(10) << std::fixed << std::setprecision(2) << (p99Latencies[opType].count() / 1000.0)
                      << std::endl;
        }
        
        LOG_DEBUG("Performance statistics calculation completed");
    }
    
    // Output detailed timing information to CSV
    void outputTimingsToCsv(const std::vector<std::vector<OperationTiming>>& threadTimings) {
        LOG_DEBUG("Writing timing information to CSV file");
        
        std::ofstream csvFile(PerfTestConfig::CSV_FILENAME);
        if (!csvFile.is_open()) {
            LOG_ERROR("Failed to open CSV file for writing");
            return;
        }
        
        // Write header
        csvFile << "ThreadID,OperationIndex,OperationType,DurationNs,Success" << std::endl;
        
        // Write timing data
        for (size_t threadId = 0; threadId < threadTimings.size(); ++threadId) {
            const auto& timings = threadTimings[threadId];
            for (size_t i = 0; i < timings.size(); ++i) {
                const auto& timing = timings[i];
                csvFile << threadId << ","
                        << i << ","
                        << OperationTiming::getTypeString(timing.type) << ","
                        << timing.duration.count() << ","
                        << (timing.success ? "1" : "0") << std::endl;
            }
        }
        
        csvFile.close();
        LOG_DEBUG("Timing information written to " + std::string(PerfTestConfig::CSV_FILENAME));
    }
    
private:
    std::shared_ptr<TextBuffer> textBuffer_;
    std::unique_ptr<EditorCoreThreadPool> threadPool_;
    std::thread::id ownerThreadId_;
    std::thread processorThread_;
    std::atomic<bool> stopProcessorThread_;
    RandomGenerator randomGen_;
};

// Main function
int main() {
    try {
        TextBufferPerformanceTest test;
        
        test.initialize();
        test.runTest();
        test.cleanup();
        
        std::cout << "TextBuffer performance test completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR" << std::endl;
        return 1;
    }
} 