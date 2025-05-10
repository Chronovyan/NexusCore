#include "gtest/gtest.h"
#include "TextBuffer.h" // Assuming this path is correct once include directories are set up
#include "EditorError.h"  // TextBuffer might throw EditorException derivatives
#include <fstream>      // For std::ifstream, std::ofstream
#include <cstdio>       // For std::remove
#include <vector>       // For std::vector

// Test fixture for TextBuffer tests
class TextBufferTest : public ::testing::Test {
protected:
    TextBuffer buffer; // Each test will get a fresh TextBuffer instance

    // You can do common setup here if needed, e.g.:
    // void SetUp() override {
    //    buffer.addLine("Initial line for all tests");
    // }

    // You can do common teardown here if needed:
    // void TearDown() override {
    // }
};

// Test case for isEmpty and initial lineCount
TEST_F(TextBufferTest, IsEmptyInitially) {
    // A default-constructed TextBuffer in our Editor usually adds one empty line.
    // Let's verify based on TextBuffer's default constructor behavior.
    // If TextBuffer() truly creates an empty (0 lines) buffer, this test needs adjustment.
    // Based on current Editor.cpp, TextBuffer is made non-empty right after construction.
    // For a direct TextBuffer test, let's assume its default constructor state.
    // If TextBuffer *itself* adds a line by default, then isEmpty is false.
    // If it's truly empty (0 lines), isEmpty is true.
    
    // Assuming TextBuffer default constructor creates a buffer that is NOT empty
    // (e.g. it adds one empty line by default as seen in some editor logic).
    // This needs to match TextBuffer's actual default constructor.
    // If TextBuffer() creates 0 lines, then:
    // EXPECT_TRUE(buffer.isEmpty());
    // EXPECT_EQ(buffer.lineCount(), 0);

    // If TextBuffer() creates 1 empty line (common pattern):
    EXPECT_FALSE(buffer.isEmpty()); // Should have one empty line
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
}

// Test case for addLine
TEST_F(TextBufferTest, AddLine) {
    // Assuming TextBuffer starts with 1 empty line (line 0)
    buffer.addLine("Hello, world!"); // Adds as line 1
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(1), "Hello, world!");

    buffer.addLine("Another line"); // Adds as line 2
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(2), "Another line");
    EXPECT_EQ(buffer.getLine(0), ""); // Original empty line should be unaffected
}

// Test case for insertLine
TEST_F(TextBufferTest, InsertLine) {
    // Starts with 1 empty line: [""]
    buffer.insertLine(0, "First line"); // Inserts at index 0
    // Buffer should be: ["First line", ""]
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "First line");
    EXPECT_EQ(buffer.getLine(1), "");

    buffer.insertLine(1, "Second line"); // Inserts at index 1
    // Buffer should be: ["First line", "Second line", ""]
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "First line");
    EXPECT_EQ(buffer.getLine(1), "Second line");
    EXPECT_EQ(buffer.getLine(2), "");

    buffer.insertLine(3, "Last line"); // Inserts at end (index == lineCount)
    // Buffer should be: ["First line", "Second line", "", "Last line"]
    EXPECT_EQ(buffer.lineCount(), 4);
    EXPECT_EQ(buffer.getLine(3), "Last line");
}

// Test for clear method
TEST_F(TextBufferTest, ClearBuffer) {
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    ASSERT_EQ(buffer.lineCount(), 3); // Incl. initial empty line + 2 added

    buffer.clear(); // Default clear should leave one empty line
    EXPECT_FALSE(buffer.isEmpty());
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");

    buffer.addLine("Another after clear");
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(1), "Another after clear");

    // Test clear(false) for truly empty if that's supported
    // buffer.clear(false); // If such an overload exists
    // EXPECT_TRUE(buffer.isEmpty());
    // EXPECT_EQ(buffer.lineCount(), 0);
}

// Test case for deleteLine
TEST_F(TextBufferTest, DeleteLineMiddle) {
    // Starts with 1 empty line: [""]
    buffer.addLine("Line 1"); // ["", "Line 1"]
    buffer.addLine("Line 2"); // ["", "Line 1", "Line 2"]
    buffer.addLine("Line 3"); // ["", "Line 1", "Line 2", "Line 3"]
    ASSERT_EQ(buffer.lineCount(), 4);

    buffer.deleteLine(2); // Delete "Line 2" (at index 2)
    // Buffer should be: ["", "Line 1", "Line 3"]
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    EXPECT_EQ(buffer.getLine(2), "Line 3");
}

