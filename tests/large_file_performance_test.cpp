#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <fstream>

#include "Editor.h"
#include "WorkspaceManager.h"
#include "test_file_utilities.h"

namespace fs = std::filesystem;

/**
 * Test fixture for measuring large file operations performance
 */
class LargeFileTest : public ::testing::Test {
protected:
    // File sizes for testing
    const size_t SMALL_FILE_SIZE = 1 * 1024 * 1024;    // 1MB
    const size_t MEDIUM_FILE_SIZE = 10 * 1024 * 1024;  // 10MB
    const size_t LARGE_FILE_SIZE = 50 * 1024 * 1024;   // 50MB
    const size_t VERY_LARGE_FILE_SIZE = 100 * 1024 * 1024; // 100MB (optional)
    
    // Test subjects
    std::unique_ptr<Editor> editor;
    
    // Test file paths
    std::string smallFilePath_;
    std::string mediumFilePath_;
    std::string largeFilePath_;
    std::string veryLargeFilePath_;
    std::string emptyFilePath_; // For "closing" files
    
    // Track generated files for cleanup
    std::vector<std::string> generatedTestFiles;
    
    // Test output directory
    const std::string testOutputDir = "test_output/large_files/";
    
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
        
        // Generate test files of different sizes
        try {
            std::cout << "Generating test files..." << std::endl;
            
            smallFilePath_ = TestFileGenerator::generateFile(
                SMALL_FILE_SIZE,
                testOutputDir + "small_test_file.txt",
                TestFileGenerator::ContentPattern::REPEATED_TEXT,
                TestFileGenerator::LineEnding::LF
            );
            generatedTestFiles.push_back(smallFilePath_);
            std::cout << "Small file generated: " << smallFilePath_ << std::endl;
            
            mediumFilePath_ = TestFileGenerator::generateFile(
                MEDIUM_FILE_SIZE,
                testOutputDir + "medium_test_file.txt",
                TestFileGenerator::ContentPattern::REPEATED_TEXT,
                TestFileGenerator::LineEnding::LF
            );
            generatedTestFiles.push_back(mediumFilePath_);
            std::cout << "Medium file generated: " << mediumFilePath_ << std::endl;
            
            largeFilePath_ = TestFileGenerator::generateFile(
                LARGE_FILE_SIZE,
                testOutputDir + "large_test_file.txt",
                TestFileGenerator::ContentPattern::REPEATED_TEXT,
                TestFileGenerator::LineEnding::LF
            );
            generatedTestFiles.push_back(largeFilePath_);
            std::cout << "Large file generated: " << largeFilePath_ << std::endl;
            
            // Only generate very large file if explicitly enabled
            if (::testing::GTEST_FLAG(filter).find("VeryLargeFile") != std::string::npos) {
                veryLargeFilePath_ = TestFileGenerator::generateFile(
                    VERY_LARGE_FILE_SIZE,
                    testOutputDir + "very_large_test_file.txt",
                    TestFileGenerator::ContentPattern::REPEATED_TEXT,
                    TestFileGenerator::LineEnding::LF
                );
                generatedTestFiles.push_back(veryLargeFilePath_);
                std::cout << "Very large file generated: " << veryLargeFilePath_ << std::endl;
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
        
        // Clean up test files
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
};

/**
 * Test to measure time taken to open files of different sizes
 */
TEST_F(LargeFileTest, MeasureFileOpenTime) {
    // Define a helper function to test opening a single file
    auto testOpenFile = [&](const std::string& filePath, const std::string& fileSizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << fileSizeLabel;
            return;
        }
        
        double openTimeMs = MeasureExecutionTimeMs([&]() {
            ASSERT_TRUE(editor->openFile(filePath)) << "Failed to open " << fileSizeLabel << " test file: " << filePath;
        });
        
        std::cout << "Time to open " << fileSizeLabel << " (" 
                  << fs::file_size(filePath) / (1024 * 1024) << "MB) file: " 
                  << openTimeMs << " ms" << std::endl;
        
        // Verify file was opened successfully by checking line count
        const TextBuffer& buffer = editor->getBuffer();
        ASSERT_GT(buffer.lineCount(), 0) << "File doesn't appear to be loaded: " << fileSizeLabel;
        
        // Measure memory usage while file is open
        size_t memoryUsage = MemoryTracker::getCurrentMemoryUsage();
        std::cout << "Memory usage after opening " << fileSizeLabel << " file: " 
                  << memoryUsage / (1024 * 1024) << " MB" << std::endl;
                  
        // Close the file after measuring
        closeCurrentFile();
    };
    
    // Test opening each file size
    std::cout << "\n===== Testing Small File (" << SMALL_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
    testOpenFile(smallFilePath_, "Small");
    
    std::cout << "\n===== Testing Medium File (" << MEDIUM_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
    testOpenFile(mediumFilePath_, "Medium");
    
    std::cout << "\n===== Testing Large File (" << LARGE_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
    testOpenFile(largeFilePath_, "Large");
    
    if (!veryLargeFilePath_.empty() && fs::exists(veryLargeFilePath_)) {
        std::cout << "\n===== Testing Very Large File (" << VERY_LARGE_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
        testOpenFile(veryLargeFilePath_, "Very Large");
    }
}

/**
 * Test to measure memory usage during editing operations
 */
TEST_F(LargeFileTest, MeasureMemoryDuringEditing) {
    // Open medium-sized file for editing
    ASSERT_TRUE(editor->openFile(mediumFilePath_)) << "Failed to open medium test file";
    
    // Track baseline memory
    size_t baselineMemory = MemoryTracker::getCurrentMemoryUsage();
    std::cout << "Baseline memory usage: " << baselineMemory / (1024 * 1024) << " MB" << std::endl;
    
    // Move to a line in the middle of the file
    int totalLines = editor->getBuffer().lineCount();
    int middleLine = totalLines / 2;
    
    // Set cursor to beginning of middle line
    editor->setCursor(middleLine, 0);
    
    // Measure peak memory during insertion
    size_t peakMemory = MemoryTracker::trackPeakMemoryDuring([&]() {
        // Insert a large amount of text (100KB)
        std::string largeText(100 * 1024, 'X');
        editor->typeText(largeText);
        
        // Perform some cursor movements to simulate user interaction
        for (int i = 0; i < 10; i++) {
            editor->moveCursorDown();
            editor->moveCursorRight();
        }
    });
    
    double insertTimeMs = MeasureExecutionTimeMs([&]() {
        // Insert another chunk of text to measure performance
        editor->typeText("Performance measurement text");
    });
    
    std::cout << "Time to insert additional text: " << insertTimeMs << " ms" << std::endl;
    
    // Check memory usage after operations
    std::cout << "Peak memory usage: " << peakMemory / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Memory usage increase: " << (peakMemory - baselineMemory) / (1024) << " KB" << std::endl;
    
    // Check if memory usage is within acceptable limits
    ASSERT_LE(peakMemory - baselineMemory, baselineMemory * 0.5) 
        << "Memory usage increased by more than 50% during text insertion";
    
    // Check editor is still responsive after large insertion
    double navigationTimeMs = MeasureExecutionTimeMs([&]() {
        for (int i = 0; i < 10; i++) {
            // Perform 10 movements to test responsiveness
            editor->moveCursorDown();
            editor->moveCursorRight();
        }
    });
    
    std::cout << "Navigation time after insertion: " << navigationTimeMs << " ms" << std::endl;
    ASSERT_LE(navigationTimeMs, 100.0) << "Editor navigation became slow after insertion";
    
    // Clean up
    closeCurrentFile();
}

/**
 * Test to measure time taken to save files of different sizes
 */
TEST_F(LargeFileTest, MeasureFileSaveTime) {
    // Create output path for saving
    std::string saveOutputDir = testOutputDir + "save_tests/";
    if (!fs::exists(saveOutputDir)) {
        fs::create_directories(saveOutputDir);
    }
    
    // Define a helper function to test saving a single file
    auto testSaveFile = [&](const std::string& filePath, const std::string& fileSizeLabel) {
        if (filePath.empty() || !fs::exists(filePath)) {
            GTEST_SKIP() << "Test file not generated or path empty for " << fileSizeLabel;
            return;
        }
        
        // Open file
        ASSERT_TRUE(editor->openFile(filePath)) << "Failed to open " << fileSizeLabel << " test file";
        
        // Create path for saving
        std::string savePath = saveOutputDir + "saved_" + fileSizeLabel + "_file.txt";
        generatedTestFiles.push_back(savePath); // Mark for cleanup
        
        // Measure save time
        double saveTimeMs = MeasureExecutionTimeMs([&]() {
            ASSERT_TRUE(editor->saveFile(savePath)) << "Failed to save " << fileSizeLabel << " test file";
        });
        
        std::cout << "Time to save " << fileSizeLabel << " (" 
                  << fs::file_size(filePath) / (1024 * 1024) << "MB) file: " 
                  << saveTimeMs << " ms" << std::endl;
        
        // Verify saved file exists and has reasonable size (not exact match due to possible line endings conversion)
        ASSERT_TRUE(fs::exists(savePath)) << "Saved file not found: " << savePath;
        
        // Size ratio should be within 5% (allowing for line ending differences)
        double sizeRatio = static_cast<double>(fs::file_size(savePath)) / static_cast<double>(fs::file_size(filePath));
        std::cout << "Saved file size: " << fs::file_size(savePath) << " bytes, Original size: " 
                  << fs::file_size(filePath) << " bytes, Ratio: " << sizeRatio << std::endl;
        
        ASSERT_TRUE(sizeRatio > 0.95 && sizeRatio < 1.05) 
            << "Saved file size differs significantly from original";
        
        // Close file after measuring
        closeCurrentFile();
    };
    
    // Test saving each file size
    std::cout << "\n===== Testing Small File Save (" << SMALL_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
    testSaveFile(smallFilePath_, "Small");
    
    std::cout << "\n===== Testing Medium File Save (" << MEDIUM_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
    testSaveFile(mediumFilePath_, "Medium");
    
    std::cout << "\n===== Testing Large File Save (" << LARGE_FILE_SIZE / (1024 * 1024) << "MB) =====" << std::endl;
    testSaveFile(largeFilePath_, "Large");
    
    // Clean up save directory
    try {
        if (fs::exists(saveOutputDir)) {
            fs::remove_all(saveOutputDir);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error removing save test directory: " << e.what() << std::endl;
    }
}

/**
 * Test to measure scrolling performance in large files
 */
TEST_F(LargeFileTest, MeasureScrollingPerformance) {
    // Open large file
    ASSERT_TRUE(editor->openFile(largeFilePath_)) << "Failed to open large test file";
    
    int totalLines = editor->getBuffer().lineCount();
    std::cout << "Total lines in file: " << totalLines << std::endl;
    
    // Set cursor to beginning
    editor->setCursor(0, 0);
    
    // Measure time to scroll down through the file
    double scrollDownTimeMs = MeasureExecutionTimeMs([&]() {
        // Move down by 1000 lines or to end, whichever comes first
        int scrollCount = std::min<int>(1000, totalLines - 1);
        for (int i = 0; i < scrollCount; i++) {
            editor->moveCursorDown();
        }
    });
    
    std::cout << "Time to scroll down 1000 lines: " << scrollDownTimeMs << " ms" << std::endl;
    ASSERT_LE(scrollDownTimeMs, 500.0) << "Scrolling down is too slow (> 500ms for 1000 lines)";
    
    // Measure time to scroll back up
    double scrollUpTimeMs = MeasureExecutionTimeMs([&]() {
        // Move up by 1000 lines or to beginning, whichever comes first
        int currentLine = editor->getCursorLine();
        int scrollCount = std::min<int>(1000, currentLine);
        for (int i = 0; i < scrollCount; i++) {
            editor->moveCursorUp();
        }
    });
    
    std::cout << "Time to scroll up 1000 lines: " << scrollUpTimeMs << " ms" << std::endl;
    ASSERT_LE(scrollUpTimeMs, 500.0) << "Scrolling up is too slow (> 500ms for 1000 lines)";
    
    // Measure time to jump to end of file
    double jumpToEndTimeMs = MeasureExecutionTimeMs([&]() {
        editor->moveCursorToBufferEnd();
    });
    
    std::cout << "Time to jump to end of file: " << jumpToEndTimeMs << " ms" << std::endl;
    ASSERT_LE(jumpToEndTimeMs, 100.0) << "Jumping to end of file is too slow";
    
    // Measure time to jump to beginning of file
    double jumpToBeginningTimeMs = MeasureExecutionTimeMs([&]() {
        editor->moveCursorToBufferStart();
    });
    
    std::cout << "Time to jump to beginning of file: " << jumpToBeginningTimeMs << " ms" << std::endl;
    ASSERT_LE(jumpToBeginningTimeMs, 100.0) << "Jumping to beginning of file is too slow";
    
    // Clean up
    closeCurrentFile();
}

/**
 * Test to verify content integrity in large files
 */
TEST_F(LargeFileTest, VerifyLargeFileContentIntegrity) {
    // Open large file
    ASSERT_TRUE(editor->openFile(largeFilePath_)) << "Failed to open large test file";
    
    // Create output path for saving
    std::string saveOutputDir = testOutputDir + "integrity_tests/";
    if (!fs::exists(saveOutputDir)) {
        fs::create_directories(saveOutputDir);
    }
    
    std::string savePath = saveOutputDir + "integrity_test_save.txt";
    generatedTestFiles.push_back(savePath);
    
    // Define marker strings
    const std::string beginMarker = "INTEGRITY_TEST_BEGIN";
    const std::string endMarker = "INTEGRITY_TEST_END";
    
    // Make simple modification at beginning
    editor->setCursor(0, 0);
    editor->typeText(beginMarker + "\n");
    
    // Make simple modification at the end
    int lastLine = editor->getBuffer().lineCount() - 1;
    editor->setCursor(lastLine, 0);
    editor->typeText("\n" + endMarker);
    
    // Save the file
    ASSERT_TRUE(editor->saveFile(savePath)) << "Failed to save modified file";
    
    // Close and reopen to verify persistence
    closeCurrentFile();
    ASSERT_TRUE(editor->openFile(savePath)) << "Failed to reopen saved file";
    
    // Verify content at beginning
    const TextBuffer& buffer = editor->getBuffer();
    std::string firstLine = buffer.getLine(0);
    ASSERT_EQ(firstLine, beginMarker) << "Beginning content not preserved";
    
    // Verify content at end - just check that the marker exists in the last line
    lastLine = buffer.lineCount() - 1;
    std::string lastLineContent = buffer.getLine(lastLine);
    ASSERT_TRUE(lastLineContent.find(endMarker) != std::string::npos) 
        << "Ending marker not found in last line: " << lastLineContent;
    
    // Clean up
    closeCurrentFile();
    
    // Clean up integrity test directory
    try {
        if (fs::exists(saveOutputDir)) {
            fs::remove_all(saveOutputDir);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error removing integrity test directory: " << e.what() << std::endl;
    }
}

/**
 * Test to measure search and replace performance in large files
 */
TEST_F(LargeFileTest, MeasureSearchReplacePerformance) {
    // Only test on medium file for reasonable test duration
    ASSERT_TRUE(editor->openFile(mediumFilePath_)) << "Failed to open medium test file";
    
    // First, insert some unique search patterns
    const std::string searchPattern = "UNIQUE_SEARCH_PATTERN";
    const std::string replacePattern = "REPLACEMENT_PATTERN";
    
    // Insert search patterns at beginning, middle, and end
    editor->setCursor(0, 0);
    editor->typeText(searchPattern + "\n");
    
    int middleLine = editor->getBuffer().lineCount() / 2;
    editor->setCursor(middleLine, 0);
    editor->typeText(searchPattern + "\n");
    
    int lastLine = editor->getBuffer().lineCount() - 1;
    editor->setCursor(lastLine, 0);
    editor->typeText("\n" + searchPattern);
    
    // Save file to ensure patterns are stored
    std::string searchTestPath = testOutputDir + "search_test.txt";
    generatedTestFiles.push_back(searchTestPath);
    ASSERT_TRUE(editor->saveFile(searchTestPath)) << "Failed to save file with search patterns";
    
    // Simply verify we have 3 patterns in the file
    int searchCount = 0;
    for (int i = 0; i < editor->getBuffer().lineCount(); i++) {
        std::string line = editor->getBuffer().getLine(i);
        if (line.find(searchPattern) != std::string::npos) {
            searchCount++;
        }
    }
    ASSERT_EQ(searchCount, 3) << "Expected 3 patterns in file, found " << searchCount;
    
    // Measure time to manually search and replace
    double searchReplaceTimeMs = MeasureExecutionTimeMs([&]() {
        // Reset cursor to beginning
        editor->setCursor(0, 0);
        
        // Iterate through lines and perform replacements
        for (int i = 0; i < editor->getBuffer().lineCount(); i++) {
            std::string line = editor->getBuffer().getLine(i);
            size_t pos = line.find(searchPattern);
            
            if (pos != std::string::npos) {
                // Position cursor at the match
                editor->setCursor(i, pos);
                
                // Select the pattern
                editor->setSelectionRange(i, pos, i, pos + searchPattern.length());
                
                // Replace the selection
                editor->replaceSelection(replacePattern);
            }
        }
    });
    
    std::cout << "Time to manually search and replace patterns: " 
              << searchReplaceTimeMs << " ms" << std::endl;
    
    // Verify all replacements
    int replaceCount = 0;
    for (int i = 0; i < editor->getBuffer().lineCount(); i++) {
        std::string line = editor->getBuffer().getLine(i);
        if (line.find(replacePattern) != std::string::npos) {
            replaceCount++;
        }
    }
    ASSERT_EQ(replaceCount, 3) << "Expected 3 replacements, found " << replaceCount;
    
    // Clean up
    closeCurrentFile();
}

// Main function for running the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 