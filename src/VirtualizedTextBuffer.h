#pragma once

#include "interfaces/ITextBuffer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <chrono>
#include <optional>
#include <atomic>
#include <deque>
#include <unordered_set>
#include <queue>
#include <functional>

/**
 * @brief A text buffer implementation optimized for large files
 * 
 * This class implements the ITextBuffer interface with optimizations
 * for handling large files efficiently. It uses a paging mechanism
 * to load only portions of the file into memory as needed, and
 * maintains an LRU cache to manage memory usage.
 */
class VirtualizedTextBuffer : public ITextBuffer {
public:
    /**
     * @brief Cache eviction policy
     * 
     * Determines the strategy used to decide which pages to evict when the cache is full.
     */
    enum class CacheEvictionPolicy {
        LRU,        ///< Least Recently Used
        SLRU,       ///< Segmented LRU with probationary and protected segments
        ARC,        ///< Adaptive Replacement Cache (balances recency and frequency)
        SPATIAL     ///< Spatial locality aware policy that considers proximity to active pages
    };

    /**
     * @brief Prefetch strategy
     * 
     * Determines how and when pages are prefetched.
     */
    enum class PrefetchStrategy {
        NONE,       ///< No prefetching
        ADJACENT,   ///< Prefetch pages adjacent to the requested page
        PREDICTIVE, ///< Use access patterns to predict which pages to prefetch
        ADAPTIVE    ///< Dynamically adjust prefetching based on hit rate and access patterns
    };

    /**
     * @brief Constructor
     * 
     * Creates an empty virtualized text buffer.
     */
    VirtualizedTextBuffer();

    /**
     * @brief Constructor with file
     * 
     * Creates a virtualized text buffer initialized from a file.
     * 
     * @param filename The path to the file to load
     * @param pageSize The number of lines per page (default: 1000)
     * @param cacheSize The maximum number of pages to keep in memory (default: 10)
     */
    explicit VirtualizedTextBuffer(const std::string& filename, 
                                  size_t pageSize = 1000, 
                                  size_t cacheSize = 10);

    /**
     * @brief Destructor
     */
    ~VirtualizedTextBuffer() override;

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
     * @brief Set the page size
     * 
     * @param pageSize The new page size (number of lines per page)
     */
    void setPageSize(size_t pageSize);

    /**
     * @brief Set the cache size
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
     * @brief Lock for reading
     * 
     * This method acquires a shared lock on the buffer for reading.
     * IMPORTANT: You MUST call unlockReading() when done to prevent deadlocks.
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
     * IMPORTANT: You MUST call unlockWriting() when done to prevent deadlocks.
     */
    void lockForWriting();

    /**
     * @brief Unlock after writing
     * 
     * Call this method to release an exclusive lock acquired with lockForWriting().
     */
    void unlockWriting();

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

    /**
     * @brief Set the cache eviction policy
     * 
     * @param policy The cache eviction policy to use
     */
    void setCacheEvictionPolicy(CacheEvictionPolicy policy);

    /**
     * @brief Get the current cache eviction policy
     * 
     * @return The current cache eviction policy
     */
    CacheEvictionPolicy getCacheEvictionPolicy() const;

    /**
     * @brief Set the prefetch strategy
     * 
     * @param strategy The prefetch strategy to use
     */
    void setPrefetchStrategy(PrefetchStrategy strategy);

    /**
     * @brief Get the current prefetch strategy
     * 
     * @return The current prefetch strategy
     */
    PrefetchStrategy getPrefetchStrategy() const;

    /**
     * @brief Set the prefetch distance
     * 
     * Controls how many pages ahead and behind the current page to prefetch
     * when using adjacent or predictive prefetching.
     * 
     * @param distance The number of pages to prefetch in each direction
     */
    void setPrefetchDistance(size_t distance);

    /**
     * @brief Get the current prefetch distance
     * 
     * @return The current prefetch distance
     */
    size_t getPrefetchDistance() const;

    /**
     * @brief Set the maximum prefetch priority queue size
     * 
     * Controls the maximum number of pages that can be queued for prefetching.
     * 
     * @param size The maximum queue size
     */
    void setMaxPrefetchQueueSize(size_t size);

private:
    /**
     * @brief A page of text data
     */
    struct Page {
        std::vector<std::string> lines;
        std::chrono::steady_clock::time_point lastAccessed;
        bool dirty = false;
        
