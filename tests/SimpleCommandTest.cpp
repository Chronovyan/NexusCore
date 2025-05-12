#include <gtest/gtest.h>
#include "../src/TextBuffer.h"
#include "../src/EditorError.h"
#include <cassert>
#include <stdexcept>

// Test DeleteLine functionality
TEST(SimpleTextBufferTest, DeleteLine) {
    TextBuffer buffer;
    
    // Test deleting the only line
    buffer.clear(true); // Start with one empty line
    ASSERT_EQ(buffer.lineCount(), 1);
    
    buffer.deleteLine(0);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");

    // Test inserting and deleting lines
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line 0");
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    
    ASSERT_EQ(buffer.lineCount(), 3);
    ASSERT_EQ(buffer.getLine(0), "Line 0");
    ASSERT_EQ(buffer.getLine(1), "Line 1");
    ASSERT_EQ(buffer.getLine(2), "Line 2");
    
    // Delete middle line
    buffer.deleteLine(1);
    ASSERT_EQ(buffer.lineCount(), 2);
    ASSERT_EQ(buffer.getLine(0), "Line 0");
    ASSERT_EQ(buffer.getLine(1), "Line 2");
    
    // Delete last line
    buffer.deleteLine(1);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "Line 0");
}

// Test insertString with edge cases
TEST(SimpleTextBufferTest, InsertString) {
    TextBuffer buffer;
    
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line");
    
    // Test inserting at valid positions
    buffer.insertString(0, 0, "Start");  // Insert at beginning
    ASSERT_EQ(buffer.getLine(0), "StartLine");
    
    buffer.insertString(0, 9, "End");    // Insert at end
    ASSERT_EQ(buffer.getLine(0), "StartLineEnd");
    
    // Test inserting beyond line length
    std::string lineBeforeThrow = buffer.getLine(0);
    ASSERT_THROW(buffer.insertString(0, 20, "Beyond"), TextBufferException);
    ASSERT_EQ(buffer.getLine(0), lineBeforeThrow);
}

// Test deleteChar with edge cases
TEST(SimpleTextBufferTest, DeleteChar) {
    TextBuffer buffer;
    
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line1");
    buffer.addLine("Line2");
    
    // Test deleting at beginning of second line (should join with previous)
    buffer.deleteChar(1, 0);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "Line1Line2");
    
    // Test deleting at beginning of first line (nothing should happen)
    buffer.clear(true);
    buffer.setLine(0, "Line");
    buffer.deleteChar(0, 0);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "Line");
    
    // Test deleting within a line
    buffer.deleteChar(0, 2);
    ASSERT_EQ(buffer.getLine(0), "Lne");
    
    // Test deleting at position beyond line length
    buffer.clear(true);
    buffer.setLine(0, "Line");
    buffer.deleteChar(0, 10);
    ASSERT_EQ(buffer.getLine(0), "Lin");
}

// Test deleteCharForward with edge cases
TEST(SimpleTextBufferTest, DeleteCharForward) {
    TextBuffer buffer;
    
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line1");
    buffer.addLine("Line2");
    
    // Test deleting at end of first line (should join with next)
    buffer.deleteCharForward(0, 5);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "Line1Line2");
    
    // Test deleting within a line
    buffer.clear(true);
    buffer.setLine(0, "Line");
    buffer.deleteCharForward(0, 2);
    ASSERT_EQ(buffer.getLine(0), "Lie");
    
    // Test deleting at end of the last line (nothing should happen)
    buffer.deleteCharForward(0, 3);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "Lie");
    
    // Test deleting at position beyond line length
    buffer.clear(true);
    buffer.setLine(0, "Line");
    buffer.addLine("Next");
    buffer.deleteCharForward(0, 10);
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "LineNext");
}

/*
int main() {
    try {
        // testDeleteLine();
        // std::cout << std::endl;
        
        // testInsertString();
        // std::cout << std::endl;
        
        // testDeleteChar();
        // std::cout << std::endl;
        
        // testDeleteCharForward();
        // std::cout << std::endl;
        
        // std::cout << "\nAll tests completed.\n";
        // return 0;
    } catch (const std::exception& e) {
        // std::cerr << "Exception: " << e.what() << std::endl;
        // return 1;
    } catch (...) {
        // std::cerr << "Unknown exception occurred" << std::endl;
        // return 1;
    }
}
*/ 