TEST_F(TextBufferTest, DeleteLineFirst) {
    // Starts with 1 empty line: [""]
    buffer.addLine("Line A"); // ["", "Line A"]
    buffer.addLine("Line B"); // ["", "Line A", "Line B"]
    ASSERT_EQ(buffer.lineCount(), 3);

    buffer.deleteLine(0); // Delete the initial empty line
    // Buffer should be: ["Line A", "Line B"]
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Line A");
    EXPECT_EQ(buffer.getLine(1), "Line B");

    buffer.deleteLine(0); // Delete "Line A"
    // Buffer should be: ["Line B"]
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Line B");
}

TEST_F(TextBufferTest, DeleteLineLast) {
    // Starts with 1 empty line: [""]
    buffer.addLine("Line X"); // ["", "Line X"]
    buffer.addLine("Line Y"); // ["", "Line X", "Line Y"]
    ASSERT_EQ(buffer.lineCount(), 3);

    buffer.deleteLine(2); // Delete "Line Y" (at index 2, which is lineCount - 1)
    // Buffer should be: ["", "Line X"]
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Line X");

    buffer.deleteLine(1); // Delete "Line X"
    // Buffer should be: [""]
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
}

TEST_F(TextBufferTest, DeleteOnlyLine) {
    // Starts with 1 empty line: [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");

    // Deleting the only line should result in the buffer still having one empty line.
    // This behavior ensures the buffer is never truly "empty" in a way that might
    // cause issues for cursor positioning or display logic.
    buffer.deleteLine(0);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_FALSE(buffer.isEmpty()); // It still contains one (empty) line
}

TEST_F(TextBufferTest, DeleteLineOutOfBounds) {
    // Starts with 1 empty line: [""]
    buffer.addLine("Content Line"); // ["", "Content Line"]
    ASSERT_EQ(buffer.lineCount(), 2);

    // Attempt to delete at lineCount (invalid, valid indices are 0 to lineCount-1)
    EXPECT_THROW(buffer.deleteLine(2), TextBufferException);
    // Negative index
    EXPECT_THROW(buffer.deleteLine(-1), TextBufferException);

    // Ensure buffer state is unchanged after throwing
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Content Line");

    // Delete all lines properly to test on an "empty" (1 line) buffer
    buffer.deleteLine(1); // Delete "Content Line" -> [""]
    buffer.deleteLine(0); // Delete "" -> [""] (should behave like DeleteOnlyLine)
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_THROW(buffer.deleteLine(1), TextBufferException); // Still out of bounds
}

// Test cases for replaceLine
TEST_F(TextBufferTest, ReplaceLineMiddle) {
    buffer.addLine("Line 1"); // ["", "Line 1"]
    buffer.addLine("Line 2"); // ["", "Line 1", "Line 2"]
    buffer.addLine("Line 3"); // ["", "Line 1", "Line 2", "Line 3"]
    ASSERT_EQ(buffer.lineCount(), 4);

    buffer.replaceLine(2, "Replacement Line 2");
    EXPECT_EQ(buffer.lineCount(), 4); // Count should not change
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    EXPECT_EQ(buffer.getLine(2), "Replacement Line 2");
    EXPECT_EQ(buffer.getLine(3), "Line 3");
}

TEST_F(TextBufferTest, ReplaceLineFirst) {
    buffer.addLine("Line A"); // ["", "Line A"]
    buffer.addLine("Line B"); // ["", "Line A", "Line B"]
    ASSERT_EQ(buffer.lineCount(), 3);

    buffer.replaceLine(0, "New First Line");
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "New First Line");
    EXPECT_EQ(buffer.getLine(1), "Line A");
    EXPECT_EQ(buffer.getLine(2), "Line B");
}

TEST_F(TextBufferTest, ReplaceLineLast) {
    buffer.addLine("Line X"); // ["", "Line X"]
    buffer.addLine("Line Y"); // ["", "Line X", "Line Y"]
    ASSERT_EQ(buffer.lineCount(), 3);

    buffer.replaceLine(2, "New Last Line");
    EXPECT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Line X");
    EXPECT_EQ(buffer.getLine(2), "New Last Line");
}

