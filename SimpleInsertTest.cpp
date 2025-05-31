#include "SimpleTextBuffer.h"
#include <iostream>
#include <string>

int main() {
    SimpleTextBuffer buffer;
    
    // Create a test string with specific content
    buffer.replaceLine(0, "Original");
    buffer.insertString(0, 8, " Line");
    
    std::cout << "Original string: '" << buffer.getLine(0) << "'" << std::endl;
    
    // Insert at a specific position
    buffer.insertString(0, 8, " Modified");
    
    std::cout << "After insertion: '" << buffer.getLine(0) << "'" << std::endl;
    
    return 0;
} 