        // New fields for enhanced caching
        size_t accessCount = 0;       // For frequency-based policies
        bool isPinned = false;        // Pinned pages are not evicted
        double priority = 0.0;        // For priority-based eviction
    };

    /**
     * @brief Represents a page to be prefetched with a priority
     */
    struct PrefetchRequest {
        size_t pageNumber;
        double priority;
        
        // Comparison operator for priority queue (higher priority first)
        bool operator<(const PrefetchRequest& other) const {
            return priority < other.priority;
        }
    };

    /**
     * @brief Get the page number for a line index
     * 
     * @param lineIndex The line index
     * @return The page number
     */
    size_t getPageNumber(size_t lineIndex) const;

    /**
     * @brief Get the line index within a page
     * 
     * @param lineIndex The global line index
     * @return The line index within the page
     */
    size_t getLineIndexInPage(size_t lineIndex) const;

    /**
     * @brief Load a page from disk
     * 
     * @param pageNumber The page number to load
     * @return A shared pointer to the loaded page
     */
    std::shared_ptr<Page> loadPage(size_t pageNumber) const;

    /**
     * @brief Get a page from cache or load it from disk
     * 
     * @param pageNumber The page number to get
     * @return A shared pointer to the page
     */
    std::shared_ptr<Page> getPage(size_t pageNumber) const;

    /**
     * @brief Ensure a page is loaded and return a reference to it
     * 
     * @param pageNumber The page number to ensure is loaded
     * @param forWriting Whether the page will be written to
     * @return A shared pointer to the page
     */
    std::shared_ptr<Page> ensurePage(size_t pageNumber, bool forWriting = false);

    /**
     * @brief Mark a page as dirty
     * 
     * @param pageNumber The page number to mark as dirty
     */
    void markPageDirty(size_t pageNumber);

    /**
     * @brief Save a dirty page to disk
     * 
     * @param pageNumber The page number to save
     * @param page The page to save
     */
    void savePage(size_t pageNumber, const Page& page) const;

    /**
     * @brief Evict a page from cache according to current policy
     */
    void evictPage() const;

    /**
     * @brief Evict the least recently used page from cache
     */
    void evictLRUPage() const;

    /**
     * @brief Evict a page using the Segmented LRU policy
     */
    void evictSLRUPage() const;

    /**
     * @brief Evict a page using the Adaptive Replacement Cache policy
     */
    void evictARCPage() const;

    /**
     * @brief Evict a page using the Spatial locality aware policy
     */
    void evictSpatialPage() const;

    /**
     * @brief Update the access time and other metrics for a page
     * 
     * @param pageNumber The page number that was accessed
     */
    void updatePageAccess(size_t pageNumber) const;

    /**
     * @brief Update the access pattern tracking
     * 
     * @param pageNumber The page number that was accessed
     */
    void updateAccessPattern(size_t pageNumber) const;

    /**
     * @brief Initiate prefetching based on current strategy and recent accesses
     * 
     * @param triggerPageNumber The page number that triggered the prefetch
     */
    void initiateStrategicPrefetch(size_t triggerPageNumber) const;

    /**
     * @brief Prefetch pages adjacent to the specified page
     * 
     * @param pageNumber The page number that triggered the prefetch
     */
    void prefetchAdjacentPages(size_t pageNumber) const;

    /**
     * @brief Prefetch pages based on predicted access patterns
     * 
     * @param pageNumber The page number that triggered the prefetch
     */
    void prefetchPredictivePages(size_t pageNumber) const;

    /**
     * @brief Adaptively prefetch pages based on current access patterns and cache stats
     * 
     * @param pageNumber The page number that triggered the prefetch
     */
    void prefetchAdaptivePages(size_t pageNumber) const;

    /**
     * @brief Evaluate the effectiveness of current prefetching strategy
     * 
     * @return True if the current strategy is effective, false otherwise
     */
    bool evaluatePrefetchEffectiveness() const;

    /**
     * @brief Adjust prefetching parameters based on current effectiveness
     */
    void adjustPrefetchParameters() const;

    /**
     * @brief Calculate the priority of a page for prefetching
     * 
     * @param pageNumber The page number to calculate priority for
     * @param triggerPage The page that triggered the prefetch
     * @return The priority value (higher is more important)
     */
    double calculatePrefetchPriority(size_t pageNumber, size_t triggerPage) const;

    /**
     * @brief Queue a page for prefetching with a given priority
     * 
     * @param pageNumber The page number to prefetch
     * @param priority The priority of the prefetch (higher is more important)
     */
    void queueForPrefetch(size_t pageNumber, double priority) const;