TEST_F(TextBufferTest, ReplaceOnlyLine) {
    // Starts with 1 empty line: [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    buffer.replaceLine(0, "The Only Line Replaced");
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "The Only Line Replaced");
}

TEST_F(TextBufferTest, ReplaceLineWithEmptyString) {
    buffer.addLine("Not Empty"); // ["", "Not Empty"]
    ASSERT_EQ(buffer.lineCount(), 2);
    buffer.replaceLine(1, "");
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "");
}

TEST_F(TextBufferTest, ReplaceLineOutOfBounds) {
    buffer.addLine("Line 1"); // ["", "Line 1"]
    ASSERT_EQ(buffer.lineCount(), 2);

    // Out of bounds indices
    EXPECT_THROW(buffer.replaceLine(2, "Too Far"), TextBufferException);
    EXPECT_THROW(buffer.replaceLine(-1, "Negative"), TextBufferException);

    // Ensure buffer is unchanged
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
}

// Test case for getLine (const and non-const versions)
TEST_F(TextBufferTest, GetLineOutOfBounds) {
    // Mutable buffer non-const getLine()
    EXPECT_THROW(buffer.getLine(1), TextBufferException);
    EXPECT_THROW(buffer.getLine(-1), TextBufferException); // Tests large positive due to size_t conversion

    // For const buffer.getLine() const
    const TextBuffer constBuffer;
    EXPECT_THROW(constBuffer.getLine(1), TextBufferException);
    EXPECT_THROW(constBuffer.getLine(-1), TextBufferException); // Though size_t makes -1 a large positive

    // Ensure adding lines changes valid range
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    EXPECT_NO_THROW(buffer.getLine(1));
    EXPECT_THROW(buffer.getLine(3), TextBufferException);
}

// Test case for lineLength
TEST_F(TextBufferTest, LineLength) {
    // Starts with 1 empty line: [""]
    EXPECT_EQ(buffer.lineLength(0), 0);
    
    buffer.replaceLine(0, "Hello");
    EXPECT_EQ(buffer.lineLength(0), 5);
    
    buffer.addLine("World!");
    EXPECT_EQ(buffer.lineLength(1), 6);
    
    // Out of bounds
    EXPECT_THROW(buffer.lineLength(2), TextBufferException); // 2 lines, so index 2 is out of bounds
    EXPECT_THROW(buffer.lineLength(-1), TextBufferException); // size_t will make -1 large positive
}

// Test cases for insertString
TEST_F(TextBufferTest, InsertStringBasic) {
    // Initial line (index 0) is empty
    buffer.insertString(0, 0, "Hello");
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.lineLength(0), 5);

    // Insert at start of existing content
    buffer.insertString(0, 0, "Say ");
    EXPECT_EQ(buffer.getLine(0), "Say Hello");
    EXPECT_EQ(buffer.lineLength(0), 9);

    // Insert in middle
    buffer.insertString(0, 4, "Cruel "); // "Say Cruel Hello"
    EXPECT_EQ(buffer.getLine(0), "Say Cruel Hello");
    EXPECT_EQ(buffer.lineLength(0), 15);

    // Insert at end (colIndex == length)
    buffer.insertString(0, 15, " World");
    EXPECT_EQ(buffer.getLine(0), "Say Cruel Hello World");
    EXPECT_EQ(buffer.lineLength(0), 21);

    // Insert with colIndex > length (should append)
    buffer.insertString(0, 100, "!"); // 100 is > 21
    EXPECT_EQ(buffer.getLine(0), "Say Cruel Hello World!");
    EXPECT_EQ(buffer.lineLength(0), 22);
}

TEST_F(TextBufferTest, InsertEmptyString) {
    buffer.addLine("TestLine"); // ["", "TestLine"]
    ASSERT_EQ(buffer.getLine(1), "TestLine");
    ASSERT_EQ(buffer.lineLength(1), 8);

    buffer.insertString(1, 4, ""); // Insert empty string in middle
    EXPECT_EQ(buffer.getLine(1), "TestLine");
    EXPECT_EQ(buffer.lineLength(1), 8);

    buffer.insertString(0, 0, ""); // Insert empty string into initially empty line 0
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.lineLength(0), 0);
}

