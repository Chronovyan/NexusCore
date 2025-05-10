#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include <vector>
#include <string>
#include <iosfwd> // For std::ostream forward declaration

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

    // Accessors
    const std::string& getLine(size_t index) const;
    std::string& getLine(size_t index); // Non-const version
    size_t lineCount() const;
    bool isEmpty() const;

    // For displaying the buffer content
    void printToStream(std::ostream& os) const;

    // File operations
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    // Character level operations
    void insertChar(size_t lineIndex, size_t colIndex, char ch);
    void deleteChar(size_t lineIndex, size_t colIndex); // For backspace
    void deleteCharForward(size_t lineIndex, size_t colIndex); // For delete key

    // Line manipulation
    void splitLine(size_t lineIndex, size_t colIndex); // For Enter key
    void joinLines(size_t lineIndex); // Join with next line
    
    // String operations
    void insertString(size_t lineIndex, size_t colIndex, const std::string& text);
    std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const;
    
    // Line properties
    size_t lineLength(size_t lineIndex) const;

    // New methods
    void clear(bool keepEmptyLine = true); // Modified to control empty line behavior

private:
    std::vector<std::string> lines_;

    // Friend declaration for potential stream operator
    // friend std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);
};

#endif // TEXTBUFFER_H 