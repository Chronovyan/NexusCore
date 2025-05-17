#include "../src/TextBuffer.h"
#include "gtest/gtest.h"
#include <string>
#include <stdexcept>
#include <vector>

class TextBufferTest : public ::testing::Test {
protected:
    TextBuffer buffer;
    
    void SetUp() override {
        // Ensure the buffer is completely empty before adding test lines
        buffer.clear(false);
        
        // Initialize with some content for testing
        buffer.addLine("First line");
        buffer.addLine("Second line");
        buffer.addLine("Third line with more text");
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
    // Test case 1: Delete within line
    {
        TextBuffer testBuffer;
        testBuffer.clear(false);
        testBuffer.addLine("Hello");
        
        testBuffer.deleteCharForward(0, 2); // Delete 'l' at position 2
        EXPECT_EQ("Helo", testBuffer.getLine(0)) << "Delete forward within line";
    }
    
    // Test case 2: Delete at end of line (no effect)
    {
        TextBuffer testBuffer;
        testBuffer.clear(false);
        testBuffer.addLine("Helo");
        
        testBuffer.deleteCharForward(0, testBuffer.getLine(0).length());
        EXPECT_EQ("Helo", testBuffer.getLine(0)) << "Delete forward at end of last line";
    }
    
    // Test case 3: Delete beyond end - should throw exception
    {
        TextBuffer testBuffer;
        testBuffer.clear(false);
        testBuffer.addLine("Helo");
        
        // Attempting to delete beyond the end should throw an exception
        EXPECT_THROW(testBuffer.deleteCharForward(0, testBuffer.getLine(0).length() + 1), std::exception) 
            << "Delete forward beyond end should throw";
    }
    
    // Test case 4: Join lines
    {
        TextBuffer testBuffer;
        testBuffer.clear(false);
        testBuffer.addLine("First");
        testBuffer.addLine("Second");
        
        testBuffer.deleteCharForward(0, testBuffer.getLine(0).length()); // Delete at end of first line
        EXPECT_EQ(1, testBuffer.lineCount()) << "Join lines should reduce line count";
        EXPECT_EQ("FirstSecond", testBuffer.getLine(0)) << "Join lines with delete forward";
    }
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

// Test lineLength method
TEST_F(TextBufferTest, LineLengthReturnsCorrectLength) {
    EXPECT_EQ(10, buffer.lineLength(0)); // "First line"
    EXPECT_EQ(11, buffer.lineLength(1)); // "Second line"
    EXPECT_EQ(25, buffer.lineLength(2)); // "Third line with more text"
}

TEST_F(TextBufferTest, LineLengthThrowsForInvalidIndex) {
    EXPECT_THROW(buffer.lineLength(3), std::out_of_range);
    EXPECT_THROW(buffer.lineLength(100), std::out_of_range);
}

// Test characterCount method
TEST_F(TextBufferTest, CharacterCountReturnsCorrectTotal) {
    EXPECT_EQ(10 + 11 + 25, buffer.characterCount()); // Sum of all line lengths
    
    // Test with empty buffer
    TextBuffer emptyBuffer;
    EXPECT_EQ(0, emptyBuffer.characterCount());
    
    // Test with empty line
    TextBuffer bufferWithEmptyLine;
    bufferWithEmptyLine.addLine("");
    EXPECT_EQ(0, bufferWithEmptyLine.characterCount());
}

// Test getAllLines method
TEST_F(TextBufferTest, GetAllLinesReturnsAllLines) {
    std::vector<std::string> expectedLines = {
        "First line",
        "Second line",
        "Third line with more text"
    };
    
    EXPECT_EQ(expectedLines, buffer.getAllLines());
}

TEST_F(TextBufferTest, GetAllLinesReturnsEmptyVectorForEmptyBuffer) {
    TextBuffer emptyBuffer;
    EXPECT_TRUE(emptyBuffer.getAllLines().empty());
}

// Test replaceLineSegment method
TEST_F(TextBufferTest, ReplaceLineSegmentReplacesTextCorrectly) {
    // Replace "First" with "New"
    buffer.replaceLineSegment(0, 0, 5, "New");
    EXPECT_EQ("New line", buffer.getLine(0));
    
    // Replace "Second" with "Modified"
    buffer.replaceLineSegment(1, 0, 6, "Modified");
    EXPECT_EQ("Modified line", buffer.getLine(1));
    
    // Replace middle part of line
    buffer.replaceLineSegment(2, 6, 15, "segment");
    EXPECT_EQ("Third segment more text", buffer.getLine(2));
}

TEST_F(TextBufferTest, ReplaceLineSegmentHandlesInvalidRanges) {
    // Test swapping startCol and endCol if startCol > endCol
    buffer.replaceLineSegment(0, 5, 0, "New");
    EXPECT_EQ("New line", buffer.getLine(0));
    
    // Test endCol beyond line length
    buffer.replaceLineSegment(1, 11, 20, " extended");
    EXPECT_EQ("Second line extended", buffer.getLine(1));
    
    // Test startCol beyond line length (should append)
    buffer.replaceLineSegment(2, 30, 35, " appended");
    EXPECT_EQ("Third line with more text appended", buffer.getLine(2));
}

TEST_F(TextBufferTest, ReplaceLineSegmentThrowsForInvalidLineIndex) {
    EXPECT_THROW(buffer.replaceLineSegment(3, 0, 5, "Invalid"), std::out_of_range);
    EXPECT_THROW(buffer.replaceLineSegment(100, 0, 5, "Invalid"), std::out_of_range);
}

// Test deleteLineSegment method
TEST_F(TextBufferTest, DeleteLineSegmentDeletesTextCorrectly) {
    // Delete "First"
    buffer.deleteLineSegment(0, 0, 5);
    EXPECT_EQ(" line", buffer.getLine(0));
    
    // Delete "Second "
    buffer.deleteLineSegment(1, 0, 7);
    EXPECT_EQ("line", buffer.getLine(1));
    
    // Delete middle part of line
    buffer.deleteLineSegment(2, 6, 15);
    EXPECT_EQ("Third  more text", buffer.getLine(2));
}

TEST_F(TextBufferTest, DeleteLineSegmentHandlesInvalidRanges) {
    // Test swapping startCol and endCol if startCol > endCol
    buffer.deleteLineSegment(0, 10, 5);
    EXPECT_EQ("First", buffer.getLine(0));
    
    // Test endCol beyond line length
    buffer.deleteLineSegment(1, 7, 20);
    EXPECT_EQ("Second ", buffer.getLine(1));
    
    // Test startCol beyond line length (should do nothing)
    buffer.deleteLineSegment(2, 30, 35);
    EXPECT_EQ("Third line with more text", buffer.getLine(2));
    
    // Test startCol equals endCol (should do nothing)
    buffer.deleteLineSegment(2, 5, 5);
    EXPECT_EQ("Third line with more text", buffer.getLine(2));
}

TEST_F(TextBufferTest, DeleteLineSegmentThrowsForInvalidLineIndex) {
    EXPECT_THROW(buffer.deleteLineSegment(3, 0, 5), std::out_of_range);
    EXPECT_THROW(buffer.deleteLineSegment(100, 0, 5), std::out_of_range);
}

// Test deleteLines method
TEST_F(TextBufferTest, DeleteLinesRemovesSpecifiedRange) {
    // Delete a range of lines
    buffer.deleteLines(0, 2);
    EXPECT_EQ(1, buffer.lineCount());
    EXPECT_EQ("Third line with more text", buffer.getLine(0));
}

TEST_F(TextBufferTest, DeleteLinesThrowsForInvalidRange) {
    // Invalid startIndex
    EXPECT_THROW(buffer.deleteLines(3, 4), std::out_of_range);
    
    // startIndex >= endIndex
    EXPECT_THROW(buffer.deleteLines(1, 1), std::out_of_range);
    EXPECT_THROW(buffer.deleteLines(2, 1), std::out_of_range);
}

TEST_F(TextBufferTest, DeleteLinesHandlesEdgeCases) {
    // Delete all lines
    buffer.deleteLines(0, 3);
    
    // Buffer should maintain one empty line
    EXPECT_EQ(1, buffer.lineCount());
    EXPECT_EQ("", buffer.getLine(0));
    
    // Prepare buffer for next test
    buffer.clear(false);
    buffer.addLine("Line 0");
    buffer.addLine("Line 1");
    
    // Delete partial range up to the end
    buffer.deleteLines(0, 5); // endIndex is beyond buffer size, should clamp
    EXPECT_EQ(1, buffer.lineCount());
    EXPECT_EQ("", buffer.getLine(0));
}

// Test insertLines method
TEST_F(TextBufferTest, InsertLinesInsertsAtSpecifiedIndex) {
    std::vector<std::string> newLines = {"New line 1", "New line 2"};
    
    // Insert in the middle
    buffer.insertLines(1, newLines);
    EXPECT_EQ(5, buffer.lineCount());
    EXPECT_EQ("First line", buffer.getLine(0));
    EXPECT_EQ("New line 1", buffer.getLine(1));
    EXPECT_EQ("New line 2", buffer.getLine(2));
    EXPECT_EQ("Second line", buffer.getLine(3));
    EXPECT_EQ("Third line with more text", buffer.getLine(4));
}

TEST_F(TextBufferTest, InsertLinesThrowsForInvalidIndex) {
    std::vector<std::string> newLines = {"New line"};
    EXPECT_THROW(buffer.insertLines(4, newLines), std::out_of_range);
}

TEST_F(TextBufferTest, InsertLinesHandlesEdgeCases) {
    std::vector<std::string> newLines = {"New line 1", "New line 2"};
    
    // Insert at beginning
    buffer.insertLines(0, newLines);
    EXPECT_EQ(5, buffer.lineCount());
    EXPECT_EQ("New line 1", buffer.getLine(0));
    EXPECT_EQ("New line 2", buffer.getLine(1));
    
    // Insert at end
    buffer.insertLines(buffer.lineCount(), newLines);
    EXPECT_EQ(7, buffer.lineCount());
    EXPECT_EQ("New line 1", buffer.getLine(5));
    EXPECT_EQ("New line 2", buffer.getLine(6));
    
    // Insert empty vector (no change)
    std::vector<std::string> emptyLines;
    buffer.insertLines(2, emptyLines);
    EXPECT_EQ(7, buffer.lineCount()); // Count should remain the same
    
    // Insert into empty buffer
    TextBuffer emptyBuffer;
    emptyBuffer.clear(false);
    EXPECT_EQ(0, emptyBuffer.lineCount());
    emptyBuffer.insertLines(0, newLines);
    EXPECT_EQ(2, emptyBuffer.lineCount());
    EXPECT_EQ("New line 1", emptyBuffer.getLine(0));
    EXPECT_EQ("New line 2", emptyBuffer.getLine(1));
}

// Test isValidPosition method
TEST_F(TextBufferTest, IsValidPositionIdentifiesValidPositions) {
    // Valid positions
    EXPECT_TRUE(buffer.isValidPosition(0, 0));  // Beginning of first line
    EXPECT_TRUE(buffer.isValidPosition(0, 10)); // End of first line
    EXPECT_TRUE(buffer.isValidPosition(2, 15)); // Middle of third line
    
    // Invalid positions
    EXPECT_FALSE(buffer.isValidPosition(3, 0));   // Line index out of bounds
    EXPECT_FALSE(buffer.isValidPosition(0, 11));  // Column index out of bounds for first line
    EXPECT_FALSE(buffer.isValidPosition(1, 100)); // Column index way out of bounds
    
    // Empty buffer has no valid positions
    TextBuffer emptyBuffer;
    emptyBuffer.clear(false);
    EXPECT_FALSE(emptyBuffer.isValidPosition(0, 0));
}

// Test clampPosition method
TEST_F(TextBufferTest, ClampPositionConstrainsToValidRange) {
    // Clamp line index
    auto clamped1 = buffer.clampPosition(5, 0);
    EXPECT_EQ(2, clamped1.first);   // Clamped to last line
    EXPECT_EQ(0, clamped1.second);  // Column unchanged
    
    // Clamp column index
    auto clamped2 = buffer.clampPosition(0, 20);
    EXPECT_EQ(0, clamped2.first);    // Line unchanged
    EXPECT_EQ(10, clamped2.second);  // Clamped to end of first line
    
    // Clamp both indices
    auto clamped3 = buffer.clampPosition(10, 30);
    EXPECT_EQ(2, clamped3.first);    // Clamped to last line
    EXPECT_EQ(25, clamped3.second);  // Clamped to end of last line
    
    // No clamping needed
    auto clamped4 = buffer.clampPosition(1, 5);
    EXPECT_EQ(1, clamped4.first);
    EXPECT_EQ(5, clamped4.second);
    
    // Empty buffer test
    TextBuffer emptyBuffer;
    emptyBuffer.clear(false);
    auto clamped5 = emptyBuffer.clampPosition(2, 3);
    EXPECT_EQ(0, clamped5.first);
    EXPECT_EQ(0, clamped5.second);
} 