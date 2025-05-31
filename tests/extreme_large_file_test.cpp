#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <fstream>
#include <algorithm>
#include <random>

#include "Editor.h"
#include "WorkspaceManager.h"
#include "test_file_utilities.h"

namespace fs = std::filesystem;

/**
 * Test fixture for measuring extremely large file operations performance
 * Specifically targeting files >10MB as per the roadmap requirement
 */
class ExtremeLargeFileTest : public ::testing::Test {
protected:
    // File sizes for testing - focusing on the boundary of >10MB
    const size_t MEDIUM_LARGE_FILE_SIZE = 12 * 1024 * 1024;  // 12MB (just over 10MB)
    const size_t VERY_LARGE_FILE_SIZE = 50 * 1024 * 1024;    // 50MB
    const size_t EXTREME_LARGE_FILE_SIZE = 150 * 1024 * 1024; // 150MB
    
    // Optional ultra-large file - only enabled with ULTRA_LARGE_TESTS env var
    const size_t ULTRA_LARGE_FILE_SIZE = 500 * 1024 * 1024;   // 500MB
    
    // Different content patterns to test with
    const std::vector<TestFileGenerator::ContentPattern> contentPatterns = {
        TestFileGenerator::ContentPattern::REPEATED_TEXT,
        TestFileGenerator::ContentPattern::CODE_LIKE,
        TestFileGenerator::ContentPattern::MIXED_LINE_LENGTHS
    };
    
    // Test subjects
    std::unique_ptr<Editor> editor;
    
    // Test file paths
    std::string mediumLargeFilePath_;
    std::string veryLargeFilePath_;
    std::string extremeLargeFilePath_;
    std::string ultraLargeFilePath_;
    std::string emptyFilePath_; // For "closing" files
    
    // Track generated files for cleanup
    std::vector<std::string> generatedTestFiles;
    
    // Test output directory
    const std::string testOutputDir = "test_output/extreme_large_files/";
    
    // Performance thresholds
    struct PerformanceThresholds {
        double openTimeMs;
        double saveTimeMs;
        double insertTimeMs;
        double searchTimeMs;
        double scrollTimeMs;
        double memoryMultiplier; // How much more memory than file size is acceptable
    };
    
    // Performance thresholds for different file sizes
    std::map<std::string, PerformanceThresholds> thresholds = {
        {"MediumLarge", {500, 1000, 20, 2000, 50, 2.0}},
        {"VeryLarge",   {2000, 3000, 50, 6000, 100, 1.8}},
        {"ExtremeLarge", {5000, 8000, 100, 12000, 200, 1.5}},
        {"UltraLarge",   {15000, 20000, 200, 30000, 500, 1.2}}
    };
    
