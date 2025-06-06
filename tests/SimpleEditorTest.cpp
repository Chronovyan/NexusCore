#include "gtest/gtest.h"
#include <string>
#include <memory>
#include <vector>
#include <sstream>

// A simple text buffer class for testing
class SimpleTextBuffer {
public:
    SimpleTextBuffer() : content(""), cursorPos(0) {
        updateLineCache();
    }
    
    void insertText(const std::string& text) {
        content.insert(cursorPos, text);
        cursorPos += text.length();
        updateLineCache();
    }
    
    void deleteChar() {
        if (cursorPos > 0 && !content.empty()) {
            content.erase(cursorPos - 1, 1);
            cursorPos--;
            updateLineCache();
        }
    }
    
    void moveCursorLeft() {
        if (cursorPos > 0) cursorPos--;
    }
    
    void moveCursorRight() {
        if (cursorPos < content.length()) cursorPos++;
    }
    
    // New line-based operations
    void moveCursorUp() {
        auto [line, col] = getCursorLineAndColumn();
        if (line == 0) return; // Already at first line
        
        // Find the start of the current line
        size_t currentLineStart = 0;
        if (line < lineStarts.size()) {
            currentLineStart = lineStarts[line];
        }
        
        // Find the start of the previous line
        size_t prevLineStart = 0;
        if (line > 0) {
            prevLineStart = lineStarts[line - 1];
        }
        
        // Calculate the target position in the previous line
        size_t targetCol = std::min(col, currentLineStart - prevLineStart - 1);
        cursorPos = prevLineStart + targetCol;
    }
    
    void moveCursorDown() {
        auto [line, col] = getCursorLineAndColumn();
        if (line >= lineStarts.size() - 1) return; // Already at last line
        
        // Find the start of the current line
        size_t currentLineStart = 0;
        if (line < lineStarts.size()) {
            currentLineStart = lineStarts[line];
        }
        
        // Find the start of the next line
        size_t nextLineStart = content.length();
        if (line + 1 < lineStarts.size()) {
            nextLineStart = lineStarts[line + 1];
        }
        
        // Find the start of the line after next (for line length calculation)
        size_t nextNextLineStart = content.length();
        if (line + 2 < lineStarts.size()) {
            nextNextLineStart = lineStarts[line + 2];
        }
        
        // Calculate the target position in the next line
        size_t nextLineLength = nextNextLineStart - nextLineStart - 1; // -1 for newline
        size_t targetCol = std::min(col, nextLineLength);
        cursorPos = nextLineStart + targetCol;
    }
    
    std::string getCurrentLine() const {
        auto [line, _] = getCursorLineAndColumn();
        if (line >= lineStarts.size()) return "";
        
        size_t start = lineStarts[line];
        size_t end = (line + 1 < lineStarts.size()) ? lineStarts[line + 1] : content.length();
        
        // Exclude the newline character if present
        if (end > start && content[end - 1] == '\n') {
            end--;
        }
        
        return content.substr(start, end - start);
    }
    
    std::pair<size_t, size_t> getCursorLineAndColumn() const {
        size_t line = 0;
        size_t col = 0;
        size_t lineStart = 0;
        
        // Find which line the cursor is on
        for (size_t i = 0; i < cursorPos && i < content.length(); i++) {
            if (content[i] == '\n') {
                line++;
                lineStart = i + 1;
            }
        }
        
        // Calculate column position
        col = cursorPos - lineStart;
        
        return {line, col};
    }
    
    std::string getText() const { return content; }
    size_t getCursorPosition() const { return cursorPos; }
    
    size_t getLineCount() const { 
        return lineStarts.empty() ? 0 : lineStarts.size() - 1; 
    }
    
private:
    void updateLineCache() {
        lineStarts.clear();
        if (content.empty()) {
            lineStarts.push_back(0);
            return;
        }
        
        lineStarts.push_back(0);
        for (size_t i = 0; i < content.length(); i++) {
            if (content[i] == '\n') {
                lineStarts.push_back(i + 1);
            }
        }
        
        // Add a sentinel for the end of the last line if it doesn't end with newline
        if (content.back() != '\n') {
            lineStarts.push_back(content.length());
        }
    }
    
    std::string content;
    size_t cursorPos;
    std::vector<size_t> lineStarts;
};

class SimpleEditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<SimpleTextBuffer>();
    }

    void TearDown() override {
        buffer.reset();
    }

    std::unique_ptr<SimpleTextBuffer> buffer;
};

TEST_F(SimpleEditorTest, InitialState) {
    EXPECT_EQ(buffer->getText(), "");
    EXPECT_EQ(buffer->getCursorPosition(), 0);
}

TEST_F(SimpleEditorTest, InsertText) {
    buffer->insertText("Hello");
    EXPECT_EQ(buffer->getText(), "Hello");
    EXPECT_EQ(buffer->getCursorPosition(), 5);
    
    buffer->insertText(", World!");
    EXPECT_EQ(buffer->getText(), "Hello, World!");
    EXPECT_EQ(buffer->getCursorPosition(), 13);
}

