#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "MinimalTextBuffer.h"

void runMinimalTextBufferTests() {
    std::cout << "=== Starting MinimalTextBuffer Tests ===\n";
    
    // Test 1: Create and check empty buffer
    {
        std::cout << "Test 1: Empty buffer... ";
        MinimalTextBuffer buffer;
        if (!buffer.isEmpty() || buffer.getLineCount() != 1 || !buffer.getLine(0).empty()) {
            std::cout << "FAILED: Empty buffer not initialized correctly\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    // Test 2: Insert text
    {
        std::cout << "Test 2: Insert text... ";
        MinimalTextBuffer buffer;
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
        MinimalTextBuffer buffer;
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
        MinimalTextBuffer buffer;
        buffer.insertText(0, 0, "Hello, World!");
        buffer.deleteText(0, 7, 0, 12);  // Remove "World"
        
        if (buffer.getLine(0) != "Hello, !") {
            std::cout << "FAILED: Text deletion failed\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    std::cout << "=== All MinimalTextBuffer Tests PASSED ===\n";
}

int main() {
    try {
        runMinimalTextBufferTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
