#pragma once

#include "interfaces/ITextBuffer.hpp"
#include "VirtualizedTextBuffer.h"
#include <memory>
#include <shared_mutex>
#include <atomic>

/**
 * @brief Thread-safe decorator for VirtualizedTextBuffer
 * 
 * This class wraps a VirtualizedTextBuffer instance and provides thread-safe access
 * to its methods using appropriate synchronization mechanisms.
 * 
 * Thread Safety Guarantees:
 * 1. Individual method calls are thread-safe
 * 2. References returned by methods remain valid only until the next modification
 *    of the buffer from any thread. Callers must be careful when storing references.
 * 3. For operations that need to be atomic across multiple method calls,
 *    use the lockForReading() and lockForWriting() methods
 */
class ThreadSafeVirtualizedTextBuffer : public ITextBuffer {
public:
    /**
     * @brief Construct a new ThreadSafeVirtualizedTextBuffer
     * 
     * @param buffer Optional existing VirtualizedTextBuffer to wrap. If nullptr, creates a new one.
     */
    explicit ThreadSafeVirtualizedTextBuffer(std::shared_ptr<VirtualizedTextBuffer> buffer = nullptr);

    /**
     * @brief Construct a new ThreadSafeVirtualizedTextBuffer from a file
     * 
     * @param filename The path to the file to load
     * @param pageSize The number of lines per page (default: 1000)
     * @param cacheSize The maximum number of pages to keep in memory (default: 10)
     */
    explicit ThreadSafeVirtualizedTextBuffer(const std::string& filename, 
                                           size_t pageSize = 1000, 
                                           size_t cacheSize = 10);

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
     * @brief Get the underlying VirtualizedTextBuffer
     * 
     * This method is provided for advanced use cases where direct access
     * to the underlying VirtualizedTextBuffer is needed. Use with caution,
     * as operations on the returned VirtualizedTextBuffer are not thread-safe.
     * 
     * @return std::shared_ptr<VirtualizedTextBuffer> The underlying VirtualizedTextBuffer
     */
    std::shared_ptr<VirtualizedTextBuffer> getUnderlyingBuffer() const;

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

    /**
     * @brief Set the page size for the underlying buffer
     * 
     * @param pageSize The new page size (number of lines per page)
     */
    void setPageSize(size_t pageSize);

    /**
     * @brief Set the cache size for the underlying buffer
     * 
     * @param cacheSize The new cache size (number of pages to keep in memory)
     */
    void setCacheSize(size_t cacheSize);

    /**
     * @brief Get the current page size
     * 
     * @return The current page size
     */
    size_t getPageSize() const;

    /**
     * @brief Get the current cache size
     * 
     * @return The current cache size
     */
    size_t getCacheSize() const;

    /**
     * @brief Get the number of pages in memory
     * 
     * @return The number of pages currently loaded in memory
     */
    size_t getPagesInMemory() const;

    /**
     * @brief Get the cache hit rate
     * 
     * @return The cache hit rate as a percentage (0-100)
     */
    double getCacheHitRate() const;

    /**
     * @brief Reset the cache statistics
     */
    void resetCacheStats();

    /**
     * @brief Prefetch a range of lines
     * 
     * This method can be used to proactively load pages that will be needed soon,
     * for example when scrolling through a file.
     * 
     * @param startLine The first line to prefetch
     * @param endLine The last line to prefetch
     */
    void prefetchLines(size_t startLine, size_t endLine);

private:
    std::shared_ptr<VirtualizedTextBuffer> buffer_;
    mutable std::shared_mutex mutex_;
    mutable std::string temporaryLine_; // For returning references
}; 