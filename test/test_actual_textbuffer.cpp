#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

// Include the actual TextBuffer implementation
#include "../src/TextBuffer.h"

// Simple ErrorReporter stub
namespace ai_editor {
class ErrorReporter {
public:
    enum class Severity {
        DEBUG, INFO, WARNING, ERROR, CRITICAL
    };
    
    static ErrorReporter& getInstance() {
        static ErrorReporter instance;
        return instance;
    }
    
    void report(Severity severity, const std::string& message, const std::string& source = "") {
        const char* level = "UNKNOWN";
        switch (severity) {
            case Severity::DEBUG: level = "DEBUG"; break;
            case Severity::INFO: level = "INFO"; break;
            case Severity::WARNING: level = "WARNING"; break;
            case Severity::ERROR: level = "ERROR"; break;
            case Severity::CRITICAL: level = "CRITICAL"; break;
        }
        std::cerr << "[" << level << "] " << source << ": " << message << std::endl;
    }
    
    static void logError(const std::string& message) {
        getInstance().report(Severity::ERROR, message, "TextBuffer");
    }
};
} // namespace ai_editor

// Simple EditorException stub
class EditorException : public std::exception {
    std::string message_;
    int severity_;
public:
    enum Severity {
        EDITOR_ERROR = 0,
        EDITOR_WARNING = 1,
        EDITOR_INFO = 2
    };
    
    EditorException(const std::string& message, int severity = EDITOR_ERROR)
        : message_(message), severity_(severity) {}
        
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    int getSeverity() const { return severity_; }
};

// TextBufferException stub
class TextBufferException : public EditorException {
public:
    TextBufferException(const std::string& message, int severity = EditorException::EDITOR_ERROR)
        : EditorException(message, severity) {}
};

// Test function declarations
void runTextBufferTests();
void testEmptyBuffer();
void testInsertText();
void testDeleteText();
void testMultiLineOperations();

int main() {
    try {
        runTextBufferTests();
        std::cout << "All TextBuffer tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}

void runTextBufferTests() {
    std::cout << "=== Running TextBuffer Tests ===" << std::endl;
    
    testEmptyBuffer();
    testInsertText();
    testDeleteText();
    testMultiLineOperations();
    
    std::cout << "=== All TextBuffer Tests Passed ===" << std::endl;
}

void testEmptyBuffer() {
    std::cout << "Test: Empty buffer... ";
    TextBuffer buffer;
    
    if (!buffer.isEmpty()) throw std::runtime_error("New buffer should be empty");
    if (buffer.lineCount() != 1) throw std::runtime_error("New buffer should have one empty line");
    if (buffer.getLine(0) != "") throw std::runtime_error("First line should be empty");
    
    std::cout << "PASSED" << std::endl;
}

void testInsertText() {
    std::cout << "Test: Insert text... ";
    TextBuffer buffer;
    
    // Test basic text insertion
    buffer.insertText(0, 0, "Hello");
    if (buffer.getLine(0) != "Hello") {
        throw std::runtime_error("Failed to insert text at beginning of line");
    }
    
    // Test appending text
    buffer.insertText(0, 5, ", World!");
    if (buffer.getLine(0) != "Hello, World!") {
        throw std::runtime_error("Failed to append text to line");
    }
    
    // Test inserting in the middle
    buffer.insertText(0, 5, " there");
    if (buffer.getLine(0) != "Hello there, World!") {
        throw std::runtime_error("Failed to insert text in the middle of line");
    }
    
    std::cout << "PASSED" << std::endl;
}

void testDeleteText() {
    std::cout << "Test: Delete text... ";
    TextBuffer buffer;
    buffer.insertText(0, 0, "Hello, World!");
    
    // Delete from middle
    buffer.deleteText(0, 5, 0, 7); // Delete ", "
    if (buffer.getLine(0) != "HelloWorld!") {
        throw std::runtime_error("Failed to delete text from middle of line");
    }
    
    // Delete from start
    buffer.deleteText(0, 0, 0, 5); // Delete "Hello"
    if (buffer.getLine(0) != "World!") {
        throw std::runtime_error("Failed to delete text from start of line");
    }
    
    // Delete from end
    buffer.deleteText(0, 5, 0, 6); // Delete "!"
    if (buffer.getLine(0) != "World") {
        throw std::runtime_error("Failed to delete text from end of line");
    }
    
    std::cout << "PASSED" << std::endl;
}

void testMultiLineOperations() {
    std::cout << "Test: Multi-line operations... ";
    TextBuffer buffer;
    
    // Test inserting multiple lines
    std::vector<std::string> lines = {"Line 1", "Line 2", "Line 3"};
    buffer.insertLines(0, lines);
    
    if (buffer.lineCount() != 3) {
        throw std::runtime_error("Incorrect number of lines after insertLines");
    }
    
    if (buffer.getLine(0) != "Line 1" || 
        buffer.getLine(1) != "Line 2" || 
        buffer.getLine(2) != "Line 3") {
        throw std::runtime_error("Incorrect line content after insertLines");
    }
    
    // Test deleting lines
    buffer.deleteLines(1, 2); // Delete "Line 2"
    if (buffer.lineCount() != 2 || buffer.getLine(1) != "Line 3") {
        throw std::runtime_error("Failed to delete line");
    }
    
    std::cout << "PASSED" << std::endl;
}
