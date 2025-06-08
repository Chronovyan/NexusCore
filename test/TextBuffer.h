#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include <vector>
#include <string>
#include <iosfwd> // For std::ostream forward declaration
#include <utility> // For std::pair
#include <thread> // For std::thread::id
#include <iostream> // For std::cerr

// Forward declaration for EditorException
class EditorException : public std::exception {
    std::string message_;
    int severity_;
public:
    enum Severity {
        EDITOR_ERROR = 0,
        EDITOR_WARNING = 1,
        EDITOR_INFO = 2
    };
    
    EditorException(const std::string& message, int severity = EDITOR_ERROR)
        : message_(message), severity_(severity) {}
        
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    int getSeverity() const { return severity_; }
};

class TextBufferException : public EditorException {
public:
    TextBufferException(const std::string& message, int severity = EditorException::EDITOR_ERROR)
        : EditorException(message, severity) {}
};

// Interface for TextBuffer
class ITextBuffer {
public:
    virtual ~ITextBuffer() = default;
    
    // Basic operations
    virtual void addLine(const std::string& line) = 0;
    virtual void insertLine(size_t index, const std::string& line) = 0;
    virtual void deleteLine(size_t index) = 0;
    virtual void replaceLine(size_t index, const std::string& newLine) = 0;
    virtual void setLine(size_t lineIndex, const std::string& text) = 0;
    
    // Multi-line operations
    virtual void deleteLines(size_t startIndex, size_t endIndex) = 0;
    virtual void insertLines(size_t index, const std::vector<std::string>& newLines) = 0;

    // Accessors
    virtual const std::string& getLine(size_t index) const = 0;
    virtual std::string& getLine(size_t index) = 0;
    virtual size_t lineCount() const = 0;
    virtual bool isEmpty() const = 0;

    // Additional buffer content information methods
    virtual size_t lineLength(size_t lineIndex) const = 0;
    virtual size_t characterCount() const = 0;
    virtual std::vector<std::string> getAllLines() const = 0;
    
    // Safety improvements
    virtual bool isValidPosition(size_t lineIndex, size_t colIndex) const = 0;
    virtual std::pair<size_t, size_t> clampPosition(size_t lineIndex, size_t colIndex) const = 0;

    // For displaying the buffer content
    virtual void printToStream(std::ostream& os) const = 0;

    // File operations
    virtual bool saveToFile(const std::string& filename) const = 0;
    virtual bool loadFromFile(const std::string& filename) = 0;
    
    // Text editing operations
    virtual void insertText(size_t line, size_t col, const std::string& text) = 0;
    virtual void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) = 0;
    
    // Other required methods
    virtual size_t getLineCount() const = 0;
    virtual std::vector<std::string> getLines() const = 0;
    virtual bool isModified() const = 0;
    virtual void setModified(bool modified) = 0;
};


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
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    // Character level operations
    void insertChar(size_t lineIndex, size_t colIndex, char ch);
    void deleteChar(size_t lineIndex, size_t colIndex); 
    void deleteCharForward(size_t lineIndex, size_t colIndex); 

    // Line segment operations
    void replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText);
    void deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol);

    // Line operations
    void splitLine(size_t lineIndex, size_t colIndex); 
    void joinLines(size_t lineIndex); 

    // Clear the buffer
    void clear(bool keepEmptyLine = true);
    
    // String operations
    void insertString(size_t lineIndex, size_t colIndex, const std::string& text);
    std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const;

    // Additional ITextBuffer interface methods
    size_t getLineCount() const;
    std::vector<std::string> getLines() const;
    void replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text);
    void insertText(size_t line, size_t col, const std::string& text);
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
    bool isModified() const;
    void setModified(bool modified);
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) override;
    bool isModified() const override;
    void setModified(bool modified) override;
    
    // Thread ownership methods for use with EditorCoreThreadPool
    
    /**
     * @brief Set the owner thread ID for this text buffer
     * 
     * This method is used by the EditorCoreThreadPool to establish ownership
     * of the buffer by a specific thread. Only the owner thread should make
     * modifications to the buffer to ensure thread safety.
     *
     * @param threadId The ID of the thread that will own this buffer
     */
    void setOwnerThread(const std::thread::id& threadId);
    
    /**
     * @brief Process pending operations in the buffer's operation queue
     * 
     * This method processes any pending operations in the buffer's operation queue.
     * It should be called periodically by the owner thread to ensure operations
     * submitted by other threads are executed.
     *
     * @return The number of operations processed
     */
    size_t processOperationQueue();

    // Optional: Friend declaration for stream operator
    // friend std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

private:
    std::vector<std::string> lines_;
    bool modified_ = false;
    std::thread::id ownerThreadId_; // ID of the thread that owns this buffer
};

// Optional: Declaration for potential stream operator
// std::ostream& operator<<(std::ostream& os, const TextBuffer& buffer);

#endif // TEXTBUFFER_H 