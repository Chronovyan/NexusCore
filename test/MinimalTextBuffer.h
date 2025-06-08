#ifndef MINIMAL_TEXT_BUFFER_H
#define MINIMAL_TEXT_BUFFER_H

#include <vector>
#include <string>
#include <stdexcept>

class MinimalTextBuffer {
public:
    MinimalTextBuffer() {
        lines_.push_back(""); // Start with one empty line
    }

    void insertText(size_t line, size_t col, const std::string& text) {
        if (line >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        
        std::string& currentLine = lines_[line];
        if (col > currentLine.length()) {
            col = currentLine.length(); // Clamp to end of line
        }
        
        currentLine.insert(col, text);
    }
    
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
        if (startLine >= lines_.size() || endLine >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        
        if (startLine == endLine) {
            // Simple case: deleting within a single line
            std::string& line = lines_[startLine];
            if (startCol < line.length() && endCol <= line.length()) {
                line.erase(startCol, endCol - startCol);
            }
        } else {
            // Handle multi-line deletion if needed
            // For simplicity, we'll just implement single-line deletion for now
            throw std::runtime_error("Multi-line deletion not implemented");
        }
    }
    
    const std::string& getLine(size_t index) const {
        if (index >= lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        return lines_[index];
    }
    
    size_t getLineCount() const {
        return lines_.size();
    }
    
    bool isEmpty() const {
        return lines_.empty() || (lines_.size() == 1 && lines_[0].empty());
    }
    
    void insertLines(size_t index, const std::vector<std::string>& newLines) {
        if (index > lines_.size()) {
            throw std::out_of_range("Line index out of range");
        }
        // If buffer is empty, clear it first
        if (lines_.size() == 1 && lines_[0].empty()) {
            lines_.clear();
        }
        lines_.insert(lines_.begin() + index, newLines.begin(), newLines.end());
    }
    
    void clear(bool keepEmptyLine = true) {
        lines_.clear();
        if (keepEmptyLine) {
            lines_.push_back("");
        }
    }
    
private:
    std::vector<std::string> lines_;
};

#endif // MINIMAL_TEXT_BUFFER_H
