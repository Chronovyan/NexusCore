#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

// Include our stub ErrorReporter first to avoid including the real one
#include "ErrorReporterStub.h"

// Now include TextBuffer.h which will use our stub
#include "TextBuffer.h"

// Define a simple exception class for testing
class TextBufferException : public std::runtime_error {
public:
    TextBufferException(const std::string& msg) : std::runtime_error(msg) {}
};

void runTextBufferTests() {
    std::cout << "=== Starting TextBuffer Tests ===\n";
    
    // Test 1: Create and check empty buffer
    {
        std::cout << "Test 1: Empty buffer... ";
        TextBuffer buffer;
        if (!buffer.isEmpty() || buffer.getLineCount() != 1 || !buffer.getLine(0).empty()) {
            std::cout << "FAILED: Empty buffer not initialized correctly\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    // Test 2: Insert text
    {
        std::cout << "Test 2: Insert text... ";
        TextBuffer buffer;
        buffer.insertText(0, 0, "Hello");
        
        if (buffer.getLine(0) != "Hello") {
            std::cout << "FAILED: Text insertion failed\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    // Test 3: Insert multiple lines
    {
        std::cout << "Test 3: Insert multiple lines... ";
        TextBuffer buffer;
        std::vector<std::string> lines = {"Line 1", "Line 2", "Line 3"};
        buffer.insertLines(0, lines);
        
        if (buffer.getLineCount() != 3 || 
            buffer.getLine(0) != "Line 1" || 
            buffer.getLine(1) != "Line 2" || 
            buffer.getLine(2) != "Line 3") {
            std::cout << "FAILED: Multi-line insertion failed\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    // Test 4: Delete text
    {
        std::cout << "Test 4: Delete text... ";
        TextBuffer buffer;
        buffer.insertText(0, 0, "Hello, World!");
        buffer.deleteText(0, 7, 0, 12);  // Remove "World"
        
        if (buffer.getLine(0) != "Hello, !") {
            std::cout << "FAILED: Text deletion failed\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    // Note: Removing Undo/Redo test as these methods don't exist in the interface
    
    std::cout << "=== All TextBuffer Tests PASSED ===\n";
}

int main() {
    try {
        runTextBufferTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
