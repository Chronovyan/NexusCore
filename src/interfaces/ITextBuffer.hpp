#pragma once

#include <string>
#include <vector>
#include <utility>
#include <iosfwd>
#include <functional>

/**
 * @interface ITextBuffer
 * @brief Interface for text buffer management
 * 
 * Defines the contract for components that store and manipulate text content.
 * This interface is designed to be thread-safe, with most methods taking
 * appropriate locks to ensure data consistency across threads.
 */
class ITextBuffer {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~ITextBuffer() = default;

    // Basic operations
    /**
     * @brief Adds a line at the end of the buffer (thread-safe, takes write lock)
     */
    virtual void addLine(const std::string& line) = 0;
    
    /**
     * @brief Inserts a line at the specified index (thread-safe, takes write lock)
     */
    virtual void insertLine(size_t index, const std::string& line) = 0;
    
    /**
     * @brief Deletes the line at the specified index (thread-safe, takes write lock)
     */
    virtual void deleteLine(size_t index) = 0;
    
    /**
     * @brief Replaces the line at the specified index (thread-safe, takes write lock)
     */
    virtual void replaceLine(size_t index, const std::string& newLine) = 0;
    
    /**
     * @brief Sets the line at the specified index (thread-safe, takes write lock)
     */
    virtual void setLine(size_t lineIndex, const std::string& text) = 0;
    
    // Multi-line operations
    /**
     * @brief Deletes multiple lines (thread-safe, takes write lock)
     */
    virtual void deleteLines(size_t startIndex, size_t endIndex) = 0;
    
    /**
     * @brief Inserts multiple lines at specified index (thread-safe, takes write lock)
     */
    virtual void insertLines(size_t index, const std::vector<std::string>& newLines) = 0;

    // Accessors
    /**
     * @brief Gets a const reference to line at index (thread-safe, takes read lock)
     * @warning The reference is only valid during the method call and should not be stored
     */
    virtual const std::string& getLine(size_t index) const = 0;
    
    /**
     * @brief Gets a modifiable copy of the line at index
     * @warning This method returns a copy of the line for thread safety. 
     * Changes to the returned string will NOT affect the actual buffer.
     * @see modifyLine for thread-safe modifications
     */
    virtual std::string& getLine(size_t index) = 0;
    
    /**
     * @brief Thread-safe way to modify a line using a callback function
     * @param index The line index to modify
     * @param modifier A function that takes a reference to the line and modifies it in place
     * 
     * This method holds an exclusive lock during the modification to ensure thread safety.
     * Example usage:
     * ```
     * buffer->modifyLine(lineIdx, [](std::string& line) {
     *     line += " appended text";
     * });
     * ```
     */
    virtual void modifyLine(size_t index, const std::function<void(std::string&)>& modifier) = 0;
    
    /**
     * @brief Gets the number of lines in the buffer (thread-safe, takes read lock)
     */
    virtual size_t lineCount() const = 0;
    
    /**
     * @brief Checks if the buffer is empty (thread-safe, takes read lock)
     */
    virtual bool isEmpty() const = 0;

    // Additional buffer content information methods
    /**
     * @brief Gets the length of a line (thread-safe, takes read lock)
     */
    virtual size_t lineLength(size_t lineIndex) const = 0;
    
    /**
     * @brief Gets the total number of characters in the buffer (thread-safe, takes read lock)
     */
    virtual size_t characterCount() const = 0;
    
    /**
     * @brief Gets a copy of all lines in the buffer (thread-safe, takes read lock)
     */
    virtual std::vector<std::string> getAllLines() const = 0;
    
    // Safety improvements
    /**
     * @brief Checks if a position is valid (thread-safe, takes read lock)
     */
    virtual bool isValidPosition(size_t lineIndex, size_t colIndex) const = 0;
    
    /**
     * @brief Clamps a position to be within valid bounds (thread-safe, takes read lock)
     */
    virtual std::pair<size_t, size_t> clampPosition(size_t lineIndex, size_t colIndex) const = 0;

    // For displaying the buffer content
    /**
     * @brief Prints the buffer content to a stream (thread-safe, takes read lock)
     */
    virtual void printToStream(std::ostream& os) const = 0;

    // File operations
    /**
     * @brief Saves the buffer to a file (thread-safe, takes read lock)
     */
    virtual bool saveToFile(const std::string& filename) const = 0;
    
    /**
     * @brief Loads the buffer from a file (thread-safe, takes write lock)
     */
    virtual bool loadFromFile(const std::string& filename) = 0;

    // Character level operations
    /**
     * @brief Inserts a character at specified position (thread-safe, takes write lock)
     */
    virtual void insertChar(size_t lineIndex, size_t colIndex, char ch) = 0;
    
    /**
     * @brief Deletes the character before the cursor (thread-safe, takes write lock)
     */
    virtual void deleteChar(size_t lineIndex, size_t colIndex) = 0;
    
    /**
     * @brief Deletes the character at the cursor position (thread-safe, takes write lock)
     */
    virtual void deleteCharForward(size_t lineIndex, size_t colIndex) = 0;

    // Enhanced string manipulation
    /**
     * @brief Replaces a segment of text (thread-safe, takes write lock)
     */
    virtual void replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText) = 0;
    
    /**
     * @brief Deletes a segment of text (thread-safe, takes write lock)
     */
    virtual void deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol) = 0;

    // Line manipulation
    /**
     * @brief Splits a line at the specified position (thread-safe, takes write lock)
     */
    virtual void splitLine(size_t lineIndex, size_t colIndex) = 0;
    
    /**
     * @brief Joins a line with the next line (thread-safe, takes write lock)
     */
    virtual void joinLines(size_t lineIndex) = 0;
    
    /**
     * @brief Clears the buffer (thread-safe, takes write lock)
     */
    virtual void clear(bool keepEmptyLine) = 0;

    // String manipulation
    /**
     * @brief Inserts a string at specified position (thread-safe, takes write lock)
     */
    virtual void insertString(size_t lineIndex, size_t colIndex, const std::string& text) = 0;
    
    /**
     * @brief Gets a segment of a line (thread-safe, takes read lock)
     */
    virtual std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const = 0;
}; 