#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Configuration for text buffer behavior
 * 
 * This class provides configuration options for text buffers,
 * particularly for the virtualized text buffer implementation.
 */
class TextBufferConfig {
public:
    /**
     * @brief Constructor with default values
     */
    TextBufferConfig()
        : useVirtualizedBufferForLargeFiles(true)
        , largeFileThresholdBytes(10 * 1024 * 1024)  // 10MB
        , defaultPageSize(1000)
        , defaultCacheSize(10)
        , maxMemoryUsageMB(200)                      // 200MB max memory usage
        , prefetchEnabled(true)
        , prefetchWindowSize(5)                      // 5 pages before and after current view
    {}
    
    /**
     * @brief Whether to automatically use virtualized buffer for large files
     */
    bool useVirtualizedBufferForLargeFiles;
    
    /**
     * @brief File size threshold in bytes for considering a file "large"
     */
    size_t largeFileThresholdBytes;
    
    /**
     * @brief Default page size in lines for virtualized buffers
     */
    size_t defaultPageSize;
    
    /**
     * @brief Default cache size in pages for virtualized buffers
     */
    size_t defaultCacheSize;
    
    /**
     * @brief Maximum memory usage in MB for virtualized buffers
     */
    size_t maxMemoryUsageMB;
    
    /**
     * @brief Whether to enable prefetching of nearby pages
     */
    bool prefetchEnabled;
    
    /**
     * @brief Number of pages to prefetch before and after current view
     */
    size_t prefetchWindowSize;
}; 