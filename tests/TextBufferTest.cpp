#include "../src/TextBuffer.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cassert>

void testTextBufferInitialization() {
    std::cout << "Testing TextBuffer initialization...\n";
    
    TextBuffer buffer;
    if (buffer.lineCount() >= 1) {
        std::cout << "  PASS: Buffer starts with at least one line\n";
    } else {
        std::cout << "  FAIL: Buffer should start with at least one line\n";
    }
}

void testTextBufferInsertString() {
    std::cout << "Testing insertString...\n";
    
    TextBuffer buffer;
    buffer.clear(true);
    buffer.setLine(0, "Hello");
    
    // Insert at beginning
    buffer.insertString(0, 0, "Start-");
    if (buffer.getLine(0) == "Start-Hello") {
        std::cout << "  PASS: Insert at beginning\n";
    } else {
        std::cout << "  FAIL: Insert at beginning - Expected 'Start-Hello', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Insert at middle
    buffer.insertString(0, 6, ", ");
    if (buffer.getLine(0) == "Start-, Hello") {
        std::cout << "  PASS: Insert in middle\n";
    } else {
        std::cout << "  FAIL: Insert in middle - Expected 'Start-, Hello', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Insert at end
    buffer.insertString(0, buffer.getLine(0).length(), " End");
    if (buffer.getLine(0) == "Start-, Hello End") {
        std::cout << "  PASS: Insert at end\n";
    } else {
        std::cout << "  FAIL: Insert at end - Expected 'Start-, Hello End', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Insert beyond end (should clamp to end)
    buffer.insertString(0, 100, "!");
    if (buffer.getLine(0) == "Start-, Hello End!") {
        std::cout << "  PASS: Insert beyond end (clamped)\n";
    } else {
        std::cout << "  FAIL: Insert beyond end - Expected 'Start-, Hello End!', got '" 
                  << buffer.getLine(0) << "'\n";
    }
}

void testTextBufferDeleteChar() {
    std::cout << "Testing deleteChar...\n";
    
    TextBuffer buffer;
    buffer.clear(true);
    buffer.setLine(0, "Hello");
    
    // Delete within line
    buffer.deleteChar(0, 2); // Delete 'e' (position at index 1)
    if (buffer.getLine(0) == "Hllo") {
        std::cout << "  PASS: Delete within line\n";
    } else {
        std::cout << "  FAIL: Delete within line - Expected 'Hllo', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Delete at beginning of first line (no effect)
    buffer.deleteChar(0, 0);
    if (buffer.getLine(0) == "Hllo") {
        std::cout << "  PASS: Delete at beginning of first line (no effect)\n";
    } else {
        std::cout << "  FAIL: Delete at beginning of first line - Expected 'Hllo', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Delete beyond end (should delete at end)
    buffer.deleteChar(0, 10);
    if (buffer.getLine(0) == "Hll") {
        std::cout << "  PASS: Delete beyond end (deletes at end)\n";
    } else {
        std::cout << "  FAIL: Delete beyond end - Expected 'Hll', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Test joining lines
    buffer.clear(true);
    buffer.setLine(0, "First");
    buffer.addLine("Second");
    buffer.deleteChar(1, 0); // Delete the newline between lines
    if (buffer.lineCount() == 1 && buffer.getLine(0) == "FirstSecond") {
        std::cout << "  PASS: Join lines with backspace\n";
    } else {
        std::cout << "  FAIL: Join lines - Expected 'FirstSecond', got '" 
                  << buffer.getLine(0) << "' with " << buffer.lineCount() << " lines\n";
    }
}

void testTextBufferDeleteCharForward() {
    std::cout << "Testing deleteCharForward...\n";
    
    TextBuffer buffer;
    buffer.clear(true);
    buffer.setLine(0, "Hello");
    
    // Delete within line
    buffer.deleteCharForward(0, 2); // Delete 'l' at position 2
    if (buffer.getLine(0) == "Helo") {
        std::cout << "  PASS: Delete forward within line\n";
    } else {
        std::cout << "  FAIL: Delete forward within line - Expected 'Helo', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Delete at end of last line (no effect)
    buffer.deleteCharForward(0, buffer.getLine(0).length());
    if (buffer.getLine(0) == "Helo") {
        std::cout << "  PASS: Delete forward at end of last line (no effect)\n";
    } else {
        std::cout << "  FAIL: Delete forward at end of last line - Expected 'Helo', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Delete beyond end (should have no effect on last line)
    buffer.deleteCharForward(0, 10);
    if (buffer.getLine(0) == "Helo") {
        std::cout << "  PASS: Delete forward beyond end of last line (no effect)\n";
    } else {
        std::cout << "  FAIL: Delete forward beyond end - Expected 'Helo', got '" 
                  << buffer.getLine(0) << "'\n";
    }
    
    // Test joining lines
    buffer.clear(true);
    buffer.setLine(0, "First");
    buffer.addLine("Second");
    buffer.deleteCharForward(0, buffer.getLine(0).length()); // Delete at end of first line
    if (buffer.lineCount() == 1 && buffer.getLine(0) == "FirstSecond") {
        std::cout << "  PASS: Join lines with delete forward\n";
    } else {
        std::cout << "  FAIL: Join lines forward - Expected 'FirstSecond', got '" 
                  << buffer.getLine(0) << "' with " << buffer.lineCount() << " lines\n";
    }
}

void testTextBufferDeleteLine() {
    std::cout << "Testing deleteLine...\n";
    
    TextBuffer buffer;
    
    // Test deleting the only line
    buffer.clear(true);
    try {
        buffer.deleteLine(0);
        std::cout << "  FAIL: Should throw when deleting the only line\n";
    } catch (const std::exception& e) {
        std::cout << "  PASS: Exception when deleting the only line: " << e.what() << "\n";
    }
    
    // Test deleting a line among multiple
    buffer.clear(true);
    buffer.setLine(0, "Line 0");
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    
    buffer.deleteLine(1);
    if (buffer.lineCount() == 2 && buffer.getLine(0) == "Line 0" && buffer.getLine(1) == "Line 2") {
        std::cout << "  PASS: Delete middle line\n";
    } else {
        std::cout << "  FAIL: Delete middle line - Expected 2 lines, got " 
                  << buffer.lineCount() << "\n";
    }
}

int main() {
    try {
        testTextBufferInitialization();
        std::cout << std::endl;
        
        testTextBufferInsertString();
        std::cout << std::endl;
        
        testTextBufferDeleteChar();
        std::cout << std::endl;
        
        testTextBufferDeleteCharForward();
        std::cout << std::endl;
        
        testTextBufferDeleteLine();
        
        std::cout << "\nAll TextBuffer tests completed successfully.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
} 