    void SetUp() override {
        // Initialize editor
        editor = std::make_unique<Editor>();
        
        // Create test directory if it doesn't exist
        if (!fs::exists(testOutputDir)) {
            fs::create_directories(testOutputDir);
        }
        
        // Create an empty file to use for "closing" operations
        emptyFilePath_ = testOutputDir + "empty.txt";
        {
            std::ofstream emptyFile(emptyFilePath_);
            emptyFile << "";
        }
        generatedTestFiles.push_back(emptyFilePath_);
        
        // Select a random content pattern for this test run
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> patternDist(0, contentPatterns.size() - 1);
        auto selectedPattern = contentPatterns[patternDist(gen)];
        
        // Generate test files of different sizes
        try {
            std::cout << "Generating extreme large test files..." << std::endl;
            
            mediumLargeFilePath_ = TestFileGenerator::generateFile(
                MEDIUM_LARGE_FILE_SIZE,
                testOutputDir + "medium_large_test_file.txt",
                selectedPattern,
                TestFileGenerator::LineEnding::LF
            );
            generatedTestFiles.push_back(mediumLargeFilePath_);
            std::cout << "Medium-Large file generated: " << mediumLargeFilePath_ << std::endl;
            
            veryLargeFilePath_ = TestFileGenerator::generateFile(
                VERY_LARGE_FILE_SIZE,
                testOutputDir + "very_large_test_file.txt",
                selectedPattern,
                TestFileGenerator::LineEnding::LF
            );
            generatedTestFiles.push_back(veryLargeFilePath_);
            std::cout << "Very Large file generated: " << veryLargeFilePath_ << std::endl;
            
            // Only generate extreme large file if not running in CI environment
            // (To avoid timeouts in CI pipelines)
            if (std::getenv("CI") == nullptr) {
                extremeLargeFilePath_ = TestFileGenerator::generateFile(
                    EXTREME_LARGE_FILE_SIZE,
                    testOutputDir + "extreme_large_test_file.txt",
                    selectedPattern,
                    TestFileGenerator::LineEnding::LF
                );
                generatedTestFiles.push_back(extremeLargeFilePath_);
                std::cout << "Extreme Large file generated: " << extremeLargeFilePath_ << std::endl;
                
                // Only generate ultra large file if explicitly enabled
                if (std::getenv("ULTRA_LARGE_TESTS") != nullptr) {
                    ultraLargeFilePath_ = TestFileGenerator::generateFile(
                        ULTRA_LARGE_FILE_SIZE,
                        testOutputDir + "ultra_large_test_file.txt",
                        selectedPattern,
                        TestFileGenerator::LineEnding::LF
                    );
                    generatedTestFiles.push_back(ultraLargeFilePath_);
                    std::cout << "Ultra Large file generated: " << ultraLargeFilePath_ << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error generating test files: " << e.what() << std::endl;
            throw;
        }
    }
    
    void TearDown() override {
        // "Close" any open files by opening an empty file
        if (editor) {
            editor->openFile(emptyFilePath_);
        }
        
        // Clean up test files unless KEEP_TEST_FILES env var is set
        if (std::getenv("KEEP_TEST_FILES") == nullptr) {
            for (const auto& filePath : generatedTestFiles) {
                try {
                    if (fs::exists(filePath)) {
                        fs::remove(filePath);
                        std::cout << "Removed test file: " << filePath << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error removing test file " << filePath << ": " << e.what() << std::endl;
                }
            }
        } else {
            std::cout << "Test files kept for inspection in: " << testOutputDir << std::endl;
        }
        generatedTestFiles.clear();
    }
    
    /**
     * Measure execution time of an operation in milliseconds
     */
    template<typename Func>
    double MeasureExecutionTimeMs(Func&& operation) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        return duration.count();
    }
    
    /**
     * "Close" the current file by opening an empty file
     */
    void closeCurrentFile() {
        editor->openFile(emptyFilePath_);
    }
    
    /**
     * Test opening a file and measure performance
     */
    void testFileOpen(const std::string& filePath, const std::string& sizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << sizeLabel;
            return;
        }
        
        // Record memory before opening file
        size_t memoryBefore = MemoryTracker::getCurrentMemoryUsage();
        
        // Measure file open time
        double openTimeMs = MeasureExecutionTimeMs([&]() {
            ASSERT_TRUE(editor->openFile(filePath)) 
                << "Failed to open " << sizeLabel << " test file: " << filePath;
        });
        
        // Record memory after opening file
        size_t memoryAfter = MemoryTracker::getCurrentMemoryUsage();
        size_t memoryDiff = memoryAfter - memoryBefore;
        size_t fileSize = fs::file_size(filePath);
        
        // Output metrics
        std::cout << "[" << sizeLabel << "] File open time: " << openTimeMs << " ms" << std::endl;
        std::cout << "[" << sizeLabel << "] Memory before: " << memoryBefore / (1024 * 1024) << " MB" << std::endl;
        std::cout << "[" << sizeLabel << "] Memory after: " << memoryAfter / (1024 * 1024) << " MB" << std::endl;
        std::cout << "[" << sizeLabel << "] Memory diff: " << memoryDiff / (1024 * 1024) << " MB" << std::endl;
        std::cout << "[" << sizeLabel << "] File size: " << fileSize / (1024 * 1024) << " MB" << std::endl;
        std::cout << "[" << sizeLabel << "] Memory/File ratio: " << (double)memoryDiff / fileSize << std::endl;
        
        // Check file was opened successfully
        const TextBuffer& buffer = editor->getBuffer();
        ASSERT_GT(buffer.lineCount(), 0) << "File doesn't appear to be loaded: " << sizeLabel;
        
        // Check against performance thresholds
        auto& threshold = thresholds[sizeLabel];
        ASSERT_LE(openTimeMs, threshold.openTimeMs * 1.5) 
            << sizeLabel << " file open time exceeded threshold by too much";
        ASSERT_LE((double)memoryDiff, fileSize * threshold.memoryMultiplier) 
            << sizeLabel << " memory usage exceeded threshold";
    }
    
    /**
     * Test saving a file and measure performance
     */
    void testFileSave(const std::string& filePath, const std::string& sizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << sizeLabel;
            return;
        }
        
        // First open the file
        ASSERT_TRUE(editor->openFile(filePath)) 
            << "Failed to open " << sizeLabel << " test file for save test";
        
        // Generate a new save path
        std::string savePath = testOutputDir + "save_test_" + sizeLabel + ".txt";
        generatedTestFiles.push_back(savePath);
        
        // Measure save time
        double saveTimeMs = MeasureExecutionTimeMs([&]() {
            ASSERT_TRUE(editor->saveFileAs(savePath)) 
                << "Failed to save " << sizeLabel << " test file";
        });
        
        // Output metrics
        std::cout << "[" << sizeLabel << "] File save time: " << saveTimeMs << " ms" << std::endl;
        
        // Verify the saved file exists and has the correct size
        ASSERT_TRUE(fs::exists(savePath)) << "Saved file doesn't exist: " << savePath;
        ASSERT_EQ(fs::file_size(filePath), fs::file_size(savePath)) 
            << "Saved file size doesn't match original";
        
        // Check against performance thresholds
        auto& threshold = thresholds[sizeLabel];
        ASSERT_LE(saveTimeMs, threshold.saveTimeMs * 1.5) 
            << sizeLabel << " file save time exceeded threshold by too much";
    }
    
