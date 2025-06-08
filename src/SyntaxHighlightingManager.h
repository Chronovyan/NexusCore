#pragma once

#include "SyntaxHighlighter.h"
#include "EditorError.h"
#include "TextBuffer.h"
#include "interfaces/ISyntaxHighlightingManager.hpp"
#include "ThreadPool.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <future>
#include <deque>

// Forward declarations
class ITextBuffer;
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
    static constexpr size_t MIN_PROCESS_LINES_BEFORE_TIMEOUT = 10;
    
    // Default number of threads in thread pool
    static constexpr size_t DEFAULT_THREAD_POOL_SIZE = 2;
    
    // Maximum size of the work queue (to prevent unbounded growth)
    static constexpr size_t MAX_WORK_QUEUE_SIZE = 100;
    
    // Maximum number of processed ranges to track
    static constexpr size_t MAX_PROCESSED_RANGES = 50;

    /**
     * @brief Task types for thread pool
     */
    enum class TaskType {
        VISIBLE_RANGE,     ///< Highlighting visible range
        CONTEXT_RANGE,     ///< Highlighting context around visible range
        BACKGROUND_RANGE,  ///< Highlighting non-visible range
        SINGLE_LINE        ///< Highlighting single line
    };

    // Constructor
    SyntaxHighlightingManager();
    
    // Destructor
    ~SyntaxHighlightingManager() override;
    
    // Copy/move not allowed
    SyntaxHighlightingManager(const SyntaxHighlightingManager&) = delete;
    SyntaxHighlightingManager& operator=(const SyntaxHighlightingManager&) = delete;
    SyntaxHighlightingManager(SyntaxHighlightingManager&&) = delete;
    SyntaxHighlightingManager& operator=(SyntaxHighlightingManager&&) = delete;
    
    // ISyntaxHighlightingManager interface implementation
    void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) override;
    std::shared_ptr<SyntaxHighlighter> getHighlighter() const override;
    bool isEnabled() const override;
    void setEnabled(bool enabled) override;
    void setBuffer(const ITextBuffer* buffer) override;
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) const override;
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) override;
    void invalidateLine(size_t line) override;
    void invalidateLines(size_t startLine, size_t endLine) override;
    void invalidateAllLines() override;
    void setVisibleRange(size_t startLine, size_t endLine) const override;
    void setHighlightingTimeout(size_t timeoutMs) override;
    size_t getHighlightingTimeout() const override;
    void setContextLines(size_t contextLines) override;
    size_t getContextLines() const override;
    void highlightLine(size_t line) override;
    
    // Thread pool management
    void setThreadPoolSize(size_t numThreads);
    size_t getThreadPoolSize() const;
    size_t getActiveThreadCount() const;
    size_t getQueuedTaskCount() const;
    
    // Internal methods for testing
    size_t getCacheSize() const override;
    bool isDebugLoggingEnabled() const override { return debugLoggingEnabled_; }
    void setDebugLoggingEnabled(bool enabled) override { debugLoggingEnabled_ = enabled; }
    static bool getGlobalDebugLoggingState() { return globalDebugLoggingEnabled_; }
    static void setDebugLoggingEnabled_static(bool enabled) { globalDebugLoggingEnabled_ = enabled; }
    