TEST_F(TextBufferTest, InsertStringOutOfBoundsLine) {
    // Line out of bounds
    EXPECT_THROW(buffer.insertString(1, 0, "Error"), TextBufferException);
    EXPECT_THROW(buffer.insertString(-1, 0, "Error"), TextBufferException);
    
    // Column out of bounds
    buffer.addLine("Hello");
    EXPECT_THROW(buffer.insertString(0, 10, "Error"), TextBufferException);
}

// Test cases for deleteChar (simulates backspace)
TEST_F(TextBufferTest, DeleteCharBasic) {
    buffer.replaceLine(0, "abcde"); // Line 0: "abcde"
    
    // Delete in middle: cursor after 'c' (colIndex 3), deletes 'c'
    buffer.deleteChar(0, 3);
    EXPECT_EQ(buffer.getLine(0), "abde");

    // Delete at effective start: cursor after 'a' (colIndex 1), deletes 'a'
    buffer.replaceLine(0, "fghij"); // Reset line to "fghij"
    buffer.deleteChar(0, 1);
    EXPECT_EQ(buffer.getLine(0), "ghij");

    // Delete at end: cursor after 'j' (colIndex 4 for "ghij"), deletes 'j'
    buffer.deleteChar(0, 4); 
    EXPECT_EQ(buffer.getLine(0), "ghi");

    // Delete last remaining char: cursor after 'i' (colIndex 3 for "ghi"), deletes 'i'
    buffer.deleteChar(0, 3);
    EXPECT_EQ(buffer.getLine(0), "gh");
    buffer.deleteChar(0, 2);
    EXPECT_EQ(buffer.getLine(0), "g");
    buffer.deleteChar(0, 1);
    EXPECT_EQ(buffer.getLine(0), ""); // Line becomes empty
}

TEST_F(TextBufferTest, DeleteCharLineJoining) {
    buffer.replaceLine(0, "First");
    buffer.addLine("Second"); // Lines: ["First", "Second"]
    ASSERT_EQ(buffer.lineCount(), 2);

    // Delete at start of second line (colIndex 0), joins "Second" to "First"
    buffer.deleteChar(1, 0); 
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "FirstSecond");

    // Reset for another join scenario
    buffer.replaceLine(0, "Hello");
    buffer.addLine(""); // Lines: ["Hello", ""] (empty second line)
    buffer.addLine("World"); // Lines: ["Hello", "", "World"]
    ASSERT_EQ(buffer.lineCount(), 3);

    // Delete at start of empty second line, joins empty line to "Hello"
    buffer.deleteChar(1, 0);
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello"); // "Hello" + "" = "Hello"
    EXPECT_EQ(buffer.getLine(1), "World");
}

TEST_F(TextBufferTest, DeleteCharBoundaryConditions) {
    // Delete at start of first line (colIndex 0) - no change
    buffer.replaceLine(0, "abc");
    buffer.deleteChar(0, 0);
    EXPECT_EQ(buffer.getLine(0), "abc");
    EXPECT_EQ(buffer.lineCount(), 1);

    // Delete from an empty line (colIndex 0) - no change, no join if it's the only line
    buffer.replaceLine(0, "");
    buffer.deleteChar(0, 0);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.lineCount(), 1);

    // Delete with colIndex > line.length() - acts like deleting last char
    buffer.replaceLine(0, "xyz");
    buffer.deleteChar(0, 10); // colIndex 10 on "xyz" (length 3)
    EXPECT_EQ(buffer.getLine(0), "xy");
    
    buffer.deleteChar(0, 10); // colIndex 10 on "xy" (length 2)
    EXPECT_EQ(buffer.getLine(0), "x");

    buffer.deleteChar(0, 10); // colIndex 10 on "x" (length 1)
    EXPECT_EQ(buffer.getLine(0), "");

    // Delete with colIndex > line.length() on an empty line - no change
    buffer.deleteChar(0, 10); // colIndex 10 on "" (length 0)
    EXPECT_EQ(buffer.getLine(0), "");
}

TEST_F(TextBufferTest, DeleteCharOutOfBounds) {
    EXPECT_THROW(buffer.deleteChar(1, 0), TextBufferException); // Only 1 line (index 0)
    EXPECT_THROW(buffer.deleteChar(-1, 0), TextBufferException);
}

