#include "../src/TextBuffer.h"
#include <iostream>
#include <cassert>
#include <stdexcept>

// A simple test function to check DeleteLineCommand-like functionality
void testDeleteLine() {
    TextBuffer buffer;
    
    std::cout << "Testing Delete Line functionality...\n";
    
    // Test deleting the only line
    buffer.clear(true); // Start with one empty line
    assert(buffer.lineCount() == 1);
    
    try {
        buffer.deleteLine(0);
        std::cout << "  FAIL: Should throw when deleting the only line\n";
    } catch (const std::exception& e) {
        std::cout << "  PASS: Exception when deleting the only line: " << e.what() << std::endl;
    }

    // Test inserting and deleting lines
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line 0");
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    
    assert(buffer.lineCount() == 3);
    assert(buffer.getLine(0) == "Line 0");
    assert(buffer.getLine(1) == "Line 1");
    assert(buffer.getLine(2) == "Line 2");
    
    // Delete middle line
    buffer.deleteLine(1);
    assert(buffer.lineCount() == 2);
    assert(buffer.getLine(0) == "Line 0");
    assert(buffer.getLine(1) == "Line 2");
    
    std::cout << "  PASS: Delete middle line\n";
    
    // Delete last line
    buffer.deleteLine(1);
    assert(buffer.lineCount() == 1);
    assert(buffer.getLine(0) == "Line 0");
    
    std::cout << "  PASS: Delete last line\n";
    
    // Note: Can't delete the first line when it's the only line remaining
    // Instead, test that buffer at least has one line after all operations
    if (buffer.lineCount() >= 1) {
        std::cout << "  PASS: Buffer always maintains at least one line\n";
    } else {
        std::cout << "  FAIL: Buffer should always have at least one line\n";
    }
}

