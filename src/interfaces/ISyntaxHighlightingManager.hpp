#pragma once

#include <vector>
#include <memory>
#include <string>

// Forward declarations
class TextBuffer;
class SyntaxHighlighter;
struct SyntaxStyle;

/**
 * @interface ISyntaxHighlightingManager
 * @brief Interface for syntax highlighting management
 * 
 * Defines the contract for components that manage syntax highlighting
 * for text buffers, including caching and style computation.
 */
class ISyntaxHighlightingManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~ISyntaxHighlightingManager() = default;
    
    /**
     * @brief Set the active syntax highlighter
     * 
     * @param highlighter Shared pointer to the highlighter to use
     */
    virtual void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) = 0;
    
    /**
     * @brief Get the current syntax highlighter
     * 
     * @return std::shared_ptr<SyntaxHighlighter> The active highlighter
     */
    virtual std::shared_ptr<SyntaxHighlighter> getHighlighter() const = 0;
    
    /**
     * @brief Enable or disable syntax highlighting
     * 
     * @param enabled Whether highlighting should be enabled
     */
    virtual void setEnabled(bool enabled) = 0;
    
    /**
     * @brief Check if syntax highlighting is enabled
     * 
     * @return bool True if highlighting is enabled
     */
    virtual bool isEnabled() const = 0;
    
    /**
     * @brief Set the buffer to highlight
     * 
     * @param buffer Pointer to the text buffer to highlight
     */
    virtual void setBuffer(const TextBuffer* buffer) = 0;
    
    /**
     * @brief Get highlighting styles for a range of lines (readonly access)
     * 
     * @param startLine First line to get styles for
     * @param endLine Last line to get styles for
     * @return std::vector<std::vector<SyntaxStyle>> The highlighting styles
     */
    virtual std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const = 0;
    
    /**
     * @brief Get highlighting styles for a range of lines (may update cache)
     * 
     * @param startLine First line to get styles for
     * @param endLine Last line to get styles for
     * @return std::vector<std::vector<SyntaxStyle>> The highlighting styles
     */
    virtual std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) = 0;
    
    /**
     * @brief Invalidate the cache for a specific line
     * 
     * @param line Line number to invalidate
     */
    virtual void invalidateLine(size_t line) = 0;
    
    /**
     * @brief Invalidate all lines in a range
     * 
     * @param startLine First line to invalidate
     * @param endLine Last line to invalidate
     */
    virtual void invalidateLines(size_t startLine, size_t endLine) = 0;
    
    /**
     * @brief Invalidate the entire cache
     */
    virtual void invalidateAllLines() = 0;
    
    /**
     * @brief Set the visible range (for prioritizing highlighting)
     * 
     * @param startLine First visible line
     * @param endLine Last visible line
     */
    virtual void setVisibleRange(size_t startLine, size_t endLine) const = 0;
    
    /**
     * @brief Set the highlighting timeout in milliseconds
     * 
     * @param timeoutMs Maximum time allowed for highlighting operations
     */
    virtual void setHighlightingTimeout(size_t timeoutMs) = 0;
    
    /**
     * @brief Get the current highlighting timeout in milliseconds
     * 
     * @return size_t The timeout value
     */
    virtual size_t getHighlightingTimeout() const = 0;
    
    /**
     * @brief Set the number of context lines to highlight around visible area
     * 
     * @param contextLines Number of lines to highlight above and below
     */
    virtual void setContextLines(size_t contextLines) = 0;
    
    /**
     * @brief Get the number of context lines highlighted around visible area
     * 
     * @return size_t The context lines count
     */
    virtual size_t getContextLines() const = 0;

    /**
     * @brief Highlight a single line and store in cache
     * 
     * @param line Line number to highlight
     */
    virtual void highlightLine(size_t line) = 0;
    
    /**
     * @brief Get current cache size
     * 
     * @return size_t Number of valid entries in the cache
     */
    virtual size_t getCacheSize() const = 0;
    
    /**
     * @brief Enable or disable debug logging
     * 
     * @param enabled Whether debug logging should be enabled
     */
    virtual void setDebugLoggingEnabled(bool enabled) = 0;
    
    /**
     * @brief Check if debug logging is enabled
     * 
     * @return bool True if debug logging is enabled
     */
    virtual bool isDebugLoggingEnabled() const = 0;
}; 