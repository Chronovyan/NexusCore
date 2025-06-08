#include "gtest/gtest.h"
#include "../src/TextBuffer.h"
#include <string>
#include <memory>
#include <vector>

class TextBufferLineOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<TextBuffer>();
        // Start with some initial text
        buffer->addLine("line1");
        buffer->addLine("line2");
        buffer->addLine("line3");
    }
    
    // Helper function to get text content as a single string
    std::string getBufferContent() {
        std::string content;
        for (size_t i = 0; i < buffer->lineCount(); ++i) {
            if (i > 0) content += "\n";
            content += buffer->getLine(i);
        }
        return content;
    }
    
    void TearDown() override {
        buffer.reset();
    }
    
    std::unique_ptr<TextBuffer> buffer;
};

TEST_F(TextBufferLineOperationsTest, GetLine) {
    EXPECT_EQ(buffer->getLine(0), "line1");
    EXPECT_EQ(buffer->getLine(1), "line2");
    EXPECT_EQ(buffer->getLine(2), "line3");
    
    // Test out of bounds - should throw
    EXPECT_THROW(buffer->getLine(3), std::out_of_range);
}

TEST_F(TextBufferLineOperationsTest, GetLineCount) {
    EXPECT_EQ(buffer->getLineCount(), 3);
    
    // Add another line
    buffer->insertText("\nline4");
    EXPECT_EQ(buffer->getLineCount(), 4);
    
    // Clear the buffer
    buffer->clear(true); // Keep one empty line
    EXPECT_EQ(buffer->getLineCount(), 1); // Empty buffer has one empty line
}

TEST_F(TextBufferLineOperationsTest, InsertNewline) {
    // Insert newline in the middle of first line
    buffer->splitLine(0, 2);
    
    EXPECT_EQ(buffer->getLine(0), "li");
    EXPECT_EQ(buffer->getLine(1), "ne1");
    EXPECT_EQ(buffer->getLine(2), "line2");
    EXPECT_EQ(buffer->getLine(3), "line3");
    
    // Test undo is not directly supported in TextBuffer
    // We'll need to implement this functionality separately
}

TEST_F(TextBufferLineOperationsTest, DeleteLine) {
    // Delete middle line
    buffer->deleteLine(1);
    
    EXPECT_EQ(buffer->getLine(0), "line1");
    EXPECT_EQ(buffer->getLine(1), "line3");
    EXPECT_EQ(buffer->lineCount(), 2);
    
    // Delete first line
    buffer->deleteLine(0);
    EXPECT_EQ(buffer->getLine(0), "line3");
    EXPECT_EQ(buffer->lineCount(), 1);
    
    // Delete last line (should make it empty)
    buffer->deleteLine(0);
    EXPECT_EQ(buffer->lineCount(), 1); // TextBuffer keeps at least one empty line
    EXPECT_TRUE(buffer->getLine(0).empty());
}

TEST_F(TextBufferLineOperationsTest, JoinLines) {
    // Join first and second lines
    buffer->joinLines(0);
    
    EXPECT_EQ(buffer->getLine(0), "line1line2");
    EXPECT_EQ(buffer->getLine(1), "line3");
    EXPECT_EQ(buffer->lineCount(), 2);
    
    // Join with the last line
    buffer->joinLines(0);
    EXPECT_EQ(buffer->getLine(0), "line1line2line3");
    EXPECT_EQ(buffer->lineCount(), 1);
    
    // Join when there's only one line (should do nothing)
    buffer->joinLines(0);
    EXPECT_EQ(buffer->lineCount(), 1);
}

TEST_F(TextBufferLineOperationsTest, LineLength) {
    EXPECT_EQ(buffer->lineLength(0), 5); // "line1"
    EXPECT_EQ(buffer->lineLength(1), 5); // "line2"
    
    // Test out of bounds - should throw
    EXPECT_THROW(buffer->lineLength(10), std::out_of_range);
}

TEST_F(TextBufferLineOperationsTest, InsertText) {
    // Insert text in the middle of first line
    buffer->insertString(0, 2, "XXX");
    EXPECT_EQ(buffer->getLine(0), "liXXXne1");
    
    // Insert text at the end of line
    buffer->insertString(0, buffer->lineLength(0), "ZZZ");
    EXPECT_EQ(buffer->getLine(0), "liXXXne1ZZZ");
    
    // Insert text with newlines
    buffer->insertString(0, 2, "A\nB\nC");
    EXPECT_EQ(buffer->getLine(0), "liA");
    EXPECT_EQ(buffer->getLine(1), "B");
    EXPECT_EQ(buffer->getLine(2), "CXXXne1ZZZ");
    EXPECT_EQ(buffer->lineCount(), 5); // Original 3 + 2 new lines - 1 merged
}

TEST_F(TextBufferLineOperationsTest, SplitLine) {
    // Split first line after 'n'
    buffer->splitLine(0, 3);
    
    EXPECT_EQ(buffer->getLine(0), "lin");
    EXPECT_EQ(buffer->getLine(1), "e1");
    EXPECT_EQ(buffer->getLine(2), "line2");
    EXPECT_EQ(buffer->getLine(3), "line3");
    EXPECT_EQ(buffer->lineCount(), 4);
}

TEST_F(TextBufferLineOperationsTest, ComplexOperations) {
    // Add a line
    buffer->addLine("line4");
    EXPECT_EQ(buffer->lineCount(), 4);
    
    // Insert text in the middle of line2
    buffer->insertString(1, 2, "XXX");
    EXPECT_EQ(buffer->getLine(1), "liXXXne2");
    
    // Split line2 after 'X'
    buffer->splitLine(1, 4);
    EXPECT_EQ(buffer->getLine(1), "liXX");
    EXPECT_EQ(buffer->getLine(2), "Xne2");
    EXPECT_EQ(buffer->lineCount(), 5);
    
    // Join lines 1 and 2
    buffer->joinLines(1);
    EXPECT_EQ(buffer->getLine(1), "liXXXne2");
    EXPECT_EQ(buffer->lineCount(), 4);
    
    // Delete line2
    buffer->deleteLine(1);
    EXPECT_EQ(buffer->getLine(1), "line3");
    EXPECT_EQ(buffer->lineCount(), 3);
}

TEST_F(TextBufferLineOperationsTest, EmptyBuffer) {
    buffer->clear(true); // Clear but keep one empty line
    
    EXPECT_EQ(buffer->lineCount(), 1);
    EXPECT_TRUE(buffer->getLine(0).empty());
    EXPECT_EQ(buffer->lineLength(0), 0);
    
    // Operations on empty buffer
    buffer->addLine(""); // Add empty line
    EXPECT_EQ(buffer->lineCount(), 2);
    EXPECT_TRUE(buffer->getLine(0).empty());
    EXPECT_TRUE(buffer->getLine(1).empty());
    
    buffer->deleteLine(0);
    EXPECT_EQ(buffer->lineCount(), 1);
    
    // Join should do nothing on single line
    buffer->joinLines(0);
    EXPECT_EQ(buffer->lineCount(), 1);
}
