#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include <vector>
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <utility> // For std::pair
#include "interfaces/ITextBuffer.hpp"

// Forward declaration for a friend function if needed later for direct stream output
// class TextBuffer;
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

class TextBuffer : public ITextBuffer {
public:
    TextBuffer();

    // Basic operations
    void addLine(const std::string& line) override;
    void insertLine(size_t index, const std::string& line) override;
    void deleteLine(size_t index) override;
    void replaceLine(size_t index, const std::string& newLine) override;
    void setLine(size_t lineIndex, const std::string& text) override;
    
    // Multi-line operations
    void deleteLines(size_t startIndex, size_t endIndex) override;
    void insertLines(size_t index, const std::vector<std::string>& newLines) override;

    // Accessors
    const std::string& getLine(size_t index) const override;
    std::string& getLine(size_t index) override; // Non-const version
    size_t lineCount() const override;
    bool isEmpty() const override;

    // Additional buffer content information methods
    size_t lineLength(size_t lineIndex) const override;
    size_t characterCount() const override;
    std::vector<std::string> getAllLines() const override;
    
    // Safety improvements
    bool isValidPosition(size_t lineIndex, size_t colIndex) const override;
    std::pair<size_t, size_t> clampPosition(size_t lineIndex, size_t colIndex) const override;

    // For displaying the buffer content
    void printToStream(std::ostream& os) const override;

    // File operations
    bool saveToFile(const std::string& filename) const override;
    bool loadFromFile(const std::string& filename) override;

    // Character level operations
    void insertChar(size_t lineIndex, size_t colIndex, char ch) override;
    void deleteChar(size_t lineIndex, size_t colIndex) override; // For backspace
    void deleteCharForward(size_t lineIndex, size_t colIndex) override; // For delete key

    // Enhanced string manipulation
    void replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText) override;
    void deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol) override;

    // Line manipulation
    void splitLine(size_t lineIndex, size_t colIndex) override; // For Enter key
    void joinLines(size_t lineIndex) override; // Join with next line
    void clear(bool keepEmptyLine) override;

    // Allow direct string manipulation for access to internal structure
    void insertString(size_t lineIndex, size_t colIndex, const std::string& text) override;
    std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const override;

    // Additional ITextBuffer interface methods
    size_t getLineCount() const override;
    std::vector<std::string> getLines() const override;
    void replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text) override;
    void insertText(size_t line, size_t col, const std::string& text) override;
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) override;
    bool isModified() const override;
    void setModified(bool modified) override;

    // Optional: Friend declaration for stream operator
    // friend std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

private:
    std::vector<std::string> lines_;
    bool modified_ = false;
};

// Optional: Declaration for potential stream operator
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

#endif // TEXTBUFFER_H 