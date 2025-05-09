#ifndef SYNTAX_HIGHLIGHTING_MANAGER_H
#define SYNTAX_HIGHLIGHTING_MANAGER_H

#include "SyntaxHighlighter.h"
#include "EditorError.h"
#include "TextBuffer.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>

class SyntaxHighlightingManager {
public:
    // Maximum time allowed for a highlighting operation (milliseconds)
    static constexpr size_t DEFAULT_HIGHLIGHTING_TIMEOUT_MS = 50;
    
    // Default number of context lines to highlight around visible area
    static constexpr size_t DEFAULT_CONTEXT_LINES = 100;
    
    // Cache entry lifetime in milliseconds
    static constexpr size_t CACHE_ENTRY_LIFETIME_MS = 10000; // 10 seconds
    
    SyntaxHighlightingManager();
    ~SyntaxHighlightingManager() = default;
    
    // Set the active highlighter
    void setHighlighter(SyntaxHighlighter* highlighter);
    
    // Get the current highlighter
    SyntaxHighlighter* getHighlighter() const;
    
    // Enable or disable syntax highlighting
    void setEnabled(bool enabled);
    
    // Check if syntax highlighting is enabled
    bool isEnabled() const;
    
    // Set the buffer to highlight
    void setBuffer(const TextBuffer* buffer);
    
    // Get highlighting styles for a range of lines (const version - only returns cached styles)
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const;
    
    // Get highlighting styles for a range of lines (non-const version - updates cache as needed)
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine);
    
    // Invalidate the cache for a specific line
    void invalidateLine(size_t line);
    
    // Invalidate all lines in a range
    void invalidateLines(size_t startLine, size_t endLine);
    
    // Invalidate the entire cache
    void invalidateAllLines();
    
    // Set the visible range (for prioritizing highlighting)
    void setVisibleRange(size_t startLine, size_t endLine);
    
    // Set the highlighting timeout in milliseconds
    void setHighlightingTimeout(size_t timeoutMs);
    
    // Get the current highlighting timeout in milliseconds
    size_t getHighlightingTimeout() const;
    
    // Set the number of context lines to highlight around visible area
    void setContextLines(size_t contextLines);
    
    // Get the number of context lines highlighted around visible area
    size_t getContextLines() const;
    
private:
    void invalidateAllLines_nolock(); // Added for internal use
    
    // Highlight a single line and store in cache
    void highlightLine(size_t line);
    
    // Highlight a range of lines with timeout
    bool highlightLines(size_t startLine, size_t endLine, 
                        const std::chrono::milliseconds& timeout);
    
    // Check if a line is within the cache range
    bool isLineInCache(size_t line) const;
    
    // Check if a line's highlighting is valid
    bool isLineValid(size_t line) const;
    
    // Calculate the effective range to highlight (with context)
    std::pair<size_t, size_t> calculateEffectiveRange(size_t startLine, size_t endLine) const;
    
    // Clean up old cache entries to manage memory
    void cleanupCache();

private:
    // The buffer to highlight
    const TextBuffer* buffer_;
    
    // The active highlighter
    SyntaxHighlighter* highlighter_;
    
    // Is syntax highlighting enabled
    bool enabled_;
    
    // Highlighted styles cache
    std::vector<std::vector<SyntaxStyle>> cachedStyles_;
    
    // Set of lines that need to be rehighlighted
    std::unordered_set<size_t> invalidatedLines_;
    
    // Last time each line was highlighted (for cache management)
    std::unordered_map<size_t, std::chrono::steady_clock::time_point> lineTimestamps_;
    
    // The current visible range
    size_t visibleStartLine_;
    size_t visibleEndLine_;
    
    // Highlighting timeout in milliseconds
    size_t highlightingTimeoutMs_;
    
    // Number of context lines to highlight around visible area
    size_t contextLines_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
};

#endif // SYNTAX_HIGHLIGHTING_MANAGER_H 