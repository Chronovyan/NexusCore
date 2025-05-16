#include "gtest/gtest.h"
#include "Editor.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>

class EditorApiExtensionsTest : public ::testing::Test {
protected:
    Editor editor;
    const std::string test_dir = "tests/data/editor_api/";
    const std::string test_file = test_dir + "test_file.txt";
    const std::string test_file_cpp = test_dir + "test_file.cpp";

    void SetUp() override {
        // Create test directory if it doesn't exist
        std::filesystem::create_directories(test_dir);

        // Create a test file with some content
        std::ofstream file(test_file);
        file << "Line 1\n";
        file << "Line 2\n";
        file << "Line 3\n";
        file.close();

        // Create a test C++ file with some content
        std::ofstream file_cpp(test_file_cpp);
        file_cpp << "#include <iostream>\n";
        file_cpp << "int main() {\n";
        file_cpp << "    std::cout << \"Hello, World!\" << std::endl;\n";
        file_cpp << "    return 0;\n";
        file_cpp << "}\n";
        file_cpp.close();
    }

    void TearDown() override {
        // Remove test files
        std::remove(test_file.c_str());
        std::remove(test_file_cpp.c_str());

        // Try to remove the test directory and its contents
        std::filesystem::remove_all(test_dir);
    }
};

// Test getFileExtension method
TEST_F(EditorApiExtensionsTest, GetFileExtension) {
    // Default filename "untitled.txt"
    EXPECT_EQ(editor.getFileExtension(), "txt");

    // Set filename to a C++ file
    editor.setFilename(test_file_cpp);
    EXPECT_EQ(editor.getFileExtension(), "cpp");

    // Set filename to a file without extension
    editor.setFilename("filename_without_extension");
    EXPECT_EQ(editor.getFileExtension(), "");

    // Set filename to a hidden file (starting with .)
    editor.setFilename(".hidden_file");
    EXPECT_EQ(editor.getFileExtension(), "");

    // Set filename with multiple dots
    editor.setFilename("file.name.with.multiple.dots.txt");
    EXPECT_EQ(editor.getFileExtension(), "txt");

    // Set filename with just a dot at the end
    editor.setFilename("file_with_dot_at_end.");
    EXPECT_EQ(editor.getFileExtension(), "");
}

// Test isNewFile method
TEST_F(EditorApiExtensionsTest, IsNewFile) {
    // Default state should be a new file (untitled.txt and not modified)
    EXPECT_TRUE(editor.isNewFile());

    // Modified file but still untitled is not a new file
    editor.typeText("Some text");
    EXPECT_FALSE(editor.isNewFile());

    // Reset to empty and unmodified
    editor.getBuffer().clear(true);
    editor.setModified(false);
    editor.setFilename("untitled.txt");
    EXPECT_TRUE(editor.isNewFile());

    // Named file is not a new file, even if unmodified
    editor.setFilename(test_file);
    EXPECT_FALSE(editor.isNewFile());
}

// Test getCurrentLineText method
TEST_F(EditorApiExtensionsTest, GetCurrentLineText) {
    // Default state with empty buffer
    EXPECT_EQ(editor.getCurrentLineText(), "");

    // Add some text to the current line
    editor.typeText("This is line 1");
    EXPECT_EQ(editor.getCurrentLineText(), "This is line 1");

    // Add multiple lines and move cursor to a different line
    editor.newLine();
    editor.typeText("This is line 2");
    editor.newLine();
    editor.typeText("This is line 3");
    
    editor.setCursor(1, 0);
    EXPECT_EQ(editor.getCurrentLineText(), "This is line 2");

    // Move cursor to the last line
    editor.setCursor(2, 0);
    EXPECT_EQ(editor.getCurrentLineText(), "This is line 3");
}

// Test cursor position querying methods
TEST_F(EditorApiExtensionsTest, CursorPositionQuery) {
    // Open a test file to have some content
    editor.openFile(test_file);
    
    // Test cursor at start of line
    editor.setCursor(1, 0);
    EXPECT_TRUE(editor.isCursorAtLineStart());
    EXPECT_FALSE(editor.isCursorAtLineEnd());
    EXPECT_FALSE(editor.isCursorAtBufferStart());
    EXPECT_FALSE(editor.isCursorAtBufferEnd());
    
    // Test cursor at end of line
    const auto line1Length = editor.getBuffer().getLine(1).length();
    editor.setCursor(1, line1Length);
    EXPECT_FALSE(editor.isCursorAtLineStart());
    EXPECT_TRUE(editor.isCursorAtLineEnd());
    EXPECT_FALSE(editor.isCursorAtBufferStart());
    EXPECT_FALSE(editor.isCursorAtBufferEnd());
    
    // Test cursor at start of buffer
    editor.setCursor(0, 0);
    EXPECT_TRUE(editor.isCursorAtLineStart());
    EXPECT_FALSE(editor.isCursorAtLineEnd());
    EXPECT_TRUE(editor.isCursorAtBufferStart());
    EXPECT_FALSE(editor.isCursorAtBufferEnd());
    
    // Test cursor at end of buffer
    const auto lastLine = editor.getBuffer().lineCount() - 1;
    const auto lastLineLength = editor.getBuffer().getLine(lastLine).length();
    editor.setCursor(lastLine, lastLineLength);
    EXPECT_FALSE(editor.isCursorAtLineStart());
    EXPECT_TRUE(editor.isCursorAtLineEnd());
    EXPECT_FALSE(editor.isCursorAtBufferStart());
    EXPECT_TRUE(editor.isCursorAtBufferEnd());
    
    // Test middle of a line
    editor.setCursor(1, 2);
    EXPECT_FALSE(editor.isCursorAtLineStart());
    EXPECT_FALSE(editor.isCursorAtLineEnd());
    EXPECT_FALSE(editor.isCursorAtBufferStart());
    EXPECT_FALSE(editor.isCursorAtBufferEnd());
}