    /**
     * Test scrolling through a file and measure performance
     */
    void testScrolling(const std::string& filePath, const std::string& sizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << sizeLabel;
            return;
        }
        
        // First open the file
        ASSERT_TRUE(editor->openFile(filePath)) 
            << "Failed to open " << sizeLabel << " test file for scrolling test";
        
        // Get total line count
        const TextBuffer& buffer = editor->getBuffer();
        int totalLines = buffer.lineCount();
        
        // Measure scrolling time - page down through file
        int pageSize = 40; // Typical page size
        int totalScrolls = std::min(50, totalLines / pageSize); // Cap at 50 scrolls
        
        double scrollTimeMs = MeasureExecutionTimeMs([&]() {
            for (int i = 0; i < totalScrolls; i++) {
                editor->pageDown();
            }
        });
        
        // Calculate average scroll time per page
        double avgScrollTimeMs = scrollTimeMs / totalScrolls;
        
        // Output metrics
        std::cout << "[" << sizeLabel << "] Total scroll time: " << scrollTimeMs << " ms" << std::endl;
        std::cout << "[" << sizeLabel << "] Average page down time: " << avgScrollTimeMs << " ms" << std::endl;
        
        // Check against performance thresholds
        auto& threshold = thresholds[sizeLabel];
        ASSERT_LE(avgScrollTimeMs, threshold.scrollTimeMs) 
            << sizeLabel << " scrolling time exceeded threshold";
    }
    
    /**
     * Test searching through a file and measure performance
     */
    void testSearching(const std::string& filePath, const std::string& sizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << sizeLabel;
            return;
        }
        
        // First open the file
        ASSERT_TRUE(editor->openFile(filePath)) 
            << "Failed to open " << sizeLabel << " test file for search test";
        
        // Define search patterns (common words likely to be found)
        const std::vector<std::string> searchPatterns = {"the", "test", "file", "editor", "performance"};
        
        // Measure search time for all patterns
        double totalSearchTimeMs = 0;
        int searchCount = 0;
        
        for (const auto& pattern : searchPatterns) {
            double searchTimeMs = MeasureExecutionTimeMs([&]() {
                editor->findNext(pattern);
            });
            
            totalSearchTimeMs += searchTimeMs;
            searchCount++;
            
            std::cout << "[" << sizeLabel << "] Search time for '" << pattern << "': " 
                      << searchTimeMs << " ms" << std::endl;
        }
        
        // Calculate average search time
        double avgSearchTimeMs = totalSearchTimeMs / searchCount;
        std::cout << "[" << sizeLabel << "] Average search time: " << avgSearchTimeMs << " ms" << std::endl;
        
        // Check against performance thresholds
        auto& threshold = thresholds[sizeLabel];
        ASSERT_LE(avgSearchTimeMs, threshold.searchTimeMs) 
            << sizeLabel << " search time exceeded threshold";
    }
    
    /**
     * Test inserting text and measure performance
     */
    void testInserting(const std::string& filePath, const std::string& sizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << sizeLabel;
            return;
        }
        
        // First open the file
        ASSERT_TRUE(editor->openFile(filePath)) 
            << "Failed to open " << sizeLabel << " test file for insert test";
        
        // Get total line count and move to middle of file
        const TextBuffer& buffer = editor->getBuffer();
        int totalLines = buffer.lineCount();
        int middleLine = totalLines / 2;
        
        // Set cursor to beginning of middle line
        editor->setCursor(middleLine, 0);
        
        // Track memory before insertion
        size_t memoryBefore = MemoryTracker::getCurrentMemoryUsage();
        
        // Create test text (5KB)
        std::string testText(5 * 1024, 'X');
        
        // Measure insertion time
        double insertTimeMs = MeasureExecutionTimeMs([&]() {
            editor->typeText(testText);
        });
        
        // Track memory after insertion
        size_t memoryAfter = MemoryTracker::getCurrentMemoryUsage();
        size_t memoryDiff = memoryAfter - memoryBefore;
        
        // Output metrics
        std::cout << "[" << sizeLabel << "] Text insertion time: " << insertTimeMs << " ms" << std::endl;
        std::cout << "[" << sizeLabel << "] Memory increase: " << memoryDiff / 1024 << " KB" << std::endl;
        
        // Check against performance thresholds
        auto& threshold = thresholds[sizeLabel];
        ASSERT_LE(insertTimeMs, threshold.insertTimeMs * 1.5) 
            << sizeLabel << " insert time exceeded threshold by too much";
        
        // Verify the insertion was successful
        ASSERT_TRUE(editor->getBuffer().lineCount() >= totalLines) 
            << "Line count decreased after insertion";
    }
};

