#include "gtest/gtest.h"
#include "../src/VirtualizedTextBuffer.h"
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iostream>

// Helper function to create a large test file
static std::string createLargeTestFile(const std::string& filename, size_t lineCount) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        return "Failed to create test file";
    }
    
    for (size_t i = 0; i < lineCount; ++i) {
        outFile << "This is test line " << i << " with some additional content to make it longer." << std::endl;
    }
    
    outFile.close();
    return "";
}

// Helper function to delete a test file
static void deleteTestFile(const std::string& filename) {
    std::remove(filename.c_str());
}

// Helper function to measure cache performance
static double measureCachePerformance(VirtualizedTextBuffer& buffer, 
                                     const std::vector<size_t>& accessPattern) {
    // Reset cache stats before measurement
    buffer.resetCacheStats();
    
    // Access the buffer according to the pattern
    for (size_t lineIndex : accessPattern) {
        buffer.getLine(lineIndex);
    }
    
    // Return the cache hit rate
    return buffer.getCacheHitRate();
}

class VirtualizedTextBufferCachingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test file
        std::string error = createLargeTestFile(testFilename, lineCount);
        ASSERT_TRUE(error.empty());
    }

    void TearDown() override {
        // Delete test file
        deleteTestFile(testFilename);
    }

    const std::string testFilename = "virtualized_buffer_test.txt";
    const size_t lineCount = 10000;
    const size_t pageSize = 100;
    const size_t cacheSize = 10;
};

TEST_F(VirtualizedTextBufferCachingTest, CacheEvictionPoliciesComparison) {
    // Create buffers with different cache policies
    VirtualizedTextBuffer lruBuffer(testFilename, pageSize, cacheSize);
    lruBuffer.setCacheEvictionPolicy(VirtualizedTextBuffer::CacheEvictionPolicy::LRU);
    
    VirtualizedTextBuffer slruBuffer(testFilename, pageSize, cacheSize);
    slruBuffer.setCacheEvictionPolicy(VirtualizedTextBuffer::CacheEvictionPolicy::SLRU);
    
    VirtualizedTextBuffer arcBuffer(testFilename, pageSize, cacheSize);
    arcBuffer.setCacheEvictionPolicy(VirtualizedTextBuffer::CacheEvictionPolicy::ARC);
    
    VirtualizedTextBuffer spatialBuffer(testFilename, pageSize, cacheSize);
    spatialBuffer.setCacheEvictionPolicy(VirtualizedTextBuffer::CacheEvictionPolicy::SPATIAL);
    
    // Generate a random access pattern with some locality (80% local, 20% random)
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> lineDist(0, lineCount - 1);
    
    const size_t accessCount = 1000;
    std::vector<size_t> accessPattern;
    accessPattern.reserve(accessCount);
    
    size_t currentLine = lineDist(rng);
    for (size_t i = 0; i < accessCount; ++i) {
        if (i % 5 == 0) {
            // Every 5th access is random (20%)
            currentLine = lineDist(rng);
        } else {
            // 80% of accesses are within ±50 lines of current position
            std::uniform_int_distribution<int> localDist(-50, 50);
            int delta = localDist(rng);
            currentLine = std::min(std::max(0, static_cast<int>(currentLine) + delta), 
                                 static_cast<int>(lineCount - 1));
        }
        accessPattern.push_back(currentLine);
    }
    
    // Measure cache performance for each policy
    double lruHitRate = measureCachePerformance(lruBuffer, accessPattern);
    double slruHitRate = measureCachePerformance(slruBuffer, accessPattern);
    double arcHitRate = measureCachePerformance(arcBuffer, accessPattern);
    double spatialHitRate = measureCachePerformance(spatialBuffer, accessPattern);
    
    // Output results
    std::cout << "LRU hit rate: " << lruHitRate << "%" << std::endl;
    std::cout << "SLRU hit rate: " << slruHitRate << "%" << std::endl;
    std::cout << "ARC hit rate: " << arcHitRate << "%" << std::endl;
    std::cout << "Spatial hit rate: " << spatialHitRate << "%" << std::endl;
    
    // Verify that at least one of the advanced policies outperforms LRU
    bool improvedPerformance = (slruHitRate > lruHitRate) || 
                              (arcHitRate > lruHitRate) || 
                              (spatialHitRate > lruHitRate);
    
    EXPECT_TRUE(improvedPerformance);
}

