#include "TextBuffer.h"
#include "EditorError.h" // Include EditorError.h for exception types
#include <stdexcept>     // For std::out_of_range (can be removed later)
#include <ostream>       // For std::ostream (needed for printToStream)
#include <iostream>      // For std::cout, std::endl
#include <fstream>       // For std::ofstream and std::ifstream

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
        throw TextBufferException("Index out of range for insertLine", EditorException::Severity::EDITOR_ERROR);
    }
    lines_.insert(lines_.begin() + index, line);
}

void TextBuffer::deleteLine(size_t index) {
    if (index >= lines_.size()) {
        throw TextBufferException("Index out of range for deleteLine", EditorException::Severity::EDITOR_ERROR);
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
        throw TextBufferException("Index out of range for replaceLine", EditorException::Severity::EDITOR_ERROR);
    }
    lines_[index] = newLine;
}

const std::string& TextBuffer::getLine(size_t index) const {
    if (index >= lines_.size()) {
        throw TextBufferException("Index out of range for getLine", EditorException::Severity::EDITOR_ERROR);
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
        ErrorReporter::logError("Could not open file for saving: " + filename);
        return false;
    }

    for (const auto& line : lines_) {
        outfile << line << '\n'; // Append newline, as typical for text files
    }

    if (outfile.fail()) {
        ErrorReporter::logError("Failed while writing to file: " + filename);
        outfile.close(); // Attempt to close even on failure
        return false;
    }

    outfile.close();
    return true;
}

bool TextBuffer::loadFromFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        ErrorReporter::logError("Could not open file for loading: " + filename);
        return false;
    }

    lines_.clear(); // Clear existing content before loading
    std::string current_line;
    while (std::getline(infile, current_line)) {
        lines_.push_back(current_line);
    }

    if (infile.bad()) { // I/O error during read
        ErrorReporter::logError("An I/O error occurred while reading file: " + filename);
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
        throw TextBufferException("Line index out of range for insertChar", EditorException::Severity::EDITOR_ERROR);
    }
    std::string& line = lines_[lineIndex];
    if (colIndex > line.length()) { // Allow inserting at the very end (colIndex == length)
        throw TextBufferException("Column index out of range for insertChar", EditorException::Severity::EDITOR_ERROR);
    }
    line.insert(colIndex, 1, ch);
}

void TextBuffer::deleteChar(size_t lineIndex, size_t colIndex) {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Line index out of range for deleteChar", EditorException::Severity::EDITOR_ERROR);
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
        throw TextBufferException("Line index out of range for deleteCharForward", EditorException::Severity::EDITOR_ERROR);
    }
    
    std::string& line = lines_[lineIndex];
    
    // Validate column index - only throw if we can't handle this operation
    // When colIndex >= line.length() we can still handle line joining with the next line
    // Only throw if colIndex > line.length() AND we're on the last line OR colIndex is unreasonably large
    if (colIndex > line.length() && (lineIndex == lines_.size() - 1 || colIndex > line.length() + 100)) {
        throw TextBufferException("Column index out of range for deleteCharForward", EditorException::Severity::EDITOR_ERROR);
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
        throw TextBufferException("Line index out of range for splitLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    std::string& line = lines_[lineIndex];
    
    if (colIndex > line.length()) {
        throw TextBufferException("Column index out of range for splitLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Extract the part of the line after the split point
    std::string newLine = line.substr(colIndex);
    // Keep only the part before the split point in the original line
    line.erase(colIndex);
    
    // Insert the new line after the current line
    lines_.insert(lines_.begin() + lineIndex + 1, newLine);
}

void TextBuffer::joinLines(size_t lineIndex) {
    if (lineIndex >= lines_.size() - 1) {
        throw TextBufferException("Cannot join last line with next line", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Append the next line to the current line
    lines_[lineIndex] += lines_[lineIndex + 1];
    // Remove the next line
    lines_.erase(lines_.begin() + lineIndex + 1);
}

void TextBuffer::insertString(size_t lineIndex, size_t colIndex, const std::string& text) {
    // ADD LOGGING FOR PARAMETERS

    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for insertString (lineIndex)", EditorException::Severity::EDITOR_ERROR);
    }
    if (colIndex > lines_[lineIndex].length()) {
        // The column index is out of range - this is the check that's failing
        throw TextBufferException("Index out of range for insertString (colIndex)", EditorException::Severity::EDITOR_ERROR);
    }

    size_t currentPosInInputText = 0; 
    size_t lastNewlinePosInInputText = std::string::npos;

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') {

            std::string textAfterNewline = lines_[lineIndex].substr(colIndex + currentPosInInputText);
            lines_[lineIndex].erase(colIndex + currentPosInInputText);
            
            lines_[lineIndex] += text.substr(lastNewlinePosInInputText == std::string::npos ? 0 : lastNewlinePosInInputText + 1, i - (lastNewlinePosInInputText == std::string::npos ? 0 : lastNewlinePosInInputText + 1));
            
            lineIndex++; 
            lines_.insert(lines_.begin() + lineIndex, textAfterNewline); 
            

            colIndex = 0; 
            currentPosInInputText = 0; 
            lastNewlinePosInInputText = i;
        } else {
            currentPosInInputText++;
        }
    }
    
    std::string remainingTextToInsert = text.substr(lastNewlinePosInInputText == std::string::npos ? 0 : lastNewlinePosInInputText + 1);
    
    if (!remainingTextToInsert.empty()) {
        if (lineIndex >= lines_.size()) { 
             throw TextBufferException("Index out of range for insertString final (lineIndex)", EditorException::Severity::EDITOR_ERROR);
        }
        if (colIndex > lines_[lineIndex].length()){ 
            throw TextBufferException("Index out of range for insertString final (colIndex)", EditorException::Severity::EDITOR_ERROR);
        }

        
        lines_[lineIndex].insert(colIndex, remainingTextToInsert); 
        
    } else {
    }
}

std::string TextBuffer::getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for getLineSegment (lineIndex)", EditorException::Severity::EDITOR_ERROR);
    }
    
    const std::string& line = lines_[lineIndex];
    
    // Validate column indices
    if (startCol > endCol || startCol > line.length()) {
        throw TextBufferException("Invalid column range for getLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Clamp endCol to line length
    endCol = std::min(endCol, line.length());
    
    // Return the segment (substr handles the case of zero-length segment correctly)
    return line.substr(startCol, endCol - startCol);
}

size_t TextBuffer::lineLength(size_t lineIndex) const {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for lineLength", EditorException::Severity::EDITOR_ERROR);
    }
    return lines_[lineIndex].length();
}

std::string& TextBuffer::getLine(size_t lineIndex) {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for getLine (non-const)", EditorException::Severity::EDITOR_ERROR);
    }
    
    return lines_[lineIndex];
}

