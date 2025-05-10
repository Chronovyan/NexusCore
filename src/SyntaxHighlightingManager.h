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
#include <shared_mutex>
#include <atomic>
#include <memory>

// Forward declarations
class TextBuffer;
class SyntaxHighlighter;

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
    
    // Deleted copy/move operations for safety
    SyntaxHighlightingManager(const SyntaxHighlightingManager&) = delete;
    SyntaxHighlightingManager& operator=(const SyntaxHighlightingManager&) = delete;
    SyntaxHighlightingManager(SyntaxHighlightingManager&&) = delete;
    SyntaxHighlightingManager& operator=(SyntaxHighlightingManager&&) = delete;
    
    // Thread-safe cache entry representation
    struct CacheEntry {
        std::vector<SyntaxStyle> styles;
        std::chrono::steady_clock::time_point timestamp;
        std::atomic<bool> valid{true};
        
        CacheEntry() : timestamp(std::chrono::steady_clock::now()) {}
        
        CacheEntry(std::vector<SyntaxStyle> s) 
            : styles(std::move(s)), 
              timestamp(std::chrono::steady_clock::now()) {}
    };
    
    // Set the active highlighter
    void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter);
    
    // Get the current highlighter
    std::shared_ptr<SyntaxHighlighter> getHighlighter() const;
    
    // Enable or disable syntax highlighting
    void setEnabled(bool enabled);
    
    // Check if syntax highlighting is enabled
    bool isEnabled() const;
    
    // Set the buffer to highlight (still using non-owning pointer as buffer is owned elsewhere)
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
    void invalidateAllLines_nolock(); // Internal use without locking
    
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
    void cleanupCache_nolock(); // Renamed from cleanupCache
    
    // Thread-safe access to buffer_
    const TextBuffer* getBuffer() const;

private:
    // The buffer to highlight - non-owning pointer
    std::atomic<const TextBuffer*> buffer_{nullptr};
    
    // The active highlighter - using shared_ptr for safer ownership
    std::shared_ptr<std::atomic<SyntaxHighlighter*>> highlighter_{
        std::make_shared<std::atomic<SyntaxHighlighter*>>(nullptr)
    };
    
    // Is syntax highlighting enabled - use atomic for thread safety
    std::atomic<bool> enabled_{true};
    
    // Highlighted styles cache
    std::vector<std::shared_ptr<CacheEntry>> cachedStyles_;
    
    // Set of lines that need to be rehighlighted
    std::unordered_set<size_t> invalidatedLines_;
    
    // Last time each line was highlighted (for cache management)
    std::unordered_map<size_t, std::chrono::steady_clock::time_point> lineTimestamps_;
    
    // The current visible range
    std::atomic<size_t> visibleStartLine_{0};
    std::atomic<size_t> visibleEndLine_{0};
    
    // Highlighting timeout in milliseconds
    std::atomic<size_t> highlightingTimeoutMs_{DEFAULT_HIGHLIGHTING_TIMEOUT_MS};
    
    // Number of context lines to highlight around visible area
    std::atomic<size_t> contextLines_{DEFAULT_CONTEXT_LINES};
    
    // Mutex for thread safety - using shared_mutex for better read concurrency
    mutable std::shared_mutex mutex_;
};

#endif // SYNTAX_HIGHLIGHTING_MANAGER_H 