TEST_F(VirtualizedTextBufferCachingTest, PrefetchingStrategiesComparison) {
    // Create buffers with different prefetch strategies
    VirtualizedTextBuffer noPreBuffer(testFilename, pageSize, cacheSize);
    noPreBuffer.setPrefetchStrategy(VirtualizedTextBuffer::PrefetchStrategy::NONE);
    
    VirtualizedTextBuffer adjPreBuffer(testFilename, pageSize, cacheSize);
    adjPreBuffer.setPrefetchStrategy(VirtualizedTextBuffer::PrefetchStrategy::ADJACENT);
    adjPreBuffer.setPrefetchDistance(2);
    
    VirtualizedTextBuffer predPreBuffer(testFilename, pageSize, cacheSize);
    predPreBuffer.setPrefetchStrategy(VirtualizedTextBuffer::PrefetchStrategy::PREDICTIVE);
    predPreBuffer.setPrefetchDistance(2);
    
    VirtualizedTextBuffer adaptPreBuffer(testFilename, pageSize, cacheSize);
    adaptPreBuffer.setPrefetchStrategy(VirtualizedTextBuffer::PrefetchStrategy::ADAPTIVE);
    adaptPreBuffer.setPrefetchDistance(2);
    
    // Generate a sequential read pattern with occasional jumps
    std::vector<size_t> accessPattern;
    accessPattern.reserve(1000);
    
    // First, read sequentially to build up history
    for (size_t i = 0; i < 300; i += 5) {
        accessPattern.push_back(i);
    }
    
    // Then, add some patterns with jumps
    for (size_t i = 0; i < 5; ++i) {
        // Read pattern: A, B, C, A, B, C, ...
        accessPattern.push_back(1000 + i*100);
        accessPattern.push_back(2000 + i*100);
        accessPattern.push_back(3000 + i*100);
        accessPattern.push_back(1000 + i*100);
        accessPattern.push_back(2000 + i*100);
        accessPattern.push_back(3000 + i*100);
    }
    
    // Add sequential reads with larger jumps
    for (size_t i = 0; i < 300; i += 20) {
        accessPattern.push_back(5000 + i);
    }
    
    // Measure cache performance for each strategy
    double noPreHitRate = measureCachePerformance(noPreBuffer, accessPattern);
    double adjPreHitRate = measureCachePerformance(adjPreBuffer, accessPattern);
    double predPreHitRate = measureCachePerformance(predPreBuffer, accessPattern);
    double adaptPreHitRate = measureCachePerformance(adaptPreBuffer, accessPattern);
    
    // Output results
    std::cout << "No prefetching hit rate: " << noPreHitRate << "%" << std::endl;
    std::cout << "Adjacent prefetching hit rate: " << adjPreHitRate << "%" << std::endl;
    std::cout << "Predictive prefetching hit rate: " << predPreHitRate << "%" << std::endl;
    std::cout << "Adaptive prefetching hit rate: " << adaptPreHitRate << "%" << std::endl;
    
    // Verify that at least one of the prefetching strategies outperforms no prefetching
    bool improvedPerformance = (adjPreHitRate > noPreHitRate) || 
                              (predPreHitRate > noPreHitRate) || 
                              (adaptPreHitRate > noPreHitRate);
    
    EXPECT_TRUE(improvedPerformance);
}

TEST_F(VirtualizedTextBufferCachingTest, SequentialVsRandomAccessPerformance) {
    VirtualizedTextBuffer buffer(testFilename, pageSize, cacheSize);
    buffer.setCacheEvictionPolicy(VirtualizedTextBuffer::CacheEvictionPolicy::ARC);
    buffer.setPrefetchStrategy(VirtualizedTextBuffer::PrefetchStrategy::ADAPTIVE);
    
    // Generate sequential access pattern
    std::vector<size_t> sequentialPattern;
    sequentialPattern.reserve(1000);
    for (size_t i = 0; i < 1000; ++i) {
        sequentialPattern.push_back(i);
    }
    
    // Generate random access pattern
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> lineDist(0, lineCount - 1);
    
    std::vector<size_t> randomPattern;
    randomPattern.reserve(1000);
    for (size_t i = 0; i < 1000; ++i) {
        randomPattern.push_back(lineDist(rng));
    }
    
    // Measure performance
    buffer.resetCacheStats();
    
    // Sequential access
    auto startSeq = std::chrono::high_resolution_clock::now();
    for (size_t line : sequentialPattern) {
        buffer.getLine(line);
    }
    auto endSeq = std::chrono::high_resolution_clock::now();
    auto seqTime = std::chrono::duration_cast<std::chrono::microseconds>(endSeq - startSeq).count();
    double seqHitRate = buffer.getCacheHitRate();
    
    // Reset for random access test
    buffer.resetCacheStats();
    
    // Random access
    auto startRand = std::chrono::high_resolution_clock::now();
    for (size_t line : randomPattern) {
        buffer.getLine(line);
    }
    auto endRand = std::chrono::high_resolution_clock::now();
    auto randTime = std::chrono::duration_cast<std::chrono::microseconds>(endRand - startRand).count();
    double randHitRate = buffer.getCacheHitRate();
    
    // Output results
    std::cout << "Sequential access time: " << seqTime << " µs, hit rate: " << seqHitRate << "%" << std::endl;
    std::cout << "Random access time: " << randTime << " µs, hit rate: " << randHitRate << "%" << std::endl;
    
    // Sequential access should typically be faster and have a higher hit rate
    EXPECT_GT(seqHitRate, randHitRate);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 