#include "../src/TextBuffer.h"
#include "gtest/gtest.h"
#include <string>
#include <stdexcept>

class TextBufferTest : public ::testing::Test {
protected:
    TextBuffer buffer;
    
    void SetUp() override {
        buffer.clear(true);
    }
};

TEST_F(TextBufferTest, Initialization) {
    EXPECT_GE(buffer.lineCount(), 1) << "Buffer should start with at least one line";
}

TEST_F(TextBufferTest, InsertString) {
    buffer.setLine(0, "Hello");
    
    // Insert at beginning
    buffer.insertString(0, 0, "Start-");
    EXPECT_EQ("Start-Hello", buffer.getLine(0)) << "Insert at beginning";
    
    // Insert at middle
    buffer.insertString(0, 6, ", ");
    EXPECT_EQ("Start-, Hello", buffer.getLine(0)) << "Insert in middle";
    
    // Insert at end
    buffer.insertString(0, buffer.getLine(0).length(), " End");
    EXPECT_EQ("Start-, Hello End", buffer.getLine(0)) << "Insert at end";
}

// Test for the expected exception when inserting beyond the end of a line
TEST_F(TextBufferTest, InsertStringBeyondEnd) {
    buffer.setLine(0, "Test");
    EXPECT_THROW(buffer.insertString(0, 100, "!"), std::exception) << "Insert beyond end should throw";
}

TEST_F(TextBufferTest, DeleteChar) {
    buffer.setLine(0, "Hello");
    
    // Delete within line
    buffer.deleteChar(0, 2); // Delete 'l' (position at index 1)
    EXPECT_EQ("Hllo", buffer.getLine(0)) << "Delete within line";
    
    // Delete at beginning of first line (no effect)
    buffer.deleteChar(0, 0);
    EXPECT_EQ("Hllo", buffer.getLine(0)) << "Delete at beginning of first line";
    
    // Delete beyond end (should delete at end)
    buffer.deleteChar(0, 10);
    EXPECT_EQ("Hll", buffer.getLine(0)) << "Delete beyond end (deletes at end)";
    
    // Test joining lines
    buffer.clear(true);
    buffer.setLine(0, "First");
    buffer.addLine("Second");
    buffer.deleteChar(1, 0); // Delete the newline between lines
    EXPECT_EQ(1, buffer.lineCount()) << "Join lines should reduce line count";
    EXPECT_EQ("FirstSecond", buffer.getLine(0)) << "Join lines with backspace";
}

TEST_F(TextBufferTest, DeleteCharForward) {
    buffer.setLine(0, "Hello");
    
    // Delete within line
    buffer.deleteCharForward(0, 2); // Delete 'l' at position 2
    EXPECT_EQ("Helo", buffer.getLine(0)) << "Delete forward within line";
    
    // Delete at end of last line (no effect)
    buffer.deleteCharForward(0, buffer.getLine(0).length());
    EXPECT_EQ("Helo", buffer.getLine(0)) << "Delete forward at end of last line";
    
    // Delete beyond end (should have no effect on last line)
    buffer.deleteCharForward(0, 10);
    EXPECT_EQ("Helo", buffer.getLine(0)) << "Delete forward beyond end of last line";
    
    // Test joining lines
    buffer.clear(true);
    buffer.setLine(0, "First");
    buffer.addLine("Second");
    buffer.deleteCharForward(0, buffer.getLine(0).length()); // Delete at end of first line
    EXPECT_EQ(1, buffer.lineCount()) << "Join lines should reduce line count";
    EXPECT_EQ("FirstSecond", buffer.getLine(0)) << "Join lines with delete forward";
}

TEST_F(TextBufferTest, DeleteLine) {
    // Test trying to delete the only line doesn't throw (current behavior)
    buffer.clear(true);
    size_t initialCount = buffer.lineCount();
    buffer.deleteLine(0);
    EXPECT_GE(buffer.lineCount(), initialCount) << "Buffer should maintain at least one line";
    
    // Test deleting a line among multiple
    buffer.clear(true);
    buffer.setLine(0, "Line 0");
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    
    buffer.deleteLine(1);
    EXPECT_EQ(2, buffer.lineCount()) << "Buffer should have 2 lines after deletion";
    EXPECT_EQ("Line 0", buffer.getLine(0)) << "First line should remain unchanged";
    EXPECT_EQ("Line 2", buffer.getLine(1)) << "Third line should become second line";
} 