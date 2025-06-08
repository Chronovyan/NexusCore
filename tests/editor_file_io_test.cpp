#include "gtest/gtest.h"
#include "Editor.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio> // For std::remove
#include <filesystem> // For std::filesystem

class EditorFileIOTest : public ::testing::Test {
protected:
    Editor editor;
    const std::string test_dir = "tests/data/file_io/";
    const std::string test_file = test_dir + "test_file.txt";
    const std::string nonexistent_file = test_dir + "nonexistent_file.txt";
    const std::string read_only_file = test_dir + "read_only_file.txt";
    const std::string invalid_path = "*/invalid?path.txt";

    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories(test_dir);
        
        // Create a test file with some content
        std::ofstream file(test_file);
        file << "Line 1\n";
        file << "Line 2\n";
        file << "Line 3\n";
        file.close();
        
        // Create a read-only test file
        std::ofstream ro_file(read_only_file);
        ro_file << "Read only content\n";
        ro_file.close();
        
        // Set the file to read-only
        #ifdef _WIN32
        std::system(("attrib +r \"" + read_only_file + "\"").c_str());
        #else
        std::system(("chmod a-w \"" + read_only_file + "\"").c_str());
        #endif
    }

    void TearDown() override {
        // Remove test files
        std::remove(test_file.c_str());
        
        // Remove read-only protection
        #ifdef _WIN32
        std::system(("attrib -r \"" + read_only_file + "\"").c_str());
        #else
        std::system(("chmod a+w \"" + read_only_file + "\"").c_str());
        #endif
        
        std::remove(read_only_file.c_str());
        
        // Try to remove the test directory and its contents
        std::filesystem::remove_all(test_dir);
    }
};

// Test opening a valid file
TEST_F(EditorFileIOTest, OpenValidFile) {
    ASSERT_TRUE(editor.openFile(test_file));
    
    // Verify file content was loaded
    ASSERT_EQ(editor.getBuffer().lineCount(), 3);
    EXPECT_EQ(editor.getBuffer().getLine(0), "Line 1");
    EXPECT_EQ(editor.getBuffer().getLine(1), "Line 2");
    EXPECT_EQ(editor.getBuffer().getLine(2), "Line 3");
    
    // Verify filename was set correctly
    EXPECT_EQ(editor.getFilename(), test_file);
    
    // Verify cursor position was reset
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 0);
}

// Test opening a nonexistent file
TEST_F(EditorFileIOTest, OpenNonexistentFile) {
    ASSERT_FALSE(editor.openFile(nonexistent_file));
    
    // Verify editor state is unchanged (should still have one empty line)
    ASSERT_EQ(editor.getBuffer().lineCount(), 1);
    EXPECT_EQ(editor.getBuffer().getLine(0), "");
    EXPECT_EQ(editor.getFilename(), "untitled.txt"); // Default filename
}

// Test opening an invalid path
TEST_F(EditorFileIOTest, OpenInvalidPath) {
    ASSERT_FALSE(editor.openFile(invalid_path));
    
    // Verify editor state is unchanged
    ASSERT_EQ(editor.getBuffer().lineCount(), 1);
    EXPECT_EQ(editor.getBuffer().getLine(0), "");
    EXPECT_EQ(editor.getFilename(), "untitled.txt"); // Default filename
}

// Test saving to a new file
TEST_F(EditorFileIOTest, SaveToNewFile) {
    // Set up editor with some content
    editor.typeText("Save test line 1");
    editor.newLine();
    editor.typeText("Save test line 2");
    
    std::string new_file = test_dir + "new_save_file.txt";
    
    // Save to a new file
    ASSERT_TRUE(editor.saveFileAs(new_file));
    
    // Verify file was saved
    std::ifstream file(new_file);
    ASSERT_TRUE(file.is_open());
    
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0], "Save test line 1");
    EXPECT_EQ(lines[1], "Save test line 2");
    
    // Clean up
    std::remove(new_file.c_str());
}

// Test saving with no filename specified
TEST_F(EditorFileIOTest, SaveWithNoFilename) {
    // Default editor with "untitled.txt" as filename
    ASSERT_FALSE(editor.saveFileAs(""));
    
    // Should not create an empty-named file
    std::ifstream file("");
    EXPECT_FALSE(file.is_open());
}

