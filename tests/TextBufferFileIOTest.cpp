#include "gtest/gtest.h"
#include "../src/TextBuffer.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class TextBufferFileIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<TextBuffer>();
        testDir = fs::temp_directory_path() / "TextBufferTest";
        fs::create_directories(testDir);
        testFile = testDir / "testfile.txt";
        
        // Initialize with some content
        buffer->addLine("First line");
        buffer->addLine("Second line with some text");
        buffer->addLine("Third line");
    }
    
    void TearDown() override {
        // Clean up test files
        try {
            if (fs::exists(testFile)) {
                fs::remove(testFile);
            }
            if (fs::exists(testDir)) {
                fs::remove_all(testDir);
            }
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    std::unique_ptr<TextBuffer> buffer;
    fs::path testDir;
    fs::path testFile;
};

// Test basic save and load functionality
TEST_F(TextBufferFileIOTest, SaveAndLoadBasic) {
    // Save the buffer to a file
    ASSERT_TRUE(buffer->saveToFile(testFile.string()));
    
    // Create a new buffer and load from the file
    TextBuffer loadedBuffer;
    ASSERT_TRUE(loadedBuffer.loadFromFile(testFile.string()));
    
    // Verify content matches
    ASSERT_EQ(buffer->getAllLines(), loadedBuffer.getAllLines());
}

// Test saving to a non-existent directory
TEST_F(TextBufferFileIOTest, SaveToNonExistentDirectory) {
    fs::path nonExistentDir = testDir / "nonexistent";
    fs::path filePath = nonExistentDir / "test.txt";
    
    // Should fail because directory doesn't exist
    EXPECT_FALSE(buffer->saveToFile(filePath.string()));
    
    // Create directory and try again
    fs::create_directories(nonExistentDir);
    EXPECT_TRUE(buffer->saveToFile(filePath.string()));
}

// Test loading a non-existent file
TEST_F(TextBufferFileIOTest, LoadNonExistentFile) {
    fs::path nonExistentFile = testDir / "nonexistent.txt";
    
    // Should fail because file doesn't exist
    EXPECT_FALSE(buffer->loadFromFile(nonExistentFile.string()));
    
    // Verify buffer content is unchanged
    EXPECT_EQ(buffer->lineCount(), 3);
    EXPECT_EQ(buffer->getLine(0), "First line");
}

// Test saving an empty buffer
TEST_F(TextBufferFileIOTest, SaveEmptyBuffer) {
    TextBuffer emptyBuffer;
    emptyBuffer.clear(false); // Clear without keeping empty line
    
    ASSERT_TRUE(emptyBuffer.saveToFile(testFile.string()));
    
    // Verify file exists and is empty or contains a single empty line
    std::ifstream inFile(testFile);
    std::string line;
    bool hasContent = false;
    while (std::getline(inFile, line)) {
        hasContent = true;
        EXPECT_TRUE(line.empty());
    }
    
    // Either empty file or single empty line is acceptable
    if (hasContent) {
        EXPECT_EQ(emptyBuffer.lineCount(), 1);
        EXPECT_TRUE(emptyBuffer.getLine(0).empty());
    }
}

// Test loading a file with various line endings (\n, \r\n, \r)
TEST_F(TextBufferFileIOTest, LoadWithDifferentLineEndings) {
    // Test with LF line endings
    {
        std::ofstream outFile(testFile, std::ios::binary);
        outFile << "Line 1\nLine 2\nLine 3\n";
    }
    
    TextBuffer lfBuffer;
    ASSERT_TRUE(lfBuffer.loadFromFile(testFile.string()));
    ASSERT_EQ(lfBuffer.lineCount(), 3);
    EXPECT_EQ(lfBuffer.getLine(0), "Line 1");
    
    // Test with CRLF line endings
    {
        std::ofstream outFile(testFile, std::ios::binary);
        outFile << "Line 1\r\nLine 2\r\nLine 3\r\n";
    }
    
    TextBuffer crlfBuffer;
    ASSERT_TRUE(crlfBuffer.loadFromFile(testFile.string()));
    ASSERT_EQ(crlfBuffer.lineCount(), 3);
    EXPECT_EQ(crlfBuffer.getLine(0), "Line 1");
    
    // Test with CR line endings (old Mac style)
    {
        std::ofstream outFile(testFile, std::ios::binary);
        outFile << "Line 1\rLine 2\rLine 3\r";
    }
    
    TextBuffer crBuffer;
    ASSERT_TRUE(crBuffer.loadFromFile(testFile.string()));
    ASSERT_GE(crBuffer.lineCount(), 1);
    EXPECT_EQ(crBuffer.getLine(0), "Line 1");
}

// Test saving and loading special characters and Unicode
TEST_F(TextBufferFileIOTest, SaveAndLoadUnicode) {
    // Clear and add test content with special characters
    buffer->clear(false); // Clear without keeping empty line
    buffer->addLine("Line with special chars: äöüß");
    buffer->addLine("Line with emoji: 😊");
    buffer->addLine("Line with Chinese: 你好");
    
    ASSERT_TRUE(buffer->saveToFile(testFile.string()));
    
    TextBuffer loadedBuffer;
    ASSERT_TRUE(loadedBuffer.loadFromFile(testFile.string()));
    
    // Verify content matches
    ASSERT_EQ(buffer->getAllLines(), loadedBuffer.getAllLines());
}

// Test saving to and loading from the same file
TEST_F(TextBufferFileIOTest, SaveAndLoadSameFile) {
    // Save initial content
    ASSERT_TRUE(buffer->saveToFile(testFile.string()));
    
    // Modify the buffer
    buffer->addLine("Additional line");
    
    // Save again to same file
    ASSERT_TRUE(buffer->saveToFile(testFile.string()));
    
    // Load and verify
    TextBuffer loadedBuffer;
    ASSERT_TRUE(loadedBuffer.loadFromFile(testFile.string()));
    ASSERT_EQ(buffer->getAllLines(), loadedBuffer.getAllLines());
}

// Test error handling for file operations
TEST_F(TextBufferFileIOTest, ErrorHandling) {
    // Test saving to an invalid path (e.g., root directory on Unix-like systems)
    if (fs::exists("/")) {  // Only run on Unix-like systems
        EXPECT_FALSE(buffer->saveToFile("/invalid/path/test.txt"));
    }
    
    // Test loading from a directory instead of a file
    EXPECT_FALSE(buffer->loadFromFile(testDir.string()));
    
    // Test loading an empty file
    {
        std::ofstream outFile(testFile);
        // Empty file
    }
    
    TextBuffer emptyFileBuffer;
    EXPECT_TRUE(emptyFileBuffer.loadFromFile(testFile.string()));
    EXPECT_EQ(emptyFileBuffer.lineCount(), 1);  // Should have one empty line
    EXPECT_TRUE(emptyFileBuffer.getLine(0).empty());
}

// Test loading a very large file (performance test)
TEST_F(TextBufferFileIOTest, DISABLED_LoadLargeFile) {
    // Create a large file (10,000 lines)
    {
        std::ofstream outFile(testFile);
        for (int i = 0; i < 10000; ++i) {
            outFile << "This is line " << i << " of a large test file.\n";
        }
    }
    
    // Time the load operation
    auto start = std::chrono::high_resolution_clock::now();
    
    TextBuffer largeBuffer;
    EXPECT_TRUE(largeBuffer.loadFromFile(testFile.string()));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify content
    EXPECT_EQ(largeBuffer.lineCount(), 10000);
    EXPECT_EQ(largeBuffer.getLine(0), "This is line 0 of a large test file.");
    EXPECT_EQ(largeBuffer.getLine(9999), "This is line 9999 of a large test file.");
    
    std::cout << "Loaded 10,000 lines in " << duration.count() << "ms" << std::endl;
}

// Test saving with different encodings (if supported)
TEST_F(TextBufferFileIOTest, SaveWithDifferentEncodings) {
    // Test UTF-8 (default)
    buffer->clear(false); // Clear without keeping empty line
    buffer->addLine(u8"UTF-8: äöüß 你好 😊");
    
    fs::path utf8File = testDir / "utf8.txt";
    ASSERT_TRUE(buffer->saveToFile(utf8File.string()));
    
    // Test loading back
    TextBuffer loadedBuffer;
    ASSERT_TRUE(loadedBuffer.loadFromFile(utf8File.string()));
    ASSERT_EQ(buffer->getAllLines(), loadedBuffer.getAllLines());
    
    // Note: Testing other encodings would require conversion libraries
    // like iconv, which would be tested separately
}

// Test handling of very long lines
TEST_F(TextBufferFileIOTest, VeryLongLine) {
    // Create a very long line
    std::string longLine(10000, 'x');
    buffer->clear(false);
    buffer->addLine(longLine);
    
    ASSERT_TRUE(buffer->saveToFile(testFile.string()));
    
    TextBuffer loadedBuffer;
    ASSERT_TRUE(loadedBuffer.loadFromFile(testFile.string()));
    
    ASSERT_EQ(loadedBuffer.lineCount(), 1);
    EXPECT_EQ(loadedBuffer.getLine(0), longLine);
}
