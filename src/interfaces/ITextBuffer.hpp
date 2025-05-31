#pragma once

#include <string>
#include <vector>
#include <utility>
#include <iosfwd>
#include <functional>

/**
 * @interface ITextBuffer
 * @brief Interface for text buffer components
 * 
 * This interface defines the contract for text buffer implementations,
 * providing methods for line manipulation, file operations, and character-level editing.
 */
class ITextBuffer {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~ITextBuffer() = default;
    
    // Basic operations
    /**
     * @brief Add a line to the end of the buffer
     * 
     * @param line The line to add
     */
    virtual void addLine(const std::string& line) = 0;
    
    /**
     * @brief Insert a line at the specified index
     * 
     * @param index The index to insert at
     * @param line The line to insert
     */
    virtual void insertLine(size_t index, const std::string& line) = 0;
    
    /**
     * @brief Delete a line at the specified index
     * 
     * @param index The index of the line to delete
     */
    virtual void deleteLine(size_t index) = 0;
    
    /**
     * @brief Replace a line at the specified index
     * 
     * @param index The index of the line to replace
     * @param newLine The new line content
     */
    virtual void replaceLine(size_t index, const std::string& newLine) = 0;
    
    /**
     * @brief Set the content of a line at the specified index
     * 
     * @param lineIndex The index of the line to set
     * @param text The new text for the line
     */
    virtual void setLine(size_t lineIndex, const std::string& text) = 0;
    
    // Multi-line operations
    /**
     * @brief Delete a range of lines
     * 
     * @param startIndex The index of the first line to delete
     * @param endIndex The index of the last line to delete
     */
    virtual void deleteLines(size_t startIndex, size_t endIndex) = 0;
    
    /**
     * @brief Insert multiple lines at the specified index
     * 
     * @param index The index to insert at
     * @param newLines The lines to insert
     */
    virtual void insertLines(size_t index, const std::vector<std::string>& newLines) = 0;
    
    // Accessors
    /**
     * @brief Get a line at the specified index (const version)
     * 
     * @param index The index of the line to get
     * @return A const reference to the line
     */
    virtual const std::string& getLine(size_t index) const = 0;
    
    /**
     * @brief Get a line at the specified index (non-const version)
     * 
     * @param index The index of the line to get
     * @return A reference to the line
     */
    virtual std::string& getLine(size_t index) = 0;
    
    /**
     * @brief Get the number of lines in the buffer
     * 
     * @return The line count
     */
    virtual size_t lineCount() const = 0;
    
    /**
     * @brief Check if the buffer is empty
     * 
     * @return True if the buffer is empty
     */
    virtual bool isEmpty() const = 0;
    
    // Additional buffer content information methods
    /**
     * @brief Get the length of a line
     * 
     * @param lineIndex The index of the line
     * @return The length of the line
     */
    virtual size_t lineLength(size_t lineIndex) const = 0;
    
    /**
     * @brief Get the total number of characters in the buffer
     * 
     * @return The character count
     */
    virtual size_t characterCount() const = 0;
    
    /**
     * @brief Get all lines in the buffer
     * 
     * @return A vector of all lines
     */
    virtual std::vector<std::string> getAllLines() const = 0;
    
    // Safety improvements
    /**
     * @brief Check if a position is valid
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     * @return True if the position is valid
     */
    virtual bool isValidPosition(size_t lineIndex, size_t colIndex) const = 0;
    
    /**
     * @brief Clamp a position to the buffer bounds
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     * @return A pair of the clamped line and column indices
     */
    virtual std::pair<size_t, size_t> clampPosition(size_t lineIndex, size_t colIndex) const = 0;
    
    // For displaying the buffer content
    /**
     * @brief Print the buffer to an output stream
     * 
     * @param os The output stream
     */
    virtual void printToStream(std::ostream& os) const = 0;
    
    // File operations
    /**
     * @brief Save the buffer to a file
     * 
     * @param filename The filename
     * @return True if the operation was successful
     */
    virtual bool saveToFile(const std::string& filename) const = 0;
    
    /**
     * @brief Load the buffer from a file
     * 
     * @param filename The filename
     * @return True if the operation was successful
     */
    virtual bool loadFromFile(const std::string& filename) = 0;
    
    // Character level operations
    /**
     * @brief Insert a character at the specified position
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     * @param ch The character to insert
     */
    virtual void insertChar(size_t lineIndex, size_t colIndex, char ch) = 0;
    
    /**
     * @brief Delete a character before the specified position (backspace)
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     */
    virtual void deleteChar(size_t lineIndex, size_t colIndex) = 0;
    
    /**
     * @brief Delete a character at the specified position (delete)
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     */
    virtual void deleteCharForward(size_t lineIndex, size_t colIndex) = 0;
    
    // Enhanced string manipulation
    /**
     * @brief Replace a segment of a line
     * 
     * @param lineIndex The line index
     * @param startCol The start column
     * @param endCol The end column
     * @param newText The replacement text
     */
    virtual void replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText) = 0;
    
    /**
     * @brief Delete a segment of a line
     * 
     * @param lineIndex The line index
     * @param startCol The start column
     * @param endCol The end column
     */
    virtual void deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol) = 0;
    
    // Line manipulation
    /**
     * @brief Split a line at the specified position
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     */
    virtual void splitLine(size_t lineIndex, size_t colIndex) = 0;
    
    /**
     * @brief Join a line with the next line
     * 
     * @param lineIndex The line index
     */
    virtual void joinLines(size_t lineIndex) = 0;
    
    /**
     * @brief Clear the buffer
     * 
     * @param keepEmptyLine Whether to keep an empty line (default: false)
     */
    virtual void clear(bool keepEmptyLine = false) = 0;
    
    // String manipulation
    /**
     * @brief Insert a string at the specified position
     * 
     * @param lineIndex The line index
     * @param colIndex The column index
     * @param text The text to insert
     */
    virtual void insertString(size_t lineIndex, size_t colIndex, const std::string& text) = 0;
    
    /**
     * @brief Get a segment of a line
     * 
     * @param lineIndex The line index
     * @param startCol The start column
     * @param endCol The end column
     * @return The line segment
     */
    virtual std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const = 0;

    /**
     * @brief Get the number of lines in the buffer
     *
     * @return The number of lines
     */
    virtual size_t getLineCount() const = 0;

    /**
     * @brief Get all lines in the buffer
     *
     * @return A vector of all lines
     */
    virtual std::vector<std::string> getLines() const = 0;

    /**
     * @brief Replace a range of text in the buffer
     *
     * @param startLine The starting line index
     * @param startCol The starting column index
     * @param endLine The ending line index
     * @param endCol The ending column index
     * @param text The new text to replace with
     */
    virtual void replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text) = 0;

    /**
     * @brief Insert text at a specific position
     *
     * @param line The line index to insert at
     * @param col The column index to insert at
     * @param text The text to insert
     */
    virtual void insertText(size_t line, size_t col, const std::string& text) = 0;

    /**
     * @brief Delete a range of text from the buffer
     *
     * @param startLine The starting line index
     * @param startCol The starting column index
     * @param endLine The ending line index
     * @param endCol The ending column index
     */
    virtual void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) = 0;

    /**
     * @brief Check if the buffer has been modified
     *
     * @return True if the buffer has been modified
     */
    virtual bool isModified() const = 0;

    /**
     * @brief Set the modified state of the buffer
     *
     * @param modified The new modified state
     */
    virtual void setModified(bool modified) = 0;
}; 