/**
 * Comprehensive test for medium-large files (just over 10MB)
 */
TEST_F(ExtremeLargeFileTest, MediumLargeFileTest) {
    std::cout << "\n===== Testing Medium-Large File (12MB) =====" << std::endl;
    
    // Test file operations
    testFileOpen(mediumLargeFilePath_, "MediumLarge");
    testFileSave(mediumLargeFilePath_, "MediumLarge");
    testScrolling(mediumLargeFilePath_, "MediumLarge");
    testSearching(mediumLargeFilePath_, "MediumLarge");
    testInserting(mediumLargeFilePath_, "MediumLarge");
    
    // Clean up
    closeCurrentFile();
}

/**
 * Comprehensive test for very large files (50MB)
 */
TEST_F(ExtremeLargeFileTest, VeryLargeFileTest) {
    std::cout << "\n===== Testing Very Large File (50MB) =====" << std::endl;
    
    // Test file operations
    testFileOpen(veryLargeFilePath_, "VeryLarge");
    testFileSave(veryLargeFilePath_, "VeryLarge");
    testScrolling(veryLargeFilePath_, "VeryLarge");
    testSearching(veryLargeFilePath_, "VeryLarge");
    testInserting(veryLargeFilePath_, "VeryLarge");
    
    // Clean up
    closeCurrentFile();
}

/**
 * Comprehensive test for extremely large files (150MB)
 */
TEST_F(ExtremeLargeFileTest, ExtremeLargeFileTest) {
    std::cout << "\n===== Testing Extreme Large File (150MB) =====" << std::endl;
    
    // Skip if running in CI environment
    if (std::getenv("CI") != nullptr) {
        GTEST_SKIP() << "Skipping extremely large file test in CI environment";
        return;
    }
    
    // Test file operations
    testFileOpen(extremeLargeFilePath_, "ExtremeLarge");
    testFileSave(extremeLargeFilePath_, "ExtremeLarge");
    testScrolling(extremeLargeFilePath_, "ExtremeLarge");
    testSearching(extremeLargeFilePath_, "ExtremeLarge");
    testInserting(extremeLargeFilePath_, "ExtremeLarge");
    
    // Clean up
    closeCurrentFile();
}