// Test cases for deleteCharForward (simulates "delete" key)
TEST_F(TextBufferTest, DeleteCharForwardBasic) {
    buffer.replaceLine(0, "abcde"); // Line 0: "abcde"
    
    // Delete in middle: cursor at 'c' (colIndex 2), deletes 'c'
    buffer.deleteCharForward(0, 2);
    EXPECT_EQ(buffer.getLine(0), "abde");

    // Delete at start: cursor at 'a' (colIndex 0), deletes 'a'
    buffer.replaceLine(0, "fghij"); // Reset line to "fghij"
    buffer.deleteCharForward(0, 0);
    EXPECT_EQ(buffer.getLine(0), "ghij");

    // Delete last char: cursor at 'j' (colIndex 3 on "ghij"), deletes 'j'
    buffer.deleteCharForward(0, 3);
    EXPECT_EQ(buffer.getLine(0), "ghi");

    // Delete all chars one by one from start
    buffer.replaceLine(0, "xyz");
    buffer.deleteCharForward(0, 0); // "yz"
    buffer.deleteCharForward(0, 0); // "z"
    buffer.deleteCharForward(0, 0); // ""
    EXPECT_EQ(buffer.getLine(0), "");
}

TEST_F(TextBufferTest, DeleteCharForwardLineJoining) {
    buffer.replaceLine(0, "First");
    buffer.addLine("Second"); // Lines: ["First", "Second"]
    ASSERT_EQ(buffer.lineCount(), 2);

    // Delete at end of first line (colIndex 5), joins "Second" to "First"
    buffer.deleteCharForward(0, 5); 
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "FirstSecond");

    // Reset for another join scenario
    buffer.replaceLine(0, "Hello");
    buffer.addLine("");      // Lines: ["Hello", ""] (empty second line)
    buffer.addLine("World"); // Lines: ["Hello", "", "World"]
    ASSERT_EQ(buffer.lineCount(), 3);

    // Delete at end of empty second line, joins "World" to it
    buffer.deleteCharForward(1, 0); // colIndex 0 on empty line, joins next
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello"); 
    EXPECT_EQ(buffer.getLine(1), "World"); // "" + "World" = "World"

    // Delete at end of first line, joining the now "World" line
    buffer.deleteCharForward(0, 5); // "Hello" (length 5)
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "HelloWorld");
}

TEST_F(TextBufferTest, DeleteCharForwardBoundaryConditions) {
    // Delete at end of the only line (and last line) - no change
    buffer.replaceLine(0, "abc");
    buffer.deleteCharForward(0, 3); // colIndex 3 on "abc" (length 3)
    EXPECT_EQ(buffer.getLine(0), "abc");
    EXPECT_EQ(buffer.lineCount(), 1);

    // Delete from an empty line (colIndex 0), when it's the only line - no change
    buffer.replaceLine(0, "");
    buffer.deleteCharForward(0, 0);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.lineCount(), 1);

    // Delete with colIndex > line.length() on the only line - no change (acts like at end)
    buffer.replaceLine(0, "xyz");
    buffer.deleteCharForward(0, 10); // colIndex 10 on "xyz" (length 3)
    EXPECT_EQ(buffer.getLine(0), "xyz"); 

    // Delete with colIndex > line.length() on an empty line - no change
    buffer.replaceLine(0, "");
    buffer.deleteCharForward(0, 10); // colIndex 10 on "" (length 0)
    EXPECT_EQ(buffer.getLine(0), "");
}

TEST_F(TextBufferTest, DeleteCharForwardOutOfBounds) {
    EXPECT_THROW(buffer.deleteCharForward(1, 0), TextBufferException); // Only 1 line (index 0)
    EXPECT_THROW(buffer.deleteCharForward(-1, 0), TextBufferException);
}