    /**
     * @brief Process the prefetch queue
     * 
     * @param maxPages Maximum number of pages to prefetch in this call
     */
    void processPrefetchQueue(size_t maxPages = 2) const;

    /**
     * @brief Initialize from file
     * 
     * @param filename The path to the file to load
     */
    void initFromFile(const std::string& filename);

    /**
     * @brief Load the index file
     * 
     * @return True if the index file was loaded successfully, false otherwise
     */
    bool loadIndexFile();

    /**
     * @brief Update the index file
     */
    void updateIndexFile() const;

    /**
     * @brief Rebuild the line index by scanning the file
     */
    void rebuildLineIndex();

    /**
     * @brief Get the file offset for a line
     * 
     * @param lineIndex The line index
     * @return The file offset
     */
    std::streampos getFileOffsetForLine(size_t lineIndex) const;

    /**
     * @brief Check if memory usage is high
     * 
     * @return True if memory usage is high, false otherwise
     */
    bool isMemoryUsageHigh() const;

    /**
     * @brief Load a line to the temporary buffer
     * 
     * @param lineIndex The line index
     */
    void loadLineToTemporary(size_t lineIndex) const;

    // File-related members
    bool isFromFile_ = false;                          ///< Whether the buffer is backed by a file
    std::string filename_;                           ///< The file backing the buffer
    std::shared_ptr<std::fstream> fileStream_;       ///< The file stream for file operations
    std::vector<std::streampos> lineOffsets_;         ///< The file offsets for each line

    // Buffer state
    size_t pageSize_;                                 ///< The number of lines per page
    size_t cacheSize_;                                ///< The maximum number of pages to keep in memory
    size_t totalLines_;                               ///< The total number of lines in the buffer
    mutable std::string temporaryLine_;               ///< Temporary line buffer for read operations
    std::atomic<bool> modified_ = false;               ///< Whether the buffer has been modified

    // Caching
    mutable std::unordered_map<size_t, std::shared_ptr<Page>> pageCache_; ///< The page cache
    mutable std::vector<size_t> lruList_;             ///< List of page numbers in LRU order

    // New caching members for SLRU
    mutable std::deque<size_t> probationarySegment_;  ///< Probationary segment for SLRU
    mutable std::deque<size_t> protectedSegment_;     ///< Protected segment for SLRU
    
    // New caching members for ARC
    mutable std::unordered_set<size_t> recentlyUsed_; ///< Recently used pages for ARC
    mutable std::unordered_set<size_t> frequentlyUsed_; ///< Frequently used pages for ARC
    mutable std::unordered_set<size_t> ghostRecent_;  ///< Ghost cache of recently evicted pages
    mutable std::unordered_set<size_t> ghostFrequent_; ///< Ghost cache of frequently used pages
    mutable double arcP_ = 0.0;                       ///< Adaptive size parameter for ARC

    // New caching members for Spatial
    mutable std::unordered_map<size_t, double> spatialScores_; ///< Spatial locality scores

    // Access pattern tracking
    mutable std::deque<size_t> recentAccesses_;       ///< Queue of recently accessed pages
    mutable size_t recentAccessesMaxSize_ = 100;      ///< Maximum size of the recent accesses queue
    mutable std::unordered_map<size_t, std::unordered_map<size_t, size_t>> transitionCounts_; ///< Page transition counts for predictive prefetching

    // Prefetching
    mutable std::priority_queue<PrefetchRequest> prefetchQueue_; ///< Queue of pages to prefetch
    mutable size_t maxPrefetchQueueSize_ = 10;        ///< Maximum size of the prefetch queue
    mutable size_t prefetchDistance_ = 1;             ///< How many pages to prefetch in each direction
    mutable size_t prefetchHits_ = 0;                 ///< Number of cache hits on prefetched pages
    mutable size_t prefetchMisses_ = 0;               ///< Number of prefetched pages that weren't used
    
    // Cache policy settings
    CacheEvictionPolicy evictionPolicy_ = CacheEvictionPolicy::LRU; ///< Current cache eviction policy
    PrefetchStrategy prefetchStrategy_ = PrefetchStrategy::ADJACENT; ///< Current prefetch strategy

    // Statistics
    mutable size_t cacheHits_ = 0;                    ///< Number of cache hits
    mutable size_t cacheMisses_ = 0;                  ///< Number of cache misses

    // Thread safety
    mutable std::shared_mutex mutex_;                 ///< Mutex for thread safety
}; 