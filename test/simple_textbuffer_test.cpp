#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

// Forward declare what we need from TextBuffer
class TextBuffer {
    std::vector<std::string> lines_;
    
public:
    TextBuffer() {
        lines_.push_back("");
    }
    
    void insertText(size_t line, size_t col, const std::string& text) {
        if (line >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        
        if (col > lines_[line].length()) {
            col = lines_[line].length();
        }
        
        lines_[line].insert(col, text);
    }
    
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
        if (startLine != endLine) {
            throw std::runtime_error("Multi-line delete not implemented");
        }
        
        if (startLine >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        
        std::string& line = lines_[startLine];
        if (startCol > line.length()) startCol = line.length();
        if (endCol > line.length()) endCol = line.length();
        
        line.erase(startCol, endCol - startCol);
    }
    
    const std::string& getLine(size_t index) const {
        if (index >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        return lines_[index];
    }
    
    size_t lineCount() const {
        return lines_.size();
    }
    
    bool isEmpty() const {
        return lines_.empty() || (lines_.size() == 1 && lines_[0].empty());
    }
    
    void insertLines(size_t index, const std::vector<std::string>& newLines) {
        if (index > lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        
        if (lines_.size() == 1 && lines_[0].empty()) {
            lines_.clear();
        }
        
        lines_.insert(lines_.begin() + index, newLines.begin(), newLines.end());
    }
    
    void deleteLines(size_t start, size_t end) {
        if (start >= lines_.size() || end > lines_.size() || start > end) {
            throw std::out_of_range("Invalid range for deleteLines");
        }
        
        lines_.erase(lines_.begin() + start, lines_.begin() + end);
        
        if (lines_.empty()) {
            lines_.push_back("");
        }
    }
};

void runTests() {
    std::cout << "=== Running Simple TextBuffer Tests ===" << std::endl;
    
    // Test 1: Empty buffer
    {
        std::cout << "Test 1: Empty buffer... ";
        TextBuffer buffer;
        if (!buffer.isEmpty()) throw std::runtime_error("New buffer should be empty");
        if (buffer.lineCount() != 1) throw std::runtime_error("Should have one empty line");
        if (buffer.getLine(0) != "") throw std::runtime_error("First line should be empty");
        std::cout << "PASSED" << std::endl;
    }
    
    // Test 2: Insert text
    {
        std::cout << "Test 2: Insert text... ";
        TextBuffer buffer;
        buffer.insertText(0, 0, "Hello");
        if (buffer.getLine(0) != "Hello") throw std::runtime_error("Insert text failed");
        
        buffer.insertText(0, 5, ", World!");
        if (buffer.getLine(0) != "Hello, World!") throw std::runtime_error("Append text failed");
        
        buffer.insertText(0, 5, " there");
        if (buffer.getLine(0) != "Hello there, World!") throw std::runtime_error("Insert in middle failed");
        
        std::cout << "PASSED" << std::endl;
    }
    
    // Test 3: Delete text
    {
        std::cout << "Test 3: Delete text... ";
        TextBuffer buffer;
        buffer.insertText(0, 0, "Hello, World!");
        
        buffer.deleteText(0, 5, 0, 7); // Delete ", "
        if (buffer.getLine(0) != "HelloWorld!") throw std::runtime_error("Delete from middle failed");
        
        buffer.deleteText(0, 0, 0, 5); // Delete "Hello"
        if (buffer.getLine(0) != "World!") throw std::runtime_error("Delete from start failed");
        
        buffer.deleteText(0, 5, 0, 6); // Delete "!"
        if (buffer.getLine(0) != "World") throw std::runtime_error("Delete from end failed");
        
        std::cout << "PASSED" << std::endl;
    }
    
    // Test 4: Multi-line operations
    {
        std::cout << "Test 4: Multi-line operations... ";
        TextBuffer buffer;
        
        std::vector<std::string> lines = {"Line 1", "Line 2", "Line 3"};
        buffer.insertLines(0, lines);
        
        if (buffer.lineCount() != 3) throw std::runtime_error("Incorrect line count after insert");
        if (buffer.getLine(0) != "Line 1") throw std::runtime_error("Line 1 content incorrect");
        if (buffer.getLine(1) != "Line 2") throw std::runtime_error("Line 2 content incorrect");
        if (buffer.getLine(2) != "Line 3") throw std::runtime_error("Line 3 content incorrect");
        
        buffer.deleteLines(1, 2); // Delete "Line 2"
        if (buffer.lineCount() != 2) throw std::runtime_error("Incorrect line count after delete");
        if (buffer.getLine(0) != "Line 1") throw std::runtime_error("Line 1 content incorrect after delete");
        if (buffer.getLine(1) != "Line 3") throw std::runtime_error("Line 3 content incorrect after delete");
        
        std::cout << "PASSED" << std::endl;
    }
    
    std::cout << "=== All Tests Passed ===" << std::endl;
}

int main() {
    try {
        runTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