/**
 * Comprehensive test for ultra-large files (500MB)
 * Only runs if ULTRA_LARGE_TESTS environment variable is set
 */
TEST_F(ExtremeLargeFileTest, UltraLargeFileTest) {
    std::cout << "\n===== Testing Ultra Large File (500MB) =====" << std::endl;
    
    // Skip if ultra large tests not enabled
    if (std::getenv("ULTRA_LARGE_TESTS") == nullptr) {
        GTEST_SKIP() << "Skipping ultra large file test (enable with ULTRA_LARGE_TESTS env var)";
        return;
    }
    
    // Test file operations
    testFileOpen(ultraLargeFilePath_, "UltraLarge");
    testFileSave(ultraLargeFilePath_, "UltraLarge");
    testScrolling(ultraLargeFilePath_, "UltraLarge");
    testSearching(ultraLargeFilePath_, "UltraLarge");
    testInserting(ultraLargeFilePath_, "UltraLarge");
    
    // Clean up
    closeCurrentFile();
}

/**
 * Test to verify that after working with extremely large files,
 * the editor returns to normal performance when working with smaller files
 */
TEST_F(ExtremeLargeFileTest, PerformanceRecoveryTest) {
    // Skip if running in CI environment
    if (std::getenv("CI") != nullptr) {
        GTEST_SKIP() << "Skipping performance recovery test in CI environment";
        return;
    }
    
    // Test sequence: medium -> extreme -> medium again
    // First, open and work with medium file
    std::cout << "\n===== Performance Recovery Test - Initial Medium File =====" << std::endl;
    testFileOpen(mediumLargeFilePath_, "MediumLarge");
    testScrolling(mediumLargeFilePath_, "MediumLarge");
    closeCurrentFile();
    
    // Now open and work with extreme file
    std::cout << "\n===== Performance Recovery Test - Extreme File =====" << std::endl;
    testFileOpen(extremeLargeFilePath_, "ExtremeLarge");
    testScrolling(extremeLargeFilePath_, "ExtremeLarge");
    closeCurrentFile();
    
    // Now open medium file again and verify performance
    std::cout << "\n===== Performance Recovery Test - Medium File Again =====" << std::endl;
    
    // Measure performance metrics with medium file again
    double openTimeMs = MeasureExecutionTimeMs([&]() {
        ASSERT_TRUE(editor->openFile(mediumLargeFilePath_)) 
            << "Failed to open medium test file for recovery test";
    });
    
    // Get baseline metrics for comparison
    auto& threshold = thresholds["MediumLarge"];
    
    // Verify performance is still within acceptable range
    std::cout << "Recovery test - medium file open time: " << openTimeMs << " ms" << std::endl;
    ASSERT_LE(openTimeMs, threshold.openTimeMs * 1.2) 
        << "Performance did not recover after working with extreme file";
    
    // Test scrolling performance recovery
    double scrollTimeMs = MeasureExecutionTimeMs([&]() {
        for (int i = 0; i < 20; i++) {
            editor->pageDown();
        }
    });
    
    std::cout << "Recovery test - medium file scroll time: " << scrollTimeMs / 20 << " ms per page" << std::endl;
    ASSERT_LE(scrollTimeMs / 20, threshold.scrollTimeMs * 1.2) 
        << "Scrolling performance did not recover after working with extreme file";
    
    // Clean up
    closeCurrentFile();
}

/**
 * Main function to run the tests
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=====================================" << std::endl;
    std::cout << "Extreme Large File Performance Tests" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Output system info
    std::cout << "System Information:" << std::endl;
#ifdef _WIN32
    std::cout << "  OS: Windows" << std::endl;
#elif defined(__APPLE__)
    std::cout << "  OS: macOS" << std::endl;
#else
    std::cout << "  OS: Linux" << std::endl;
#endif
    
    // Output environment variables
    std::cout << "Environment:" << std::endl;
    std::cout << "  CI: " << (std::getenv("CI") ? "Yes" : "No") << std::endl;
    std::cout << "  ULTRA_LARGE_TESTS: " << (std::getenv("ULTRA_LARGE_TESTS") ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  KEEP_TEST_FILES: " << (std::getenv("KEEP_TEST_FILES") ? "Yes" : "No") << std::endl;
    
    std::cout << "=====================================" << std::endl;
    
    return RUN_ALL_TESTS();
} 