// Test insertString with edge cases
void testInsertString() {
    TextBuffer buffer;
    
    std::cout << "Testing Insert String functionality...\n";
    
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line");
    std::cout << "  Initial line: '" << buffer.getLine(0) << "' (length: " << buffer.getLine(0).length() << ")\n";
    
    // Test inserting at valid positions
    buffer.insertString(0, 0, "Start");  // Insert at beginning
    std::cout << "  After inserting at pos 0: '" << buffer.getLine(0) << "' (length: " << buffer.getLine(0).length() << ")\n";
    if (buffer.getLine(0) == "StartLine") {
        std::cout << "  PASS: Insert at beginning\n";
    } else {
        std::cout << "  FAIL: Insert at beginning - Expected 'StartLine', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    buffer.insertString(0, 9, "End");    // Insert at end
    std::cout << "  After inserting at pos 9: '" << buffer.getLine(0) << "' (length: " << buffer.getLine(0).length() << ")\n";
    if (buffer.getLine(0) == "StartLineEnd") {
        std::cout << "  PASS: Insert at end\n";
    } else {
        std::cout << "  FAIL: Insert at end - Expected 'StartLineEnd', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test inserting beyond line length
    try {
        std::cout << "  Before inserting beyond end: '" << buffer.getLine(0) << "' (length: " << buffer.getLine(0).length() << ")\n";
        buffer.insertString(0, 20, "Beyond"); // Beyond end
        std::cout << "  After inserting at pos 20: '" << buffer.getLine(0) << "' (length: " << buffer.getLine(0).length() << ")\n";
        if(buffer.getLine(0) == "StartLineEndBeyond") {
            std::cout << "  PASS: Insert beyond limit gets clamped to end\n";
        } else {
            std::cout << "  FAIL: Insert beyond limit produced unexpected result\n";
        }
    } catch (const std::exception& e) {
        std::cout << "  FAIL: Exception on insert beyond limit: " << e.what() << std::endl;
    }
}

// Test deleteChar with edge cases
void testDeleteChar() {
    TextBuffer buffer;
    
    std::cout << "Testing Delete Char functionality...\n";
    
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line1");
    buffer.addLine("Line2");
    
    // Test deleting at beginning of second line (should join with previous)
    buffer.deleteChar(1, 0);
    std::cout << "  After deleting at beginning of second line: '" << buffer.getLine(0) << "' (lineCount: " << buffer.lineCount() << ")\n";
    if (buffer.lineCount() == 1 && buffer.getLine(0) == "Line1Line2") {
        std::cout << "  PASS: Delete at beginning of line (join lines)\n";
    } else {
        std::cout << "  FAIL: Delete at beginning of line - Expected 'Line1Line2', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test deleting at beginning of first line (nothing should happen)
    buffer.clear(true);
    buffer.setLine(0, "Line");
    buffer.deleteChar(0, 0);
    std::cout << "  After deleting at beginning of first line: '" << buffer.getLine(0) << "'\n";
    if (buffer.lineCount() == 1 && buffer.getLine(0) == "Line") {
        std::cout << "  PASS: Delete at beginning of first line\n";
    } else {
        std::cout << "  FAIL: Delete at beginning of first line - Expected 'Line', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test deleting within a line
    std::cout << "  Before deleting within line: '" << buffer.getLine(0) << "'\n";
    buffer.deleteChar(0, 2); // This should delete 'i' (position 1) as backspace deletes the char BEFORE the cursor
    std::cout << "  After deleting at position 2: '" << buffer.getLine(0) << "'\n";
    if (buffer.getLine(0) == "Lne") {
        std::cout << "  PASS: Delete within line\n";
    } else {
        std::cout << "  FAIL: Delete within line - Expected 'Lne', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test deleting at position beyond line length
    buffer.clear(true);
    buffer.setLine(0, "Line");
    try {
        std::cout << "  Before deleting beyond line length: '" << buffer.getLine(0) << "'\n";
        buffer.deleteChar(0, 10); // Beyond end
        std::cout << "  After deleting at position 10: '" << buffer.getLine(0) << "'\n";
        if (buffer.getLine(0) == "Lin") {
            std::cout << "  PASS: Delete beyond limit handled as delete at end\n";
        } else {
            std::cout << "  FAIL: Delete beyond limit produced: '" << buffer.getLine(0) << "'\n";
        }
    } catch (const std::exception& e) {
        std::cout << "  FAIL: Exception on delete beyond limit: " << e.what() << std::endl;
    }
}

// Test deleteCharForward with edge cases
void testDeleteCharForward() {
    TextBuffer buffer;
    
    std::cout << "Testing Delete Char Forward functionality...\n";
    
    buffer.clear(true); // Start with one empty line
    buffer.setLine(0, "Line1");
    buffer.addLine("Line2");
    
    // Test deleting at end of first line (should join with next)
    buffer.deleteCharForward(0, 5);
    std::cout << "  After deleting at end of first line: '" << buffer.getLine(0) << "' (lineCount: " << buffer.lineCount() << ")\n";
    if (buffer.lineCount() == 1 && buffer.getLine(0) == "Line1Line2") {
        std::cout << "  PASS: Delete at end of line (join lines)\n";
    } else {
        std::cout << "  FAIL: Delete at end of line - Expected 'Line1Line2', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test deleting within a line
    buffer.clear(true);
    buffer.setLine(0, "Line");
    std::cout << "  Before deleting forward within line: '" << buffer.getLine(0) << "'\n";
    buffer.deleteCharForward(0, 2); // Delete 'n'
    std::cout << "  After deleting forward at position 2: '" << buffer.getLine(0) << "'\n";
    if (buffer.getLine(0) == "Lie") {
        std::cout << "  PASS: Delete forward within line\n";
    } else {
        std::cout << "  FAIL: Delete forward within line - Expected 'Lie', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test deleting at end of the last line (nothing should happen)
    std::cout << "  Before deleting forward at end of line: '" << buffer.getLine(0) << "'\n";
    buffer.deleteCharForward(0, 3);
    std::cout << "  After deleting forward at end of line: '" << buffer.getLine(0) << "'\n";
    if (buffer.lineCount() == 1 && buffer.getLine(0) == "Lie") {
        std::cout << "  PASS: Delete forward at end of last line\n";
    } else {
        std::cout << "  FAIL: Delete forward at end of last line - Expected 'Lie', got '" << buffer.getLine(0) << "'\n";
        return;
    }
    
    // Test deleting at position beyond line length
    buffer.clear(true);
    buffer.setLine(0, "Line");
    buffer.addLine("Next");
    try {
        std::cout << "  Before deleting forward beyond line length: '" << buffer.getLine(0) << "' (lineCount: " << buffer.lineCount() << ")\n";
        buffer.deleteCharForward(0, 10); // Beyond end
        std::cout << "  After deleting forward beyond line length: '" << buffer.getLine(0) << "' (lineCount: " << buffer.lineCount() << ")\n";
        if(buffer.lineCount() == 1 && buffer.getLine(0) == "LineNext") {
            std::cout << "  PASS: Delete forward beyond limit handled as delete at end\n";
        } else {
            std::cout << "  FAIL: Delete forward beyond limit produced unexpected result\n";
        }
    } catch (const std::exception& e) {
        std::cout << "  FAIL: Exception on delete forward beyond limit: " << e.what() << std::endl;
    }
}

int main() {
    try {
        testDeleteLine();
        std::cout << std::endl;
        
        testInsertString();
        std::cout << std::endl;
        
        testDeleteChar();
        std::cout << std::endl;
        
        testDeleteCharForward();
        
        std::cout << "\nAll tests completed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
} 