#include "TextBuffer.h"
#include <stdexcept> // For std::out_of_range
#include <ostream>   // For std::ostream (needed for printToStream)
#include <iostream>  // For std::cout, std::endl (if used directly here for debugging, though printToStream is better)
#include <fstream>   // For std::ofstream and std::ifstream

TextBuffer::TextBuffer() {
    clear(true); // Start with one empty line
}

void TextBuffer::clear(bool keepEmptyLine) {
    lines_.clear();
    if (keepEmptyLine) {
        lines_.emplace_back(""); // Add empty line only if requested
    }
}

void TextBuffer::addLine(const std::string& line) {
    lines_.push_back(line);
}

void TextBuffer::insertLine(size_t index, const std::string& line) {
    if (index > lines_.size()) {
        throw std::out_of_range("Index out of range for insertLine");
    }
    lines_.insert(lines_.begin() + index, line);
}

void TextBuffer::deleteLine(size_t index) {
    if (index >= lines_.size()) {
        throw std::out_of_range("Index out of range for deleteLine");
    }
    
    if (lines_.size() == 1 && index == 0) { // If it's the only line and we're deleting it
        lines_[0] = ""; // Make it an empty string
        // Do not erase the line itself, ensuring the buffer still has one line.
    } else {
        lines_.erase(lines_.begin() + index);
    }
}

void TextBuffer::replaceLine(size_t index, const std::string& newLine) {
    if (index >= lines_.size()) {
        throw std::out_of_range("Index out of range for replaceLine");
    }
    lines_[index] = newLine;
}

const std::string& TextBuffer::getLine(size_t index) const {
    if (index >= lines_.size()) {
        throw std::out_of_range("Index out of range for getLine");
    }
    return lines_[index];
}

size_t TextBuffer::lineCount() const {
    return lines_.size();
}

bool TextBuffer::isEmpty() const {
    return lines_.empty();
}

void TextBuffer::printToStream(std::ostream& os) const {
    for (size_t i = 0; i < lines_.size(); ++i) {
        os << lines_[i];
        // Add newline character unless it's the very last line of the buffer 
        // and we don't want an extra trailing newline printed by the stream operation itself.
        // However, typically text files end with a newline for each line.
        // For simplicity now, always add a newline. This might be refined later.
        os << '\n'; 
    }
}

// File operations
bool TextBuffer::saveToFile(const std::string& filename) const {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        // Optional: Could throw an exception or return a more detailed error object.
        // For now, printing to std::cerr and returning false.
        std::cerr << "Error: Could not open file for saving: " << filename << std::endl;
        return false;
    }

    for (const auto& line : lines_) {
        outfile << line << '\n'; // Append newline, as typical for text files
    }

    if (outfile.fail()) {
        std::cerr << "Error: Failed while writing to file: " << filename << std::endl;
        outfile.close(); // Attempt to close even on failure
        return false;
    }

    outfile.close();
    return true;
}

bool TextBuffer::loadFromFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file for loading: " << filename << std::endl;
        return false;
    }

    lines_.clear(); // Clear existing content before loading
    std::string current_line;
    while (std::getline(infile, current_line)) {
        lines_.push_back(current_line);
    }

    if (infile.bad()) { // I/O error during read
        std::cerr << "Error: An I/O error occurred while reading file: " << filename << std::endl;
        infile.close();
        // lines_.clear(); // Optionally clear partially loaded data on error
        return false;
    }
    // Note: infile.eof() is expected if read to end. infile.fail() might be set if getline fails for other reasons
    // but eof is not an error in itself for reading. We've read all we could.

    infile.close();
    return true;
}

// Character level operations
void TextBuffer::insertChar(size_t lineIndex, size_t colIndex, char ch) {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for insertChar");
    }
    std::string& line = lines_[lineIndex];
    if (colIndex > line.length()) { // Allow inserting at the very end (colIndex == length)
        throw std::out_of_range("Column index out of range for insertChar");
    }
    line.insert(colIndex, 1, ch);
}

