#pragma once

#include "SyntaxHighlighter.h"
#include "EditorError.h"
#include "TextBuffer.h"
#include "interfaces/ISyntaxHighlightingManager.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>
#include <string>
#include <sstream>
#include <thread>

// Forward declarations
class TextBuffer;
class SyntaxHighlighter;

// Declaration of global flag to completely disable all logging - used ONLY during tests
extern bool DISABLE_ALL_LOGGING_FOR_TESTS;

class SyntaxHighlightingManager : public ISyntaxHighlightingManager {
public:
    // Maximum time allowed for a highlighting operation (milliseconds)
    static constexpr size_t DEFAULT_HIGHLIGHTING_TIMEOUT_MS = 50;
    
    // Default number of context lines to highlight around visible area
    static constexpr size_t DEFAULT_CONTEXT_LINES = 10;  // Reduced from 20
    
    // Reduced context for non-visible lines
    static constexpr size_t NON_VISIBLE_CONTEXT_LINES = 2;  // Reduced from 5
    
    // Minimum number of lines to process per batch before timing out
    static constexpr size_t MIN_LINES_PER_BATCH = 5;
    
    // Cache entry lifetime in milliseconds
    static constexpr size_t CACHE_ENTRY_LIFETIME_MS = 10000; // 10 seconds
    
    // Maximum number of lines to cache before triggering eviction
    static constexpr size_t MAX_CACHE_LINES = 10000;
    
    // Threshold ratio for cache cleanup (remove entries until cache is this % of max)
    static constexpr float CACHE_CLEANUP_RATIO = 0.8f;
    
    // Maximum batch size for processing lines
    static constexpr size_t MAX_BATCH_SIZE = 25;  // Added explicit batch size limit
    
    // Time window to consider a line as "recently processed" (milliseconds)
    static constexpr size_t RECENTLY_PROCESSED_WINDOW_MS = 1000;  // 1 second
    
    // Minimum number of requested lines to process before timing out
    static constexpr size_t MIN_REQUESTED_LINES_PROGRESS = 5;  // New constant
    
    SyntaxHighlightingManager();
    ~SyntaxHighlightingManager();
    
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
        
        CacheEntry() noexcept : timestamp(std::chrono::steady_clock::now()) {}
        
        CacheEntry(const std::vector<SyntaxStyle>& s) noexcept
            : styles(s), 
              timestamp(std::chrono::steady_clock::now()) {}
              
        CacheEntry(std::vector<SyntaxStyle>&& s) noexcept
            : styles(std::move(s)), 
              timestamp(std::chrono::steady_clock::now()) {}
              
        // Simplified copy constructor that doesn't allocate memory unnecessarily
        CacheEntry(const CacheEntry& other) noexcept
            : styles(other.styles),
              timestamp(other.timestamp),
              valid(other.valid.load(std::memory_order_acquire)) {}
              
        // Simplified assignment operator
        CacheEntry& operator=(const CacheEntry& other) noexcept {
            if (this != &other) {
                styles = other.styles;
                timestamp = other.timestamp;
                valid.store(other.valid.load(std::memory_order_acquire), 
                           std::memory_order_release);
            }
            return *this;
        }
        
        // Simplified move constructor
        CacheEntry(CacheEntry&& other) noexcept
            : styles(std::move(other.styles)),
              timestamp(other.timestamp),
              valid(other.valid.load(std::memory_order_acquire)) {}
              
        // Simplified move assignment
        CacheEntry& operator=(CacheEntry&& other) noexcept {
            if (this != &other) {
                styles = std::move(other.styles);
                timestamp = other.timestamp;
                valid.store(other.valid.load(std::memory_order_acquire), 
                           std::memory_order_release);
            }
            return *this;
        }
        
        // No exception-throwing destructor
        ~CacheEntry() noexcept {}
    };
    
    // Set the active highlighter
    void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) override;
    
    // Get the current highlighter
    std::shared_ptr<SyntaxHighlighter> getHighlighter() const override;
    
    // Enable or disable syntax highlighting
    void setEnabled(bool enabled) override;
    
    // Check if syntax highlighting is enabled
    bool isEnabled() const override;
    
    // Set the buffer to highlight (still using non-owning pointer as buffer is owned elsewhere)
    void setBuffer(const TextBuffer* buffer) override;
    
    // Get highlighting styles for a range of lines (const version - only returns cached styles)
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const override;
    
    // Get highlighting styles for a range of lines (non-const version - updates cache as needed)
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) override;
    
    // Invalidate the cache for a specific line
    void invalidateLine(size_t line) override;
    
    // Invalidate all lines in a range
    void invalidateLines(size_t startLine, size_t endLine) override;
    
    // Invalidate the entire cache
    void invalidateAllLines() override;
    
    // Set the visible range (for prioritizing highlighting)
    void setVisibleRange(size_t startLine, size_t endLine) const override;
    
    // Set the highlighting timeout in milliseconds
    void setHighlightingTimeout(size_t timeoutMs) override;
    
    // Get the current highlighting timeout in milliseconds
    size_t getHighlightingTimeout() const override;
    
    // Set the number of context lines to highlight around visible area
    void setContextLines(size_t contextLines) override;
    
    // Get the number of context lines highlighted around visible area
    size_t getContextLines() const override;

    // Highlight a single line and store in cache.
    // This is a public method that will acquire a unique_lock.
    void highlightLine(size_t line) override;
    
    // Get current cache size (for testing)
    size_t getCacheSize() const {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        size_t validCount = 0;
        for (const auto& entry : cachedStyles_) {
            if (entry && entry->valid.load(std::memory_order_acquire)) {
                validCount++;
            }
        }
        return validCount;
    }

    // Control debug logging
    static void setDebugLoggingEnabled(bool enabled);
    static bool isDebugLoggingEnabled();

