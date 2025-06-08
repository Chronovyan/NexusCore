#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <cstdio>
#include <stack>
#include <memory>
#include <utility>

// Base Command class for undo/redo operations
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

class TextBuffer {
    std::vector<std::string> lines_;
    bool modified_ = false;
    
    // Undo/Redo stacks
    std::stack<std::unique_ptr<Command>> undoStack_;
    std::stack<std::unique_ptr<Command>> redoStack_;
    
    // Helper method to execute a command and add to undo stack
    void executeCommand(std::unique_ptr<Command> cmd) {
        cmd->execute();
        undoStack_.push(std::move(cmd));
        // Clear redo stack when a new command is executed
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
        modified_ = true;
    }

    void ensureValidLine(size_t lineIndex) const {
        if (lineIndex >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
    }
    
    void ensureValidPosition(size_t lineIndex, size_t colIndex) const {
        ensureValidLine(lineIndex);
        if (colIndex > lines_[lineIndex].length()) {
            throw std::out_of_range("Column index out of range");
        }
    }

public:
    TextBuffer() {
        // Start with one empty line
        lines_.emplace_back("");
        modified_ = false;
    }

    // Command for clearing the buffer
    class ClearCommand : public Command {
        TextBuffer& buffer_;
        std::vector<std::string> oldLines_;
        
    public:
        ClearCommand(TextBuffer& buffer, bool keepEmptyLine)
            : buffer_(buffer), oldLines_(buffer.lines_) {
            buffer_.lines_.clear();
            if (keepEmptyLine) {
                buffer_.lines_.emplace_back("");
            }
        }
        
        void execute() override {
            // Already done in constructor
        }
        
        void undo() override {
            buffer_.lines_ = oldLines_;
        }
        
        std::string getDescription() const override {
            return "Clear buffer";
        }
    };
    
    void clear(bool keepEmptyLine = true) {
        if (lines_.empty() || (lines_.size() == 1 && lines_[0].empty())) {
            return; // Already empty
        }
        executeCommand(std::make_unique<ClearCommand>(*this, keepEmptyLine));
    }

    // Command for text insertion
    class InsertTextCommand : public Command {
        TextBuffer& buffer_;
        size_t line_;
        size_t col_;
        std::string text_;
        
    public:
        InsertTextCommand(TextBuffer& buffer, size_t line, size_t col, std::string text)
            : buffer_(buffer), line_(line), col_(col), text_(std::move(text)) {}
            
        void execute() override {
            buffer_.lines_[line_].insert(col_, text_);
        }
        
        void undo() override {
            buffer_.lines_[line_].erase(col_, text_.length());
        }
        
        std::string getDescription() const override {
            return "Insert text: " + text_.substr(0, 10) + (text_.length() > 10 ? "..." : "");
        }
    };
    
    void insertText(size_t line, size_t col, const std::string& text) {
        ensureValidPosition(line, col);
        executeCommand(std::make_unique<InsertTextCommand>(*this, line, col, text));
    }

    // Command for text deletion
    class DeleteTextCommand : public Command {
        TextBuffer& buffer_;
        size_t line_;
        size_t startCol_;
        size_t endCol_;
        std::string deletedText_;
        
    public:
        DeleteTextCommand(TextBuffer& buffer, size_t line, size_t startCol, size_t endCol)
            : buffer_(buffer), line_(line), startCol_(startCol), endCol_(endCol) {
            if (startCol_ > endCol_) std::swap(startCol_, endCol_);
            deletedText_ = buffer_.lines_[line_].substr(startCol_, endCol_ - startCol_);
        }
        
        void execute() override {
            buffer_.lines_[line_].erase(startCol_, endCol_ - startCol_);
        }
        
        void undo() override {
            buffer_.lines_[line_].insert(startCol_, deletedText_);
        }
        
        std::string getDescription() const override {
            return "Delete text: " + deletedText_.substr(0, 10) + (deletedText_.length() > 10 ? "..." : "");
        }
    };
    
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
        if (startLine != endLine) {
            throw std::runtime_error("Multi-line delete not implemented in this simplified version");
        }
        ensureValidPosition(startLine, startCol);
        ensureValidPosition(endLine, endCol);
        
        executeCommand(std::make_unique<DeleteTextCommand>(
            *this, startLine, startCol, endCol));
    }

    const std::string& getLine(size_t index) const {
        ensureValidLine(index);
        return lines_[index];
    }

    size_t lineCount() const {
        return lines_.size();
    }

    bool isEmpty() const {
        // Buffer is considered empty if it has one line that's empty
        return lines_.size() == 1 && lines_[0].empty();
    }

    // Command for inserting lines
    class InsertLinesCommand : public Command {
        TextBuffer& buffer_;
        size_t index_;
        std::vector<std::string> lines_;
        bool wasEmpty_;
        
    public:
        InsertLinesCommand(TextBuffer& buffer, size_t index, std::vector<std::string> lines)
            : buffer_(buffer), index_(index), lines_(std::move(lines)),
              wasEmpty_(buffer.lines_.size() == 1 && buffer.lines_[0].empty()) {}
        
