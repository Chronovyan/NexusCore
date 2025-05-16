#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include <vector>
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <utility> // For std::pair

// Forward declaration for a friend function if needed later for direct stream output
// class TextBuffer;
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

class TextBuffer {
public:
    TextBuffer();

    // Basic operations
    void addLine(const std::string& line);
    void insertLine(size_t index, const std::string& line);
    void deleteLine(size_t index);
    void replaceLine(size_t index, const std::string& newLine);
    void setLine(size_t lineIndex, const std::string& text);
    
    // Multi-line operations
    void deleteLines(size_t startIndex, size_t endIndex);
    void insertLines(size_t index, const std::vector<std::string>& newLines);

    // Accessors
    const std::string& getLine(size_t index) const;
    std::string& getLine(size_t index); // Non-const version
    size_t lineCount() const;
    bool isEmpty() const;

    // Additional buffer content information methods
    size_t lineLength(size_t lineIndex) const;
    size_t characterCount() const;
    std::vector<std::string> getAllLines() const;
    
    // Safety improvements
    bool isValidPosition(size_t lineIndex, size_t colIndex) const;
    std::pair<size_t, size_t> clampPosition(size_t lineIndex, size_t colIndex) const;

    // For displaying the buffer content
    void printToStream(std::ostream& os) const;

    // File operations
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    // Character level operations
    void insertChar(size_t lineIndex, size_t colIndex, char ch);
    void deleteChar(size_t lineIndex, size_t colIndex); // For backspace
    void deleteCharForward(size_t lineIndex, size_t colIndex); // For delete key

    // Enhanced string manipulation
    void replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText);
    void deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol);

    // Line manipulation
    void splitLine(size_t lineIndex, size_t colIndex); // For Enter key
    void joinLines(size_t lineIndex); // Join with next line
    void clear(bool keepEmptyLine);

    // Allow direct string manipulation for access to internal structure
    void insertString(size_t lineIndex, size_t colIndex, const std::string& text);
    std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const;

    // Optional: Friend declaration for stream operator
    // friend std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

private:
    std::vector<std::string> lines_;
};

// Optional: Declaration for potential stream operator
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

#endif // TEXTBUFFER_H 