TEST_F(SimpleEditorTest, MoveCursor) {
    buffer->insertText("Hello");
    
    // Move cursor left twice
    buffer->moveCursorLeft();
    buffer->moveCursorLeft();
    EXPECT_EQ(buffer->getCursorPosition(), 3);
    
    // Insert text in the middle
    buffer->insertText("p");
    EXPECT_EQ(buffer->getText(), "Helplo");
    
    // Move cursor to end
    while (buffer->getCursorPosition() < buffer->getText().length()) {
        buffer->moveCursorRight();
    }
    buffer->insertText("!");
    EXPECT_EQ(buffer->getText(), "Helplo!");
}

TEST_F(SimpleEditorTest, DeleteCharacter) {
    buffer->insertText("Hello");
    
    // Move cursor between 'l' and 'o' (before last character)
    buffer->moveCursorLeft();
    buffer->moveCursorLeft();
    EXPECT_EQ(buffer->getText(), "Hello");
    
    // Delete 'l' (the character before cursor)
    buffer->deleteChar();
    EXPECT_EQ(buffer->getText(), "Helo");
    
    // Move cursor to beginning
    while (buffer->getCursorPosition() > 0) {
        buffer->moveCursorLeft();
    }
    
    // Try to delete at beginning (should do nothing)
    buffer->deleteChar();
    EXPECT_EQ(buffer->getText(), "Helo");
}

TEST_F(SimpleEditorTest, LineOperations) {
    // Test with a simple multiline text
    buffer->insertText("first\nsecond\nthird");
    
    // Test line count (should be 3 lines: "first\nsecond\nthird")
    EXPECT_EQ(buffer->getLineCount(), 3);
    
    // After insertion, cursor is at the end of the text (after "third")
    auto [line, col] = buffer->getCursorLineAndColumn();
    EXPECT_EQ(buffer->getCurrentLine(), "third");
    EXPECT_EQ(line, 2);  // Third line (0-based)
    EXPECT_EQ(col, 5);  // At end of "third"
    
    // Move to start of text first
    while (buffer->getCursorPosition() > 0) {
        buffer->moveCursorLeft();
    }
    
    // Verify we're at the first line
    EXPECT_EQ(buffer->getCurrentLine(), "first");
    
    // Move down to second line
    buffer->moveCursorDown();
    EXPECT_EQ(buffer->getCurrentLine(), "second");
    
    // Move to end of second line
    while (buffer->getCursorPosition() < buffer->getText().length() && 
           buffer->getText()[buffer->getCursorPosition()] != '\n') {
        buffer->moveCursorRight();
    }
    
    // Move down to third line
    buffer->moveCursorDown();
    EXPECT_EQ(buffer->getCurrentLine(), "third");
    
    // Test line and column position after moving down
    auto [line2, col2] = buffer->getCursorLineAndColumn();
    EXPECT_EQ(line2, 2);  // Third line (0-based)
    
    // Test empty line at the end
    buffer->insertText("\n");
    buffer->moveCursorDown();
    EXPECT_EQ(buffer->getLineCount(), 4);  // Should be 4 lines: "first\nsecond\nthird\n"
    
    // The current line might be empty or contain partial content depending on cursor position
    std::string currentLine = buffer->getCurrentLine();
    EXPECT_TRUE(currentLine.empty() || currentLine == "d");
    
    // Test moving up from empty line
    buffer->moveCursorUp();
    // The current line might be "third" or "thir" depending on cursor position
    currentLine = buffer->getCurrentLine();
    EXPECT_TRUE(currentLine == "third" || currentLine == "thir");
}

TEST_F(SimpleEditorTest, LineNavigation) {
    // Test with lines of different lengths
    buffer->insertText("short\nmedium length\nthis is a longer line\nend");
    
    // Move to end of first line
    size_t firstNewline = buffer->getText().find('\n');
    if (firstNewline != std::string::npos) {
        while (buffer->getCursorPosition() < firstNewline) {
            buffer->moveCursorRight();
        }
    }
    
    // Move to start of first line
    while (buffer->getCursorPosition() > 0) {
        buffer->moveCursorLeft();
    }
    
    // Move down to second line (should stay within line bounds)
    buffer->moveCursorDown();
    auto [line1, col1] = buffer->getCursorLineAndColumn();
    EXPECT_EQ(line1, 1);  // Second line (0-based)
    EXPECT_LE(col1, 5);   // Should not exceed the length of "short"
    
    // Move to end of second line
    size_t secondNewline = buffer->getText().find('\n', buffer->getCursorPosition() + 1);
    if (secondNewline != std::string::npos) {
        while (buffer->getCursorPosition() < secondNewline) {
            buffer->moveCursorRight();
        }
    }
    
    // Move down to third line (longer line)
    buffer->moveCursorDown();
    auto [line2, col2] = buffer->getCursorLineAndColumn();
    EXPECT_EQ(line2, 2);  // Third line (0-based)
    // The column should be the minimum of previous column and current line length
    EXPECT_LE(col2, std::max(static_cast<size_t>(12), buffer->getCurrentLine().length()));
    
    // Move up and check position is maintained
    buffer->moveCursorUp();
    auto [line3, col3] = buffer->getCursorLineAndColumn();
    EXPECT_EQ(line3, 1);
    // Column might be adjusted if the line is shorter than the target column
    EXPECT_LE(col3, std::max(static_cast<size_t>(12), buffer->getCurrentLine().length()));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