void TextBuffer::setLine(size_t lineIndex, const std::string& text) {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for setLine", EditorException::Severity::EDITOR_ERROR);
    }
    
    lines_[lineIndex] = text;
}

// New method: Get total character count in the buffer
size_t TextBuffer::characterCount() const {
    size_t count = 0;
    for (const auto& line : lines_) {
        count += line.length();
    }
    return count;
}

// New method: Get a copy of all lines in the buffer
std::vector<std::string> TextBuffer::getAllLines() const {
    // Special case: if there's only one line and it's empty, return an empty vector
    if (lines_.size() == 1 && lines_[0].empty()) {
        return std::vector<std::string>();
    }
    return lines_; // Returns a copy of the lines vector
}

// New method: Replace a segment of text within a line
void TextBuffer::replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText) {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for replaceLineSegment (lineIndex)", EditorException::Severity::EDITOR_ERROR);
    }
    std::string& line = lines_[lineIndex];
    if (startCol > line.length() || endCol > line.length()) {
        throw TextBufferException("Column index out of range for replaceLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    if (startCol > endCol) {
        throw TextBufferException("Start column cannot be greater than end column for replaceLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    line.replace(startCol, endCol - startCol, newText);
}

// New method: Delete a segment of text within a line
void TextBuffer::deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol) {
    if (lineIndex >= lines_.size()) {
        throw TextBufferException("Index out of range for deleteLineSegment (lineIndex)", EditorException::Severity::EDITOR_ERROR);
    }
    std::string& line = lines_[lineIndex];
    if (startCol > line.length() || endCol > line.length()) {
        throw TextBufferException("Column index out of range for deleteLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    if (startCol > endCol) {
        throw TextBufferException("Start column cannot be greater than end column for deleteLineSegment", EditorException::Severity::EDITOR_ERROR);
    }
    line.erase(startCol, endCol - startCol);
}

// Optional: Implementation for a friend ostream operator if you prefer `std::cout << buffer;`
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer) {
//     buffer.printToStream(os);
//     return os;
// }

// New method: Delete multiple lines from startIndex (inclusive) to endIndex (exclusive)
void TextBuffer::deleteLines(size_t startIndex, size_t endIndex) {
    if (startIndex >= lines_.size() || endIndex >= lines_.size() || startIndex > endIndex) {
        throw TextBufferException("Invalid range for deleteLines", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Special handling if deleting all lines means we need to keep one empty line
    if (startIndex == endIndex) {
        if (lines_.empty()) {
            lines_.emplace_back(""); // Add an empty line
        }
        return;
    }
    
    // Clamp endIndex to prevent out-of-bounds access
    endIndex = std::min(endIndex, lines_.size());
    
    // Delete the lines
    lines_.erase(lines_.begin() + startIndex, lines_.begin() + endIndex);
    
    // Ensure buffer is never completely empty (consistent with clear(true) behavior)
    if (lines_.empty()) {
        lines_.emplace_back(""); // Add an empty line
    }
}

// New method: Insert multiple lines at the specified index
void TextBuffer::insertLines(size_t index, const std::vector<std::string>& newLines) {
    if (index > lines_.size()) {
        throw TextBufferException("Index out of range for insertLines", EditorException::Severity::EDITOR_ERROR);
    }
    lines_.insert(lines_.begin() + index, newLines.begin(), newLines.end());
}

// New method: Check if the position is valid within the buffer
bool TextBuffer::isValidPosition(size_t lineIndex, size_t colIndex) const {
    // Empty buffer has no valid positions
    if (lines_.empty()) {
        return false;
    }
    
    // Check line index
    if (lineIndex >= lines_.size()) {
        return false;
    }
    
    // Check column index (can be at the end of the line, hence <=)
    return colIndex <= lines_[lineIndex].length();
}

// New method: Clamp a position to be within valid bounds of the buffer
std::pair<size_t, size_t> TextBuffer::clampPosition(size_t lineIndex, size_t colIndex) const {
    // Handle empty buffer case
    if (lines_.empty()) {
        return {0, 0};
    }
    
    // Clamp line index
    lineIndex = std::min(lineIndex, lines_.size() - 1);
    
    // Clamp column index
    colIndex = std::min(colIndex, lines_[lineIndex].length());
    
    return {lineIndex, colIndex};
}

// Implementation of additional ITextBuffer interface methods
size_t TextBuffer::getLineCount() const {
    return lines_.size(); // Identical to lineCount()
}

std::vector<std::string> TextBuffer::getLines() const {
    return lines_; // Return a copy of all lines
}

void TextBuffer::replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text) {
    if (startLine >= lines_.size() || endLine >= lines_.size()) {
        throw TextBufferException("Line index out of range for replaceText", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (startLine == endLine) {
        // Single line replacement
        replaceLineSegment(startLine, startCol, endCol, text);
    } else {
        // Multi-line replacement
        // Store text after endCol in the last line
        std::string endLineRemainder = "";
        if (endCol < lines_[endLine].length()) {
            endLineRemainder = lines_[endLine].substr(endCol);
        }
        
        // Keep text before startCol in the first line
        std::string startLinePrefix = "";
        if (startCol > 0) {
            startLinePrefix = lines_[startLine].substr(0, startCol);
        }
        
        // Delete all lines between startLine+1 and endLine (inclusive)
        for (size_t i = endLine; i > startLine; --i) {
            lines_.erase(lines_.begin() + i);
        }
        
        // Replace the content of the first line
        lines_[startLine] = startLinePrefix + text + endLineRemainder;
        
        modified_ = true;
    }
}

void TextBuffer::insertText(size_t line, size_t col, const std::string& text) {
    if (line >= lines_.size()) {
        throw TextBufferException("Invalid line index for insertText", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (col > lines_[line].length()) {
        throw TextBufferException("Invalid column index for insertText", EditorException::Severity::EDITOR_ERROR);
    }
    
    // Check if text contains newlines
    size_t newlinePos = text.find('\n');
    if (newlinePos == std::string::npos) {
        // Simple case: no newlines, just insert the text
        lines_[line].insert(col, text);
    } else {
        // Text contains newlines, need to split it
        insertString(line, col, text);
    }
    
    modified_ = true;
}

void TextBuffer::deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
    if (startLine >= lines_.size() || endLine >= lines_.size()) {
        throw TextBufferException("Line index out of range for deleteText", EditorException::Severity::EDITOR_ERROR);
    }
    
    if (startLine == endLine) {
        // Single line deletion
        deleteLineSegment(startLine, startCol, endCol);
    } else {
        // Multi-line deletion
        // Keep text before startCol in the first line
        std::string startLinePrefix = "";
        if (startCol > 0) {
            startLinePrefix = lines_[startLine].substr(0, startCol);
        }
        
        // Keep text after endCol in the last line
        std::string endLineSuffix = "";
        if (endCol < lines_[endLine].length()) {
            endLineSuffix = lines_[endLine].substr(endCol);
        }
        
        // Combine the remaining parts
        lines_[startLine] = startLinePrefix + endLineSuffix;
        
        // Delete all lines between startLine+1 and endLine (inclusive)
        for (size_t i = endLine; i > startLine; --i) {
            lines_.erase(lines_.begin() + i);
        }
    }
    
    modified_ = true;
}

bool TextBuffer::isModified() const {
    return modified_;
}

void TextBuffer::setModified(bool modified) {
    modified_ = modified;
}

// Thread ownership methods
void TextBuffer::setOwnerThread(const std::thread::id& threadId) {
    ownerThreadId_ = threadId;
}

size_t TextBuffer::processOperationQueue() {
    // This is a stub implementation since we don't have the actual operation queue
    // In a real implementation, this would process pending operations from a queue
    // For now, we'll just return 0 to indicate no operations were processed
    return 0;
} 