private:
    void invalidateAllLines_nolock(); // Internal use without locking
    
    // Helper function to log vector access for debugging
    void logVectorAccess(const char* location, size_t index, size_t vectorSize) const;
    
    // Internal method to highlight a single line, assumes caller holds unique_lock.
    std::unique_ptr<std::vector<SyntaxStyle>> highlightLine_nolock(size_t line);
    
    // Internal method to highlight a range of lines with timeout, assumes caller holds unique_lock.
    bool highlightLines_nolock(size_t startLine, size_t endLine, 
                               const std::chrono::milliseconds& timeout);
    
    // Check if a line is within the cache range
    bool isLineInCache(size_t line) const;
    
    // Check if a line's highlighting is valid
    bool isLineValid(size_t line) const;
    
    // Check if a cached entry is too old (expired)
    bool isCacheEntryExpired(const CacheEntry& entry) const;
    
    // Perform cache cleanup, evicting old entries
    void evictLRUEntries_nolock(size_t targetSize);
    
    // Check if a line was recently processed to avoid redundant work
    bool wasRecentlyProcessed(size_t line) const;
    
    // Mark a line as recently processed
    void markAsRecentlyProcessed(size_t line);
    
    // Log entry and exit of major functions with timings
    void logLockAcquisition(const char* method) const;
    void logLockRelease(const char* method, const std::chrono::steady_clock::time_point& start) const;
    
    // Reduce log verbosity and only log significant changes in status
    void logWithReduction(const char* format, ...) const;
    
    // Mutex for thread safety
    mutable std::recursive_mutex mutex_;
    
    // The buffer to highlight - not owned by this class
    const TextBuffer* buffer_ = nullptr;
    
    // The current highlighter
    std::shared_ptr<SyntaxHighlighter> highlighter_;
    
    // Whether syntax highlighting is enabled
    bool enabled_ = true;
    
    // Cache of highlighted lines (for const methods that can't update the cache)
    mutable std::vector<std::unique_ptr<CacheEntry>> cachedStyles_;
    
    // Tracking of invalidated lines
    mutable std::unordered_set<size_t> invalidatedLines_;
    
    // Timestamps and access tracking for cache entries
    mutable std::unordered_map<size_t, std::chrono::steady_clock::time_point> lineTimestamps_;
    mutable std::unordered_map<size_t, std::chrono::steady_clock::time_point> lineAccessTimes_;
    
    // Last cleanup time for cache maintenance
    mutable std::chrono::steady_clock::time_point lastCleanupTime_ = std::chrono::steady_clock::now();
    
    // Visible range
    mutable size_t visibleStartLine_ = 0;
    mutable size_t visibleEndLine_ = 0;
    
    // Configuration
    size_t highlightingTimeoutMs_ = DEFAULT_HIGHLIGHTING_TIMEOUT_MS;
    size_t contextLines_ = DEFAULT_CONTEXT_LINES;
    
    // Recently processed lines (to avoid redundant work)
    struct LastProcessedRange {
        size_t startLine{0};
        size_t endLine{0};
        std::chrono::steady_clock::time_point timestamp;
        
        void update(size_t start, size_t end) {
            startLine = start;
            endLine = end;
            timestamp = std::chrono::steady_clock::now();
        }
        
        void invalidate() {
            timestamp = std::chrono::steady_clock::time_point();
        }
        
        bool isSequentialWith(size_t line) const {
            return (line == endLine + 1) || (line + 1 == startLine);
        }
    };
    
    mutable LastProcessedRange lastProcessedRange_;
    
    // Debug logging control
    static bool debugLoggingEnabled_;

    // Helper methods for internal implementation
    std::pair<size_t, size_t> calculateEffectiveRange(size_t requestedStartLine, size_t requestedEndLine) const;
    void cleanupCache_nolock() const;
    bool isEntryExpired_nolock(const CacheEntry& entry) const;
    void evictLRUEntries_nolock(size_t targetSize) const;
    std::pair<size_t, size_t> calculateOptimalProcessingRange(size_t startLine, size_t endLine) const;
    void logCacheMetrics() const;
    void logVectorAccess(const char* operation, size_t index, size_t size) const;
}; 