        void execute() override {
            if (wasEmpty_) {
                buffer_.lines_.clear();
            }
            buffer_.lines_.insert(buffer_.lines_.begin() + index_, lines_.begin(), lines_.end());
        }
        
        void undo() override {
            buffer_.lines_.erase(
                buffer_.lines_.begin() + index_,
                buffer_.lines_.begin() + index_ + lines_.size());
            if (wasEmpty_) {
                buffer_.lines_.push_back("");
            }
        }
        
        std::string getDescription() const override {
            return "Insert " + std::to_string(lines_.size()) + " lines";
        }
    };
    
    void insertLines(size_t index, const std::vector<std::string>& newLines) {
        if (index > lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        
        if (newLines.empty()) return;
        
        executeCommand(std::make_unique<InsertLinesCommand>(
            *this, index, newLines));
    }

    // Command for deleting lines
    class DeleteLinesCommand : public Command {
        TextBuffer& buffer_;
        size_t start_;
        size_t end_;
        std::vector<std::string> deletedLines_;
        bool wasSingleLine_;
        
    public:
        DeleteLinesCommand(TextBuffer& buffer, size_t start, size_t end)
            : buffer_(buffer), start_(start), end_(end) {
            // Save the lines being deleted
            wasSingleLine_ = (buffer.lines_.size() == 1);
            deletedLines_.assign(
                buffer_.lines_.begin() + start_,
                buffer_.lines_.begin() + end_);
        }
        
        void execute() override {
            // Erase the specified range of lines
            buffer_.lines_.erase(
                buffer_.lines_.begin() + start_,
                buffer_.lines_.begin() + end_);
            
            // If we deleted all lines or ended up with no lines, ensure we have one empty line
            if (buffer_.lines_.empty() || (wasSingleLine_ && deletedLines_.size() == 1)) {
                buffer_.lines_.clear();
                buffer_.lines_.push_back("");
            }
        }
        
        void undo() override {
            // If we're undoing a delete that left us with an empty buffer, clear it first
            if (buffer_.lines_.size() == 1 && buffer_.lines_[0].empty()) {
                buffer_.lines_.clear();
            }
            
            // Insert the deleted lines back at their original position
            buffer_.lines_.insert(
                buffer_.lines_.begin() + start_,
                deletedLines_.begin(),
                deletedLines_.end());
        }
        
        std::string getDescription() const override {
            return "Delete " + std::to_string(end_ - start_) + " lines";
        }
    };
    
    void deleteLines(size_t start, size_t end) {
        if (start >= lines_.size() || end > lines_.size() || start > end) {
            throw std::out_of_range("Invalid range for deleteLines");
        }
        
        if (start == end) return; // No-op
        
        executeCommand(std::make_unique<DeleteLinesCommand>(
            *this, start, end));
    }
    
    bool isModified() const {
        return modified_;
    }
    
    void setModified(bool modified) {
        modified_ = modified;
    }
    
    // Undo/Redo operations
    bool canUndo() const {
        return !undoStack_.empty();
    }
    
    bool canRedo() const {
        return !redoStack_.empty();
    }
    
    std::string getUndoDescription() const {
        return canUndo() ? undoStack_.top()->getDescription() : "";
    }
    
    std::string getRedoDescription() const {
        return canRedo() ? redoStack_.top()->getDescription() : "";
    }
    
    bool undo() {
        if (!canUndo()) return false;
        
        auto cmd = std::move(undoStack_.top());
        undoStack_.pop();
        
        cmd->undo();
        redoStack_.push(std::move(cmd));
        
        modified_ = !undoStack_.empty();
        return true;
    }
    
    bool redo() {
        if (!canRedo()) return false;
        
        auto cmd = std::move(redoStack_.top());
        redoStack_.pop();
        
        cmd->execute();
        undoStack_.push(std::move(cmd));
        
        modified_ = true;
        return true;
    }
    
    // File I/O operations
    bool saveToFile(const std::string& filename) {
        std::ofstream outfile(filename);
        if (!outfile.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
            return false;
        }
        
        for (size_t i = 0; i < lines_.size(); ++i) {
            outfile << lines_[i];
            if (i != lines_.size() - 1) {
                outfile << '\n';  // Don't add newline after last line
            }
        }
        
        outfile.close();
        return !outfile.fail();
    }
    
    bool loadFromFile(const std::string& filename) {
        // Clear undo/redo stacks when loading from file
        while (!undoStack_.empty()) undoStack_.pop();
        while (!redoStack_.empty()) redoStack_.pop();
        std::ifstream infile(filename);
        if (!infile.is_open()) {
            std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
            return false;
        }
        
        lines_.clear();
        std::string line;
        while (std::getline(infile, line)) {
            lines_.push_back(line);
        }
        
        // Handle empty file or file with only one line
        if (lines_.empty()) {
            lines_.push_back("");
        }
        
        modified_ = false;
        return !infile.bad();
    }
};

void runTests() {
    std::cout << "=== Running Simplified TextBuffer Tests ===" << std::endl;
    
    // Test 1: Empty buffer
    {
        std::cout << "Test 1: Empty buffer... ";
        TextBuffer buffer;
        if (buffer.lineCount() != 1) throw std::runtime_error("New buffer should have one line");
        if (!buffer.getLine(0).empty()) throw std::runtime_error("First line should be empty");
        if (!buffer.isEmpty()) throw std::runtime_error("New buffer should be considered empty");
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
        
        // Reset buffer
        buffer.clear();
        buffer.insertText(0, 0, "Hello, World!");
        
        // Delete ", " (characters at positions 5 and 6)
        buffer.deleteText(0, 5, 0, 7);
        if (buffer.getLine(0) != "HelloWorld!") throw std::runtime_error("Delete from middle failed");
        
        // Delete "Hello" (characters 0-4)
        buffer.deleteText(0, 0, 0, 5);
        if (buffer.getLine(0) != "World!") throw std::runtime_error("Delete from start failed");
        
        // Delete "!" (last character)
        buffer.deleteText(0, 5, 0, 6);
        if (buffer.getLine(0) != "World") throw std::runtime_error("Delete from end failed");
        
        std::cout << "PASSED" << std::endl;
    }
    
    // Test 4: Multi-line operations (now using command pattern)
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
    
    // Test 5: Undo/Redo operations
    {
        std::cout << "Test 5: Undo/Redo operations... ";
        TextBuffer buffer;
        
        // Insert some text
        buffer.insertText(0, 0, "Hello");
        buffer.insertText(0, 5, ", World!");
        
        // Undo the last insertion
        if (!buffer.canUndo()) throw std::runtime_error("Should be able to undo");
        buffer.undo();
        if (buffer.getLine(0) != "Hello") throw std::runtime_error("Undo failed");
        
        // Redo the insertion
        if (!buffer.canRedo()) throw std::runtime_error("Should be able to redo");
        buffer.redo();
        if (buffer.getLine(0) != "Hello, World!") throw std::runtime_error("Redo failed");
        
        // Test multiple operations
        buffer.insertLines(1, {"Second line", "Third line"});
        buffer.deleteLines(1, 3); // Delete two lines (indexes 1 and 2)
        
        if (buffer.lineCount() != 1) {
            throw std::runtime_error("Line count should be 1 after delete");
        }
        
        // Undo delete (should restore the two deleted lines)
        buffer.undo();
        if (buffer.lineCount() != 3) {
            throw std::runtime_error("Line count should be 3 after undo delete");
        }
        
        // Undo insert (should remove the two inserted lines)
        buffer.undo();
        if (buffer.lineCount() != 1) {
            throw std::runtime_error("Line count should be 1 after undo insert");
        }
        
        // Redo insert (should add the two lines back)
        buffer.redo();
        
        // The second redo will be the delete operation, which should reduce the line count to 1
        buffer.redo();
        
        // After redoing the delete, we should have 1 line left
        if (buffer.lineCount() != 1) {
            throw std::runtime_error("Line count should be 1 after redo delete");
        }
        
        std::cout << "PASSED" << std::endl;
    }
    
    // Test 6: File I/O operations
    {
        std::cout << "Test 6: File I/O operations... ";
        const std::string testFilename = "test_file.txt";
        
        // Test saving to file
        {
            TextBuffer buffer;
            buffer.insertText(0, 0, "First line\nSecond line\nThird line");
            if (!buffer.saveToFile(testFilename)) {
                throw std::runtime_error("Failed to save buffer to file");
            }
        }
        
        // Test loading from file
        {
            TextBuffer buffer;
            if (!buffer.loadFromFile(testFilename)) {
                std::remove(testFilename.c_str()); // Clean up
                throw std::runtime_error("Failed to load buffer from file");
            }
            
            if (buffer.lineCount() != 3) throw std::runtime_error("Incorrect line count after loading");
            if (buffer.getLine(0) != "First line") throw std::runtime_error("First line content incorrect after loading");
            if (buffer.getLine(1) != "Second line") throw std::runtime_error("Second line content incorrect after loading");
            if (buffer.getLine(2) != "Third line") throw std::runtime_error("Third line content incorrect after loading");
            
            // Clean up
            std::remove(testFilename.c_str());
        }
        
        // Test empty file
        {
            TextBuffer buffer;
            buffer.clear();
            if (!buffer.saveToFile("empty_test.txt")) {
                throw std::runtime_error("Failed to save empty buffer to file");
            }
            
            TextBuffer buffer2;
            if (!buffer2.loadFromFile("empty_test.txt")) {
                std::remove("empty_test.txt");
                throw std::runtime_error("Failed to load empty file");
            }
            
            if (buffer2.lineCount() != 1 || !buffer2.getLine(0).empty()) {
                std::remove("empty_test.txt");
                throw std::runtime_error("Empty file not loaded correctly");
            }
            
            // Clean up
            std::remove("empty_test.txt");
        }
        
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