private:
    // Cache entry for highlighted line
    struct CacheEntry {
        std::vector<SyntaxStyle> styles;
        std::atomic<bool> valid;
        
        explicit CacheEntry(std::vector<SyntaxStyle> s)
            : styles(std::move(s)), valid(true) {}
    };
    
    // Range tracking for optimization
    struct ProcessedRange {
        bool valid = false;
        size_t startLine = 0;
        size_t endLine = 0;
        std::chrono::steady_clock::time_point timestamp;
        
        void update(size_t start, size_t end) {
            valid = true;
            startLine = start;
            endLine = end;
        }
        
        void invalidate() {
            valid = false;
        }
    };
    
    // Internal highlighting methods
    std::unique_ptr<std::vector<SyntaxStyle>> highlightLine_nolock(size_t line);
    bool highlightLines_nolock(size_t startLine, size_t endLine, const std::chrono::milliseconds& timeout);
    void invalidateAllLines_nolock();
    void invalidateLines_nolock(size_t startLine, size_t endLine);
    void evictLRUEntries_nolock(size_t countToEvict) const;
    void evictLinesOlderThan_nolock(const std::chrono::steady_clock::time_point& threshold) const;
    void markAsRecentlyProcessed(size_t line);
    bool wasRecentlyProcessed(size_t line) const;
    std::pair<size_t, size_t> calculateOptimalProcessingRange(size_t requestedStart, size_t requestedEnd) const;
    
    // Cache query methods
    bool isLineInCache(size_t line) const;
    bool isLineValid(size_t line) const;
    bool isEntryExpired_nolock(size_t line) const;
    
    // Thread pool task methods
    void processVisibleRangeAsync(size_t startLine, size_t endLine) const;
    void processSingleLineAsync(size_t line);
    void taskHighlightLines(size_t startLine, size_t endLine, TaskType taskType) const;
    void taskHighlightLine(size_t line);
    
    // Thread priority helpers
    ThreadPool::Priority getTaskPriority(TaskType taskType) const;
    bool shouldQueueTask(TaskType taskType) const;
    
    // Logging helpers
    template<typename... Args>
    void logManagerMessage(EditorException::Severity severity, const char* method, const char* format, Args... args) const;
    void logLockAcquisition(const char* method) const;
    void logLockRelease(const char* method, const std::chrono::steady_clock::time_point& start) const;
    void logWithReduction(const char* format, ...) const;
    void logVectorAccess(const char* location, size_t index, size_t vectorSize) const;
    void logCacheMetrics(const char* context, size_t visibleLines, size_t totalProcessed) const;
    
    // Get buffer pointer safely (may return nullptr if buffer not set)
    const ITextBuffer* getBuffer() const;
    
    // Get highlighter pointer WITHOUT locking - for internal use when mutex is already held
    SyntaxHighlighter* getHighlighterPtr_nolock() const;
    
    // Read-write mutex for thread safety
    mutable std::shared_mutex mutex_;
    
    // The buffer to highlight - not owned by this class
    std::atomic<const ITextBuffer*> buffer_{nullptr};
    
    // The current highlighter
    std::shared_ptr<SyntaxHighlighter> highlighter_;
    
    // Whether syntax highlighting is enabled
    std::atomic<bool> enabled_{true}; // Changed to atomic for lock-free reads
    
    // Cache of highlighted lines
    mutable std::vector<std::unique_ptr<CacheEntry>> cachedStyles_;
    
    // Set of invalidated lines (awaiting rehighlighting)
    mutable std::unordered_set<size_t> invalidatedLines_;
    
    // Track line timestamps (for eviction)
    mutable std::unordered_map<size_t, std::chrono::steady_clock::time_point> lineTimestamps_;
    
    // Track line access times (for LRU)
    mutable std::unordered_map<size_t, std::chrono::steady_clock::time_point> lineAccessTimes_;
    
    // Current visible range of lines
    mutable std::atomic<size_t> visibleStartLine_{0};
    mutable std::atomic<size_t> visibleEndLine_{0};
    
    // Highlighting timeout
    std::atomic<size_t> highlightingTimeoutMs_{DEFAULT_HIGHLIGHTING_TIMEOUT_MS};
    
    // Context lines
    std::atomic<size_t> contextLines_{DEFAULT_CONTEXT_LINES};
    
    // Last processed range for optimization
    ProcessedRange lastProcessedRange_;
    
    // Track processed ranges for optimization
    mutable std::deque<ProcessedRange> processedRanges_;
    
    // Track recently processed lines to avoid duplicate work
    mutable std::unordered_map<size_t, std::chrono::steady_clock::time_point> recentlyProcessedLines_;
    
    // Debug logging flag
    bool debugLoggingEnabled_ = false;
    
    // Static debug logging flag - used for all instances
    static bool globalDebugLoggingEnabled_;
    
    // Thread pool for background highlighting
    std::unique_ptr<ThreadPool> threadPool_;
    
    // Track active tasks for each line to prevent duplicate work
    mutable std::unordered_map<size_t, std::future<void>> activeLineTasks_;
    
    // Track tasks for ranges (startLine-endLine) to prevent duplicate work
    mutable std::unordered_map<std::string, std::future<void>> activeRangeTasks_;
}; 