// Test cases for splitLine
TEST_F(TextBufferTest, SplitLineBasic) {
    buffer.replaceLine(0, "HelloWorld");
    ASSERT_EQ(buffer.lineCount(), 1);

    // 1. Split in the middle
    // "HelloWorld" split at col 5 -> "Hello" and "World"
    buffer.splitLine(0, 5);
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.getLine(1), "World");

    // Reset buffer to a single line for next test
    buffer.clear(); // Clears to one empty line
    buffer.replaceLine(0, "SplitAtStart");
    ASSERT_EQ(buffer.lineCount(), 1);

    // 2. Split at the beginning (colIndex 0)
    // "SplitAtStart" split at col 0 -> "" and "SplitAtStart"
    buffer.splitLine(0, 0);
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "SplitAtStart");

    // Reset buffer
    buffer.clear();
    buffer.replaceLine(0, "SplitAtEnd");
    ASSERT_EQ(buffer.lineCount(), 1);

    // 3. Split at the end (colIndex == length)
    // "SplitAtEnd" split at col 10 -> "SplitAtEnd" and ""
    buffer.splitLine(0, buffer.lineLength(0)); // buffer.lineLength(0) is 10
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "SplitAtEnd");
    EXPECT_EQ(buffer.getLine(1), "");

    // Reset buffer
    buffer.clear(); // Clears to one empty line ""
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");

    // 4. Split an empty line
    // "" split at col 0 -> "" and ""
    buffer.splitLine(0, 0);
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "");
}

TEST_F(TextBufferTest, SplitLineOutOfBounds) {
    buffer.replaceLine(0, "SomeContent"); // Initial line: "SomeContent"
    ASSERT_EQ(buffer.lineCount(), 1);

    // Out of bounds indices - line
    EXPECT_THROW(buffer.splitLine(1, 0), TextBufferException); // Line 1 does not exist
    EXPECT_THROW(buffer.splitLine(-1, 0), TextBufferException);
    
    // Out of bounds - column
    buffer.replaceLine(0, "Hello World");
    EXPECT_THROW(buffer.splitLine(0, 12), TextBufferException); // col 12 is > length 11
    EXPECT_THROW(buffer.splitLine(0, buffer.lineLength(0) + 1), TextBufferException);
    
    // Empty line special case
    buffer.replaceLine(0, "");
    EXPECT_NO_THROW(buffer.splitLine(0, 0)); // col 0 is valid on empty line
    EXPECT_THROW(buffer.splitLine(0, 1), TextBufferException); // col 1 on empty line is out of bounds
}

// Test cases for joinLines
TEST_F(TextBufferTest, JoinLinesBasic) {
    // 1. Join two non-empty lines
    buffer.replaceLine(0, "First ");
    buffer.addLine("Second"); // Lines: ["First ", "Second"]
    ASSERT_EQ(buffer.lineCount(), 2);
    buffer.joinLines(0);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "First Second");

    // 2. Join a non-empty line with an empty next line
    buffer.replaceLine(0, "NotEmpty");
    buffer.addLine(""); // Lines: ["NotEmpty", ""]
    ASSERT_EQ(buffer.lineCount(), 2);
    buffer.joinLines(0);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "NotEmpty"); // "NotEmpty" + "" = "NotEmpty"

    // 3. Join an empty line with a non-empty next line
    buffer.replaceLine(0, "");
    buffer.addLine("NotEmptyNext"); // Lines: ["", "NotEmptyNext"]
    ASSERT_EQ(buffer.lineCount(), 2);
    buffer.joinLines(0);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "NotEmptyNext"); // "" + "NotEmptyNext" = "NotEmptyNext"

    // 4. Join two empty lines
    buffer.replaceLine(0, "");
    buffer.addLine(""); // Lines: ["", ""]
    ASSERT_EQ(buffer.lineCount(), 2);
    buffer.joinLines(0);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), ""); // "" + "" = ""

    // 5. Join when there are more than two lines
    buffer.replaceLine(0, "LineA");
    buffer.addLine("LineB");
    buffer.addLine("LineC"); // Lines: ["LineA", "LineB", "LineC"]
    ASSERT_EQ(buffer.lineCount(), 3);
    buffer.joinLines(0); // Join LineA and LineB
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "LineALineB");

    buffer.joinLines(0); // Join LineALineB and LineC
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "LineALineBLineC");
}

TEST_F(TextBufferTest, JoinLinesOutOfBounds) {
    // Only one line, can't join
    EXPECT_THROW(buffer.joinLines(0), TextBufferException);
    
    // Add a second line, now can join at index 0
    buffer.addLine("Second Line");
    EXPECT_NO_THROW(buffer.joinLines(0));
    
    // Back to one line, can't join
    EXPECT_THROW(buffer.joinLines(0), TextBufferException);
    
    // Out of range indices
    EXPECT_THROW(buffer.joinLines(100), TextBufferException);
    EXPECT_THROW(buffer.joinLines(-1), TextBufferException);
}