void TextBuffer::deleteChar(size_t lineIndex, size_t colIndex) {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for deleteChar");
    }
    
    std::string& line = lines_[lineIndex];
    
    if (colIndex == 0) {
        // Backspace at start of line - join with previous line if possible
        if (lineIndex > 0) {
            // Join current line with previous line
            lines_[lineIndex - 1] += line;
            lines_.erase(lines_.begin() + lineIndex);
            // Caller might need to adjust cursor to previous line, last column
        }
    } else if (colIndex <= line.length()) {
        // Normal backspace within a line - delete the character at colIndex-1
        // Only erase if there's something to erase and we're not at position 0
        if (colIndex > 0 && colIndex <= line.length() && line.length() > 0) {
            // Position colIndex refers to the cursor position, which is AFTER the character
            // to be deleted by backspace. So we delete the character at colIndex-1.
            line.erase(colIndex - 1, 1);
        }
    } else {
        // If colIndex is beyond line length, treat as backspace at the end of the line
        if (line.length() > 0) {
            line.erase(line.length() - 1, 1);
        }
    }
}

void TextBuffer::deleteCharForward(size_t lineIndex, size_t colIndex) {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for deleteCharForward");
    }
    
    std::string& line = lines_[lineIndex];
    
    // Limit colIndex to line.length() for safety
    if (colIndex > line.length()) {
        colIndex = line.length();  // Clamp to end of line
    }
    
    if (colIndex < line.length()) {
        // Normal delete within a line - delete the character AT the cursor position
        // This is different from backspace which deletes the character BEFORE the cursor
        line.erase(colIndex, 1);
    } else if (lineIndex < lines_.size() - 1) {
        // Delete at end of line - join with next line
        // This happens when cursor is at the very end of a line and Delete is pressed
        line += lines_[lineIndex + 1];
        lines_.erase(lines_.begin() + lineIndex + 1);
    }
    // If we're at the end of the last line, do nothing
}

void TextBuffer::splitLine(size_t lineIndex, size_t colIndex) {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for splitLine");
    }
    
    std::string& line = lines_[lineIndex];
    
    if (colIndex > line.length()) {
        throw std::out_of_range("Column index out of range for splitLine");
    }
    
    // Extract the part of the line after the split point
    std::string newLine = line.substr(colIndex);
    // Trim the current line at the split point
    line.erase(colIndex);
    
    // Insert the new line after the current one
    insertLine(lineIndex + 1, newLine);
}

void TextBuffer::joinLines(size_t lineIndex) {
    if (lineIndex >= lines_.size() - 1) {
        // Nothing to join if this is the last line
        throw std::out_of_range("Cannot join last line with next line");
    }
    
    lines_[lineIndex] += lines_[lineIndex + 1];
    lines_.erase(lines_.begin() + lineIndex + 1);
}

void TextBuffer::insertString(size_t lineIndex, size_t colIndex, const std::string& text) {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for insertString");
    }
    
    std::string& line = lines_[lineIndex];
    
    if (colIndex > line.length()) {
        colIndex = line.length();  // Limit to end of line
    }
    
    line.insert(colIndex, text);
}

std::string TextBuffer::getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for getLineSegment");
    }
    
    const std::string& line = lines_[lineIndex];
    
    if (startCol > line.length() || endCol > line.length() || startCol > endCol) {
        throw std::out_of_range("Invalid column range for getLineSegment");
    }
    
    return line.substr(startCol, endCol - startCol);
}

size_t TextBuffer::lineLength(size_t lineIndex) const {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("Line index out of range for lineLength");
    }
    
    return lines_[lineIndex].length();
}

std::string& TextBuffer::getLine(size_t lineIndex) {
    if (lineIndex >= lines_.size()) {
        throw std::out_of_range("TextBuffer::getLine non-const: lineIndex out of bounds");
    }
    return lines_[lineIndex];
}

void TextBuffer::setLine(size_t lineIndex, const std::string& text) {
    if (lineIndex < lines_.size()) {
        lines_[lineIndex] = text;
    } else {
        // Or throw std::out_of_range or handle error appropriately
        // For now, if lineIndex is out of bounds, this does nothing.
        // Consider adding lines if lineIndex == lines_.size()
    }
}

// Optional: Implementation for a friend ostream operator if you prefer `std::cout << buffer;`
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer) {
//     buffer.printToStream(os);
//     return os;
// } 