// Test saving to a read-only file
TEST_F(EditorFileIOTest, SaveToReadOnlyFile) {
    editor.typeText("This should not overwrite the read-only file");
    
    // Try to save to the read-only file
    ASSERT_FALSE(editor.saveFileAs(read_only_file));
    
    // Verify the original content is unchanged
    std::ifstream file(read_only_file);
    ASSERT_TRUE(file.is_open());
    
    std::string line;
    std::getline(file, line);
    EXPECT_EQ(line, "Read only content");
}

// Test opening and then saving a file
TEST_F(EditorFileIOTest, OpenAndSaveFile) {
    // Open the test file
    ASSERT_TRUE(editor.openFile(test_file));
    
    // Modify its content
    editor.setCursor(1, 0); // Move to line 2
    editor.typeText("Modified ");
    
    // Save to the same file
    ASSERT_TRUE(editor.saveFile());
    
    // Verify the file was updated correctly
    std::ifstream file(test_file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "Line 1");
    EXPECT_EQ(lines[1], "Modified Line 2");
    EXPECT_EQ(lines[2], "Line 3");
}

// Test saving preserves proper line endings
TEST_F(EditorFileIOTest, SavePreservesLineEndings) {
    // Set up editor with some content
    editor.typeText("Line with");
    editor.newLine();
    editor.typeText("proper");
    editor.newLine();
    editor.typeText("line endings");
    
    std::string new_file = test_dir + "line_endings_test.txt";
    
    // Save to a new file
    ASSERT_TRUE(editor.saveFileAs(new_file));
    
    // Read the file in binary mode to check line endings
    std::ifstream file(new_file, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    
    std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
    std::string content(buffer.begin(), buffer.end());
    
    // Check for proper line endings (platform dependent)
    #ifdef _WIN32
    // Windows uses CRLF
    EXPECT_NE(content.find("Line with\r\nproper\r\nline endings"), std::string::npos);
    #else
    // Unix uses LF
    EXPECT_NE(content.find("Line with\nproper\nline endings"), std::string::npos);
    #endif
    
    // Clean up
    std::remove(new_file.c_str());
}

// Test opening/saving a large file doesn't cause issues
TEST_F(EditorFileIOTest, LargeFileHandling) {
    // Create a larger temporary file (e.g., 100KB)
    std::string large_file = test_dir + "large_file.txt";
    std::ofstream large_out(large_file);
    const int LINE_COUNT = 1000;
    const std::string LINE_CONTENT = "This is a test line with some content to make it reasonably sized for testing performance.";
    
    for (int i = 0; i < LINE_COUNT; i++) {
        large_out << LINE_CONTENT << " (Line " << i << ")\n";
    }
    large_out.close();
    
    // Test opening
    ASSERT_TRUE(editor.openFile(large_file));
    EXPECT_EQ(editor.getBuffer().lineCount(), LINE_COUNT);
    
    // Modify a few lines
    editor.setCursor(50, 0);
    editor.typeText("Modified: ");
    editor.setCursor(100, 0);
    editor.typeText("Also changed: ");
    
    // Save to a new file
    std::string modified_large_file = test_dir + "modified_large_file.txt";
    ASSERT_TRUE(editor.saveFileAs(modified_large_file));
    
    // Verify the saved file
    std::ifstream verify_file(modified_large_file);
    std::string line;
    int line_count = 0;
    bool line50_correct = false;
    bool line100_correct = false;
    
    while (std::getline(verify_file, line)) {
        line_count++;
        if (line_count == 51) { // 1-indexed in the file
            line50_correct = (line.find("Modified: ") == 0);
        } else if (line_count == 101) {
            line100_correct = (line.find("Also changed: ") == 0);
        }
    }
    
    EXPECT_EQ(line_count, LINE_COUNT);
    EXPECT_TRUE(line50_correct);
    EXPECT_TRUE(line100_correct);
    
    // Clean up
    std::remove(large_file.c_str());
    std::remove(modified_large_file.c_str());
} 