// Test cases for saveToFile and loadFromFile
TEST_F(TextBufferTest, SaveAndLoadTypicalContent) {
    const std::string tempFilename = "temp_textbuffer_test_typical.txt";

    std::vector<std::string> originalLines = {
        "First line.",
        "", // Empty line
        "  Third line with spaces.  ",
        "A_final_line!@#"
    };

    buffer.clear(false); // Make buffer initially empty (0 lines)
    for (const auto& line : originalLines) {
        buffer.addLine(line);
    }
    ASSERT_EQ(buffer.lineCount(), originalLines.size());

    // Save
    ASSERT_TRUE(buffer.saveToFile(tempFilename));

    // Verify file content (optional, but good for deep check)
    std::ifstream verifyFile(tempFilename);
    ASSERT_TRUE(verifyFile.is_open());
    std::string fileLine;
    size_t currentLine = 0;
    while (std::getline(verifyFile, fileLine)) {
        ASSERT_LT(currentLine, originalLines.size());
        EXPECT_EQ(fileLine, originalLines[currentLine]);
        currentLine++;
    }
    EXPECT_EQ(currentLine, originalLines.size()); // Ensure all lines were read
    verifyFile.close();

    // Load into a new buffer
    TextBuffer loadedBuffer;
    ASSERT_TRUE(loadedBuffer.loadFromFile(tempFilename));

    // Verify loaded content
    ASSERT_EQ(loadedBuffer.lineCount(), originalLines.size());
    for (size_t i = 0; i < originalLines.size(); ++i) {
        EXPECT_EQ(loadedBuffer.getLine(i), originalLines[i]);
    }

    // Clean up
    std::remove(tempFilename.c_str());
}

TEST_F(TextBufferTest, SaveAndLoadSpecificBufferStates) {
    const std::string tempFilename = "temp_textbuffer_test_specific.txt";

    // 1. Test with a buffer in its default state (one empty line)
    buffer.clear(true); // Ensures state is [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");

    ASSERT_TRUE(buffer.saveToFile(tempFilename));

    TextBuffer loadedBuffer1;
    ASSERT_TRUE(loadedBuffer1.loadFromFile(tempFilename));
    ASSERT_EQ(loadedBuffer1.lineCount(), 1);
    EXPECT_EQ(loadedBuffer1.getLine(0), "");

    std::remove(tempFilename.c_str());

    // 2. Test with a buffer made truly empty (0 lines)
    // This depends on saveToFile saving an empty file if lines_ empty,
    // and loadFromFile loading an empty file as 0 lines.
    buffer.clear(false); // Makes lines_ empty
    ASSERT_EQ(buffer.lineCount(), 0);
    ASSERT_TRUE(buffer.isEmpty());

    ASSERT_TRUE(buffer.saveToFile(tempFilename));

    // Verify the file is indeed empty (0 bytes)
    std::ifstream checkEmptyFile(tempFilename, std::ios::ate | std::ios::binary);
    ASSERT_TRUE(checkEmptyFile.is_open());
    EXPECT_EQ(checkEmptyFile.tellg(), 0);
    checkEmptyFile.close();

    TextBuffer loadedBuffer2;
    ASSERT_TRUE(loadedBuffer2.loadFromFile(tempFilename));
    EXPECT_EQ(loadedBuffer2.lineCount(), 0);
    EXPECT_TRUE(loadedBuffer2.isEmpty());

    std::remove(tempFilename.c_str());
}

