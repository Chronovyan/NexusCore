#pragma once

#include <string>
#include <vector>
#include <utility>
#include <iosfwd>
#include <stdexcept>
#include <memory>

/**
 * @brief A simplified text buffer interface for standalone testing
 * 
 * This interface includes only the essential methods needed for testing
 * text buffer implementations without dependencies on the full editor codebase.
 */
class ISimpleTextBuffer {
public:
    virtual ~ISimpleTextBuffer() = default;

    // Basic operations
    virtual void addLine(const std::string& line) = 0;
    virtual void insertLine(size_t index, const std::string& line) = 0;
    virtual void deleteLine(size_t index) = 0;
    virtual void replaceLine(size_t index, const std::string& newLine) = 0;
    
    // Accessors
    virtual const std::string& getLine(size_t index) const = 0;
    virtual std::string& getLine(size_t index) = 0;
    virtual size_t lineCount() const = 0;
    virtual bool isEmpty() const = 0;
    
    // Basic modifications
    virtual void clear(bool keepEmptyLine = false) = 0;
    virtual void insertString(size_t lineIndex, size_t colIndex, const std::string& text) = 0;
    virtual void insertChar(size_t lineIndex, size_t colIndex, char ch) = 0;
    virtual void deleteChar(size_t lineIndex, size_t colIndex) = 0;
};

/**
 * @class SimpleTextBuffer
 * @brief A simplified implementation of a text buffer for standalone testing
 * 
 * This class provides a basic text buffer implementation that can be used
 * in unit tests without depending on the full TextBuffer implementation
 * with all its dependencies.
 */
class SimpleTextBuffer : public ISimpleTextBuffer {
public:
    SimpleTextBuffer() {
        clear(true); // Start with one empty line
    }

    void addLine(const std::string& line) override {
        lines_.push_back(line);
    }

    void insertLine(size_t index, const std::string& line) override {
        if (index > lines_.size()) {
            throw std::out_of_range("Index out of range in SimpleTextBuffer::insertLine");
        }
        lines_.insert(lines_.begin() + index, line);
    }

    void deleteLine(size_t index) override {
        if (index >= lines_.size()) {
            throw std::out_of_range("Index out of range in SimpleTextBuffer::deleteLine");
        }
        
        if (lines_.size() == 1 && index == 0) {
            lines_[0] = ""; // Keep at least one line, but make it empty
        } else {
            lines_.erase(lines_.begin() + index);
        }
    }

    void replaceLine(size_t index, const std::string& newLine) override {
        if (index >= lines_.size()) {
            throw std::out_of_range("Index out of range in SimpleTextBuffer::replaceLine");
        }
        lines_[index] = newLine;
    }

    const std::string& getLine(size_t index) const override {
        if (index >= lines_.size()) {
            throw std::out_of_range("Index out of range in SimpleTextBuffer::getLine");
        }
        return lines_[index];
    }

    std::string& getLine(size_t index) override {
        if (index >= lines_.size()) {
            throw std::out_of_range("Index out of range in SimpleTextBuffer::getLine");
        }
        return lines_[index];
    }

    size_t lineCount() const override {
        return lines_.size();
    }

    bool isEmpty() const override {
        return lines_.empty() || (lines_.size() == 1 && lines_[0].empty());
    }

    void clear(bool keepEmptyLine) override {
        lines_.clear();
        if (keepEmptyLine) {
            lines_.emplace_back("");
        }
    }

    void insertString(size_t lineIndex, size_t colIndex, const std::string& text) override {
        if (lineIndex >= lines_.size()) {
            throw std::out_of_range("Line index out of range in SimpleTextBuffer::insertString");
        }
        
        if (colIndex > lines_[lineIndex].length()) {
            throw std::out_of_range("Column index out of range in SimpleTextBuffer::insertString");
        }
        
        // Check if text contains newlines
        size_t newlinePos = text.find('\n');
        if (newlinePos == std::string::npos) {
            // Simple case: no newlines, just insert the text at the specified position
            lines_[lineIndex].insert(colIndex, text);
        } else {
            // Split the text at newlines and insert multiple lines
            std::vector<std::string> newLines;
            
            // Get the part of the original line before the insertion point
            std::string beforeText = lines_[lineIndex].substr(0, colIndex);
            
            // Get the part of the original line after the insertion point
            std::string afterText = lines_[lineIndex].substr(colIndex);
            
            // Start processing the text to insert
            size_t startPos = 0;
            
            // Find all newlines and create the new lines
            while (newlinePos != std::string::npos) {
                // Create a line from the text up to the newline
                std::string newLine = text.substr(startPos, newlinePos - startPos);
                
                // If this is the first segment, append it to the text before the insertion point
                if (startPos == 0) {
                    newLines.push_back(beforeText + newLine);
                } else {
                    newLines.push_back(newLine);
                }
                
                // Move past the newline
                startPos = newlinePos + 1;
                
                // Find the next newline
                newlinePos = text.find('\n', startPos);
            }
            
            // Add the last segment with the remaining text after the insertion point
            newLines.push_back(text.substr(startPos) + afterText);
            
            // Replace the original line with the first new line
            lines_[lineIndex] = newLines[0];
            
            // Insert the rest of the new lines after the original line
            lines_.insert(lines_.begin() + lineIndex + 1, newLines.begin() + 1, newLines.end());
        }
    }

    void insertChar(size_t lineIndex, size_t colIndex, char ch) override {
        if (lineIndex >= lines_.size()) {
            throw std::out_of_range("Line index out of range in SimpleTextBuffer::insertChar");
        }
        
        if (colIndex > lines_[lineIndex].length()) {
            throw std::out_of_range("Column index out of range in SimpleTextBuffer::insertChar");
        }
        
        lines_[lineIndex].insert(colIndex, 1, ch);
    }

    void deleteChar(size_t lineIndex, size_t colIndex) override {
        // Backspace - delete character before cursor
        if (lineIndex >= lines_.size()) {
            throw std::out_of_range("Line index out of range in SimpleTextBuffer::deleteChar");
        }
        
        // If at beginning of a line (except first line), join with previous line
        if (colIndex == 0 && lineIndex > 0) {
            // Join with previous line directly
            lines_[lineIndex - 1] += lines_[lineIndex];
            lines_.erase(lines_.begin() + lineIndex);
        }
        // If within a line
        else if (colIndex > 0 && colIndex <= lines_[lineIndex].length()) {
            lines_[lineIndex].erase(colIndex - 1, 1);
        }
    }

private:
    std::vector<std::string> lines_;
}; 