// Test edge cases for cursor position methods
TEST_F(EditorApiExtensionsTest, CursorPositionEdgeCases) {
    // Empty buffer case
    Editor emptyEditor;
    EXPECT_TRUE(emptyEditor.isCursorAtLineStart());
    EXPECT_TRUE(emptyEditor.isCursorAtLineEnd());
    EXPECT_TRUE(emptyEditor.isCursorAtBufferStart());
    EXPECT_TRUE(emptyEditor.isCursorAtBufferEnd());
    
    // Single line buffer
    editor.getBuffer().clear(true);
    editor.typeText("Single line");
    
    // At start of the single line
    editor.setCursor(0, 0);
    EXPECT_TRUE(editor.isCursorAtLineStart());
    EXPECT_FALSE(editor.isCursorAtLineEnd());
    EXPECT_TRUE(editor.isCursorAtBufferStart());
    EXPECT_FALSE(editor.isCursorAtBufferEnd());
    
    // At end of the single line
    const auto lineLength = editor.getBuffer().getLine(0).length();
    editor.setCursor(0, lineLength);
    EXPECT_FALSE(editor.isCursorAtLineStart());
    EXPECT_TRUE(editor.isCursorAtLineEnd());
    EXPECT_FALSE(editor.isCursorAtBufferStart());
    EXPECT_TRUE(editor.isCursorAtBufferEnd());
    
    // Single empty line
    editor.getBuffer().clear(true);
    
    // On an empty line
    editor.setCursor(0, 0);
    EXPECT_TRUE(editor.isCursorAtLineStart());
    EXPECT_TRUE(editor.isCursorAtLineEnd()); // Empty line, so start is also end
    EXPECT_TRUE(editor.isCursorAtBufferStart());
    EXPECT_TRUE(editor.isCursorAtBufferEnd());
}

// Test viewport methods
TEST_F(EditorApiExtensionsTest, ViewportMethods) {
    // Test the initial values of the viewport methods
    EXPECT_EQ(editor.getViewportStartLine(), 0); // Expecting default value
    EXPECT_GT(editor.getViewportHeight(), 0);    // Should be a positive value
    
    // For now, we're just testing that the methods return the expected initial values
    // In the future, tests could verify the viewport updates as expected when scrolling
}

// Test getWordUnderCursor method
TEST_F(EditorApiExtensionsTest, GetWordUnderCursor) {
    // Setup a test line with various word types
    if (editor.getBuffer().isEmpty()) {
        editor.getBuffer().addLine("");
    }
    editor.getBuffer().replaceLine(0, "word1 another_word  123 symbol@special end");
    
    // Case 1: Cursor in the middle of a word
    editor.setCursor(0, 2); // 'r' in "word1"
    EXPECT_EQ(editor.getWordUnderCursor(), "word1");
    
    // Case 2: Cursor at the beginning of a word
    editor.setCursor(0, 6); // 'a' in "another_word"
    EXPECT_EQ(editor.getWordUnderCursor(), "another_word");
    
    // Case 3: Cursor at the end of a word
    editor.setCursor(0, 5); // Just after "word1"
    EXPECT_EQ(editor.getWordUnderCursor(), "word1");
    
    // Case 4: Cursor on underscore (part of word)
    editor.setCursor(0, 13); // '_' in "another_word"
    EXPECT_EQ(editor.getWordUnderCursor(), "another_word");
    
    // Case 5: Cursor on whitespace between words
    editor.setCursor(0, 19); // Space after "another_word"
    EXPECT_EQ(editor.getWordUnderCursor(), "");
    
    // Case 6: Cursor on a number (which is considered part of a word)
    editor.setCursor(0, 21); // '2' in "123"
    EXPECT_EQ(editor.getWordUnderCursor(), "123");
    
    // Case 7: Cursor on a special character
    // Find the exact position of '@' in the string
    std::string currentLine = editor.getBuffer().getLine(0);
    size_t atPos = currentLine.find('@');
    ASSERT_NE(atPos, std::string::npos) << "@ character not found in test string";
    editor.setCursor(0, atPos); // '@' in "symbol@special"
    
    // When cursor is on a non-alphanumeric character that immediately follows a word,
    // our implementation will return that word
    EXPECT_EQ(editor.getWordUnderCursor(), "symbol");
    
    // Case 8: Cursor at the very end of the line
    editor.setCursor(0, editor.getBuffer().getLine(0).length());
    EXPECT_EQ(editor.getWordUnderCursor(), "end");
    
    // Case 9: Empty line
    editor.getBuffer().clear(true);
    if (editor.getBuffer().isEmpty()) {
        editor.getBuffer().addLine("");
    } else {
        editor.getBuffer().replaceLine(0, "");
    }
    editor.setCursor(0, 0);
    EXPECT_EQ(editor.getWordUnderCursor(), "");
    
    // Case 10: Single-letter word
    // Ensure buffer has at least one line
    if (editor.getBuffer().isEmpty()) {
        editor.getBuffer().addLine("");
    }
    editor.getBuffer().replaceLine(0, "a b c");
    editor.setCursor(0, 0); // On 'a'
    EXPECT_EQ(editor.getWordUnderCursor(), "a");
    
    // Case 11: Word containing numbers
    // Ensure buffer has at least one line
    if (editor.getBuffer().isEmpty()) {
        editor.getBuffer().addLine("");
    }
    editor.getBuffer().replaceLine(0, "test123 next");
    editor.setCursor(0, 4); // 't' in "test123"
    EXPECT_EQ(editor.getWordUnderCursor(), "test123");
} 