TEST_F(TextBufferTest, FileOperationFailureCases) {
    // 1. Load from a non-existent file
    TextBuffer freshBuffer;
    EXPECT_FALSE(freshBuffer.loadFromFile("non_existent_temp_file.txt"));
    // Buffer should remain in default state: one empty line
    EXPECT_EQ(freshBuffer.lineCount(), 1);
    EXPECT_EQ(freshBuffer.getLine(0), "");

    // 2. Save to an invalid filename (e.g., empty string)
    // The TextBuffer::saveToFile prints to cerr but should return false.
    buffer.clear(true); // Known state
    buffer.addLine("Some content");
    size_t lineCountBeforeSave = buffer.lineCount();
    
    // Note: Behavior of saving to "" can be OS-dependent for std::ofstream.
    // However, robust code should handle it or it should be disallowed by TextBuffer earlier.
    // We expect TextBuffer::saveToFile to return false.
    EXPECT_FALSE(buffer.saveToFile(""));
    
    // Ensure buffer content is unchanged after failed save
    EXPECT_EQ(buffer.lineCount(), lineCountBeforeSave);
    EXPECT_EQ(buffer.getLine(0),""); // From clear(true)
    EXPECT_EQ(buffer.getLine(1), "Some content");
}

// Test cases for getLineSegment
TEST_F(TextBufferTest, GetLineSegmentBasic) {
    buffer.replaceLine(0, "ThisIsALongLine"); // Length 15
    ASSERT_EQ(buffer.lineLength(0), 15);

    // 1. Segment from the middle
    EXPECT_EQ(buffer.getLineSegment(0, 4, 6), "Is"); // "Is" from "This[Is]ALongLine"

    // 2. Segment from the start
    EXPECT_EQ(buffer.getLineSegment(0, 0, 4), "This"); // "This"

    // 3. Segment to the end
    EXPECT_EQ(buffer.getLineSegment(0, 11, 15), "Line"); // "Line" from "ThisIsALong[Line]"

    // 4. Full line segment
    EXPECT_EQ(buffer.getLineSegment(0, 0, 15), "ThisIsALongLine");

    // 5. Empty segment (startCol == endCol)
    EXPECT_EQ(buffer.getLineSegment(0, 5, 5), "");
    EXPECT_EQ(buffer.getLineSegment(0, 0, 0), ""); // At start
    EXPECT_EQ(buffer.getLineSegment(0, 15, 15), ""); // At end

    // 6. Segment from an empty line
    buffer.replaceLine(0, "");
    ASSERT_EQ(buffer.lineLength(0), 0);
    EXPECT_EQ(buffer.getLineSegment(0, 0, 0), "");
}

TEST_F(TextBufferTest, GetLineSegmentOutOfBounds) {
    buffer.replaceLine(0, "abcdefg"); // length 7, indices 0-6
    ASSERT_EQ(buffer.lineCount(), 1);

    // Line index out of bounds
    EXPECT_THROW(buffer.getLineSegment(1, 0, 1), TextBufferException);
    EXPECT_THROW(buffer.getLineSegment(-1, 0, 1), TextBufferException);
    
    // startCol > line.length()
    EXPECT_THROW(buffer.getLineSegment(0, 8, 8), TextBufferException); // startCol=8, length=7
    
    // endCol > line.length() should now be clamped instead of throwing
    EXPECT_NO_THROW(buffer.getLineSegment(0, 0, 8)); // endCol=8 should be clamped to 7
    EXPECT_EQ(buffer.getLineSegment(0, 0, 8), "abcdefg"); // Should get the full string
    
    // startCol > endCol
    EXPECT_THROW(buffer.getLineSegment(0, 5, 4), TextBufferException);
    
    // Valid cases
    EXPECT_EQ(buffer.getLineSegment(0, 0, 7), "abcdefg"); // Full line
    EXPECT_EQ(buffer.getLineSegment(0, 1, 3), "bc");      // Middle segment
    
    // Empty buffer case
    buffer.replaceLine(0, "");
    EXPECT_EQ(buffer.getLineSegment(0, 0, 0), "");        // Empty line, valid empty segment
    EXPECT_THROW(buffer.getLineSegment(0, 1, 1), TextBufferException); // startCol=1 on empty line
}

// TODO: Add more tests for:
// - deleteLine (various positions, last line, only line)
// - replaceLine
// - getLine (out of bounds, though it throws)
// - lineLength
// - insertString (at start, middle, end of line, empty string)
// - deleteChar (at start, middle, end of line, across line boundaries if it handles join)
// - deleteCharForward (similar to deleteChar)
// - splitLine (at start, middle, end of line)
// - joinLines (various scenarios)
// - saveToFile / loadFromFile (might need helper for temp files or be integration tests)
// - Edge cases for all operations.
// - Test throwing EditorException for invalid operations (e.g., getLine out of bounds). 