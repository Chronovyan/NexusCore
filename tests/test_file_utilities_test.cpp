#include <gtest/gtest.h>
#include "test_file_utilities.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class TestFileUtilitiesTest : public ::testing::Test {
protected:
    // Test files to be cleaned up
    std::vector<std::string> testFiles;
    
    void SetUp() override {
        // Create a test output directory if it doesn't exist
        if (!fs::exists("test_output")) {
            fs::create_directory("test_output");
        }
    }
    
    void TearDown() override {
        // Clean up any generated test files
        for (const auto& file : testFiles) {
            if (fs::exists(file)) {
                try {
                    fs::remove(file);
                } catch (const std::exception& e) {
                    std::cerr << "Error removing test file: " << e.what() << std::endl;
                }
            }
        }
    }
};

// Test the file generation utility with different patterns and line endings
TEST_F(TestFileUtilitiesTest, GenerateFileTest) {
    // Test parameters
    const size_t testSize = 1024 * 10; // 10KB - small for unit testing
    
    // Test each content pattern
    std::vector<TestFileGenerator::ContentPattern> patterns = {
        TestFileGenerator::ContentPattern::SEQUENTIAL_NUMBERS,
        TestFileGenerator::ContentPattern::REPEATED_TEXT,
        TestFileGenerator::ContentPattern::RANDOM_TEXT,
        TestFileGenerator::ContentPattern::CODE_LIKE,
        TestFileGenerator::ContentPattern::MIXED_LINE_LENGTHS,
        TestFileGenerator::ContentPattern::MIXED_LINE_ENDINGS
    };
    
    // Test each line ending
    std::vector<TestFileGenerator::LineEnding> endings = {
        TestFileGenerator::LineEnding::LF,
        TestFileGenerator::LineEnding::CRLF,
        TestFileGenerator::LineEnding::CR,
        TestFileGenerator::LineEnding::MIXED
    };
    
    // Test a few combinations to keep test time reasonable
    for (auto pattern : patterns) {
        for (auto ending : endings) {
            // Generate a unique filename for this test
            std::string filename = "test_output/test_file_" + 
                                  std::to_string(static_cast<int>(pattern)) + "_" +
                                  std::to_string(static_cast<int>(ending)) + ".txt";
            
            testFiles.push_back(filename);
            
            // Generate the file
            ASSERT_NO_THROW({
                std::string result = TestFileGenerator::generateFile(
                    testSize, filename, pattern, ending);
                EXPECT_EQ(result, filename);
            }) << "Failed to generate file with pattern " << static_cast<int>(pattern)
               << " and line ending " << static_cast<int>(ending);
            
            // Verify the file exists and has content
            ASSERT_TRUE(fs::exists(filename));
            EXPECT_GT(fs::file_size(filename), 0);
            
            // For small test files, we can verify some content
            std::ifstream file(filename, std::ios::binary);
            ASSERT_TRUE(file.is_open());
            
            // Read first few bytes to make sure there's actual content
            char buffer[100];
            file.read(buffer, sizeof(buffer));
            EXPECT_GT(file.gcount(), 0);
            
            file.close();
        }
    }
}

// Test the memory tracking utility
TEST_F(TestFileUtilitiesTest, MemoryTrackerTest) {
    // Get initial memory usage
    size_t initialMemory = MemoryTracker::getCurrentMemoryUsage();
    EXPECT_GT(initialMemory, 0) << "Memory tracking function returned 0, which is unlikely to be correct";
    
    // Test trackPeakMemoryDuring with a simple allocation
    size_t peakMemory = MemoryTracker::trackPeakMemoryDuring([]() {
        // Allocate a large block of memory
        std::vector<int> largeArray(1024 * 1024 * 10); // 40MB (10M integers)
        
        // Fill it to ensure it's actually allocated
        for (size_t i = 0; i < largeArray.size(); ++i) {
            largeArray[i] = static_cast<int>(i);
        }
        
        // Sleep to ensure memory monitor has time to detect it
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    
    // Verify peak memory was higher than initial memory
    EXPECT_GT(peakMemory, initialMemory) << "Peak memory should be higher than initial memory";
    
    // Get memory after operation (should be close to initial memory again)
    size_t finalMemory = MemoryTracker::getCurrentMemoryUsage();
    
    // Log memory values for debugging
    std::cout << "Initial memory: " << (initialMemory / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "Peak memory: " << (peakMemory / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "Final memory: " << (finalMemory / (1024 * 1024)) << " MB" << std::endl;
    
    // We expect peak memory to be at least 10MB more than initial
    EXPECT_GT(peakMemory, initialMemory + 1024 * 1024 * 10) 
        << "Peak memory should show at least 10MB increase during test";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 