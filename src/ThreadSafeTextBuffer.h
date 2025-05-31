#pragma once

#include "interfaces/ITextBuffer.hpp"
#include "TextBuffer.h"
#include <memory>
#include <shared_mutex>
#include <atomic>

/**
 * @brief Thread-safe decorator for TextBuffer
 * 
 * This class wraps a TextBuffer instance and provides thread-safe access
 * to its methods using appropriate synchronization mechanisms.
 * 
 * Thread Safety Guarantees:
 * 1. Individual method calls are thread-safe
 * 2. References returned by methods remain valid only until the next modification
 *    of the buffer from any thread. Callers must be careful when storing references.
 * 3. For operations that need to be atomic across multiple method calls,
 *    use the lockForReading() and lockForWriting() methods
 */
class ThreadSafeTextBuffer : public ITextBuffer {
public:
    /**
     * @brief Construct a new ThreadSafeTextBuffer
     * 
     * @param buffer Optional existing TextBuffer to wrap. If nullptr, creates a new one.
     */
    explicit ThreadSafeTextBuffer(std::shared_ptr<TextBuffer> buffer = nullptr);
    
    // ITextBuffer interface implementation
    void addLine(const std::string& line) override;
    void insertLine(size_t index, const std::string& line) override;
    void deleteLine(size_t index) override;
    void replaceLine(size_t index, const std::string& newLine) override;
    void setLine(size_t lineIndex, const std::string& text) override;
    void deleteLines(size_t startIndex, size_t endIndex) override;
    void insertLines(size_t index, const std::vector<std::string>& newLines) override;
    const std::string& getLine(size_t index) const override;
    std::string& getLine(size_t index) override;
    size_t lineCount() const override;
    bool isEmpty() const override;
    size_t lineLength(size_t lineIndex) const override;
    size_t characterCount() const override;
    std::vector<std::string> getAllLines() const override;
    bool isValidPosition(size_t lineIndex, size_t colIndex) const override;
    std::pair<size_t, size_t> clampPosition(size_t lineIndex, size_t colIndex) const override;
    void printToStream(std::ostream& os) const override;
    bool saveToFile(const std::string& filename) const override;
    bool loadFromFile(const std::string& filename) override;
    void insertChar(size_t lineIndex, size_t colIndex, char ch) override;
    void deleteChar(size_t lineIndex, size_t colIndex) override;
    void deleteCharForward(size_t lineIndex, size_t colIndex) override;
    void replaceLineSegment(size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText) override;
    void deleteLineSegment(size_t lineIndex, size_t startCol, size_t endCol) override;
    void splitLine(size_t lineIndex, size_t colIndex) override;
    void joinLines(size_t lineIndex) override;
    void clear(bool keepEmptyLine) override;
    void insertString(size_t lineIndex, size_t colIndex, const std::string& text) override;
    std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const override;
    size_t getLineCount() const override;
    std::vector<std::string> getLines() const override;
    void replaceText(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& text) override;
    void insertText(size_t line, size_t col, const std::string& text) override;
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) override;
    bool isModified() const override;
    void setModified(bool modified) override;
    
    /**
     * @brief Get the underlying TextBuffer
     * 
     * This method is provided for advanced use cases where direct access
     * to the underlying TextBuffer is needed. Use with caution, as operations
     * on the returned TextBuffer are not thread-safe.
     * 
     * @return std::shared_ptr<TextBuffer> The underlying TextBuffer
     */
    std::shared_ptr<TextBuffer> getUnderlyingBuffer() const;
    
    /**
     * @brief Lock for reading
     * 
     * This method acquires a shared lock on the buffer for reading.
     * Use this when you need to perform multiple read operations atomically.
     * IMPORTANT: You MUST call unlockReading() when done to prevent deadlocks.
     * 
     * Example usage:
     * ```
     * buffer.lockForReading();
     * try {
     *     // Multiple read operations...
     * } catch (...) {
     *     buffer.unlockReading();
     *     throw;
     * }
     * buffer.unlockReading();
     * ```
     */
    void lockForReading() const;
    
    /**
     * @brief Unlock after reading
     * 
     * Call this method to release a shared lock acquired with lockForReading().
     */
    void unlockReading() const;
    
    /**
     * @brief Lock for writing
     * 
     * This method acquires an exclusive lock on the buffer for writing.
     * Use this when you need to perform multiple write operations atomically.
     * IMPORTANT: You MUST call unlockWriting() when done to prevent deadlocks.
     * 
     * Example usage:
     * ```
     * buffer.lockForWriting();
     * try {
     *     // Multiple write operations...
     * } catch (...) {
     *     buffer.unlockWriting();
     *     throw;
     * }
     * buffer.unlockWriting();
     * ```
     */
    void lockForWriting();
    
    /**
     * @brief Unlock after writing
     * 
     * Call this method to release an exclusive lock acquired with lockForWriting().
     */
    void unlockWriting();
    
private:
    std::shared_ptr<TextBuffer> buffer_;
    mutable std::shared_mutex mutex_;
    mutable std::atomic<bool> modified_{false};
}; 