#include <iostream>
#include <string>
#include <memory>
#include "TextBuffer.h"

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
        buffer.insert(0, 0, "Hello");
        
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
        buffer.insert(0, 0, "Line 1\nLine 2\nLine 3");
        
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
        buffer.insert(0, 0, "Hello, World!");
        buffer.remove(0, 7, 5);  // Remove "World"
        
        if (buffer.getLine(0) != "Hello, !") {
            std::cout << "FAILED: Text deletion failed\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
    // Test 5: Undo/Redo
    {
        std::cout << "Test 5: Undo/Redo... ";
        TextBuffer buffer;
        buffer.insert(0, 0, "Hello");
        buffer.saveState();
        buffer.insert(0, 5, ", World!");
        
        // Undo
        buffer.undo();
        if (buffer.getLine(0) != "Hello") {
            std::cout << "FAILED: Undo failed\n";
            return;
        }
        
        // Redo
        buffer.redo();
        if (buffer.getLine(0) != "Hello, World!") {
            std::cout << "FAILED: Redo failed\n";
            return;
        }
        std::cout << "PASSED\n";
    }
    
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
