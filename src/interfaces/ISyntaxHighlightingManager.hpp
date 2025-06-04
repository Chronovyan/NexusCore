#pragma once

#include <vector>
#include <memory>
#include <string>
#include "SyntaxHighlighter.h"

// Forward declarations
class ITextBuffer;
class TextBuffer;
class SyntaxHighlighter;
struct SyntaxStyle;

/**
 * @interface ISyntaxHighlightingManager
 * @brief Interface for syntax highlighting manager components
 * 
 * This interface defines the contract for syntax highlighting manager implementations,
 * providing methods for managing syntax highlighting of text in a buffer.
 */
class ISyntaxHighlightingManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~ISyntaxHighlightingManager() = default;
    
    /**
     * @brief Set the active highlighter
     * 
     * @param highlighter The highlighter to use
     */
    virtual void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) = 0;
    
    /**
     * @brief Get the current highlighter
     * 
     * @return The current highlighter
     */
    virtual std::shared_ptr<SyntaxHighlighter> getHighlighter() const = 0;
    
    /**
     * @brief Enable or disable syntax highlighting
     * 
     * @param enabled Whether to enable highlighting
     */
    virtual void setEnabled(bool enabled) = 0;
    
    /**
     * @brief Check if syntax highlighting is enabled
     * 
     * @return True if highlighting is enabled
     */
    virtual bool isEnabled() const = 0;
    
    /**
     * @brief Set the buffer to highlight
     * 
     * @param buffer The buffer to highlight
     */
    virtual void setBuffer(const ITextBuffer* buffer) = 0;
    
    /**
     * @brief Get highlighting styles for a range of lines (const version)
     * 
     * @param startLine The first line to get styles for
     * @param endLine The last line to get styles for
     * @return The highlighting styles
     */
    virtual std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const = 0;
    
    /**
     * @brief Get highlighting styles for a range of lines (non-const version)
     * 
     * @param startLine The first line to get styles for
     * @param endLine The last line to get styles for
     * @return The highlighting styles
     */
    virtual std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) = 0;
    
    /**
     * @brief Invalidate the cache for a specific line
     * 
     * @param line The line to invalidate
     */
    virtual void invalidateLine(size_t line) = 0;
    
    /**
     * @brief Invalidate all lines in a range
     * 
     * @param startLine The first line to invalidate
     * @param endLine The last line to invalidate
     */
    virtual void invalidateLines(size_t startLine, size_t endLine) = 0;
    
    /**
     * @brief Invalidate the entire cache
     */
    virtual void invalidateAllLines() = 0;
    
    /**
     * @brief Set the visible range (for prioritizing highlighting)
     * 
     * @param startLine The first visible line
     * @param endLine The last visible line
     */
    virtual void setVisibleRange(size_t startLine, size_t endLine) const = 0;
    
    /**
     * @brief Set the highlighting timeout in milliseconds
     * 
     * @param timeoutMs The timeout in milliseconds
     */
    virtual void setHighlightingTimeout(size_t timeoutMs) = 0;
    
    /**
     * @brief Get the current highlighting timeout in milliseconds
     * 
     * @return The timeout in milliseconds
     */
    virtual size_t getHighlightingTimeout() const = 0;
    
    /**
     * @brief Set the number of context lines to highlight around visible area
     * 
     * @param contextLines The number of context lines
     */
    virtual void setContextLines(size_t contextLines) = 0;
    
    /**
     * @brief Get the number of context lines highlighted around visible area
     * 
     * @return The number of context lines
     */
    virtual size_t getContextLines() const = 0;
    
    /**
     * @brief Highlight a single line and store in cache
     * 
     * @param line The line to highlight
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