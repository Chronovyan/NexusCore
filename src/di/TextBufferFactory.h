#pragma once

#include <memory>
#include <string>
#include <fstream>
#include "interfaces/ITextBuffer.hpp"
#include "TextBuffer.h"
#include "ThreadSafeTextBuffer.h"
#include "VirtualizedTextBuffer.h"
#include "ThreadSafeVirtualizedTextBuffer.h"
#include "TextBufferConfig.h"

/**
 * @brief Factory class for creating different text buffer implementations
 * 
 * This factory provides methods to create various text buffer implementations:
 * - Basic TextBuffer: Simple in-memory buffer, not thread-safe
 * - ThreadSafeTextBuffer: Basic buffer with thread safety
 * - VirtualizedTextBuffer: Performance-optimized buffer for large files
 * - ThreadSafeVirtualizedTextBuffer: Thread-safe version of the virtualized buffer
 */
class TextBufferFactory {
public:
    /**
     * @brief Create a basic text buffer
     * @param filename Optional filename to load
     * @return A shared pointer to an ITextBuffer implementation
     */
    static std::shared_ptr<ITextBuffer> createBasicTextBuffer(const std::string& filename = "") {
        if (filename.empty()) {
            return std::make_shared<TextBuffer>();
        } else {
            return std::make_shared<TextBuffer>(filename);
        }
    }

    /**
     * @brief Create a thread-safe text buffer
     * @param filename Optional filename to load
     * @return A shared pointer to an ITextBuffer implementation
     */
    static std::shared_ptr<ITextBuffer> createThreadSafeTextBuffer(const std::string& filename = "") {
        if (filename.empty()) {
            return std::make_shared<ThreadSafeTextBuffer>();
        } else {
            return std::make_shared<ThreadSafeTextBuffer>(filename);
        }
    }

    /**
     * @brief Create a virtualized text buffer optimized for large files
     * @param filename Optional filename to load
     * @param pageSize Size of each page in lines (default: 1000)
     * @param cacheSize Number of pages to keep in memory (default: 10)
     * @return A shared pointer to an ITextBuffer implementation
     */
    static std::shared_ptr<ITextBuffer> createVirtualizedTextBuffer(
        const std::string& filename = "", 
        size_t pageSize = 1000, 
        size_t cacheSize = 10) {
        
        if (filename.empty()) {
            auto buffer = std::make_shared<VirtualizedTextBuffer>();
            buffer->setPageSize(pageSize);
            buffer->setCacheSize(cacheSize);
            return buffer;
        } else {
            return std::make_shared<VirtualizedTextBuffer>(filename, pageSize, cacheSize);
        }
    }

    /**
     * @brief Create a thread-safe virtualized text buffer optimized for large files
     * @param filename Optional filename to load
     * @param pageSize Size of each page in lines (default: 1000)
     * @param cacheSize Number of pages to keep in memory (default: 10)
     * @return A shared pointer to an ITextBuffer implementation
     */
    static std::shared_ptr<ITextBuffer> createThreadSafeVirtualizedTextBuffer(
        const std::string& filename = "", 
        size_t pageSize = 1000, 
        size_t cacheSize = 10) {
        
        if (filename.empty()) {
            auto buffer = std::make_shared<ThreadSafeVirtualizedTextBuffer>();
            buffer->setPageSize(pageSize);
            buffer->setCacheSize(cacheSize);
            return buffer;
        } else {
            return std::make_shared<ThreadSafeVirtualizedTextBuffer>(filename, pageSize, cacheSize);
        }
    }

    /**
     * @brief Create the default text buffer based on configuration or file size
     * 
     * This method determines the best text buffer implementation based on:
     * - For empty buffers: basic thread-safe buffer
     * - For small files (<threshold): basic thread-safe buffer
     * - For large files (≥threshold): thread-safe virtualized buffer
     * 
     * @param filename Optional filename to load
     * @param config Text buffer configuration, used to determine settings like threshold size
     * @return A shared pointer to an ITextBuffer implementation
     */
    static std::shared_ptr<ITextBuffer> createDefaultTextBuffer(
        const std::string& filename = "",
        const TextBufferConfig& config = TextBufferConfig()) {
        
        // For empty buffers, use the basic thread-safe implementation
        if (filename.empty()) {
            return createThreadSafeTextBuffer();
        }
        
        // If virtualized buffers are disabled in config, always use thread-safe buffer
        if (!config.useVirtualizedBufferForLargeFiles) {
            return createThreadSafeTextBuffer(filename);
        }
        
        // Check file size to determine the best implementation
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            // If file can't be opened, default to basic thread-safe buffer
            return createThreadSafeTextBuffer();
        }
        
        // Get file size
        const size_t fileSize = file.tellg();
        file.close();
        
        // For files larger than the threshold, use virtualized buffer
        if (fileSize >= config.largeFileThresholdBytes) {
            return createThreadSafeVirtualizedTextBuffer(
                filename, 
                config.defaultPageSize, 
                config.defaultCacheSize);
        } else {
            return createThreadSafeTextBuffer(filename);
        }
    }
}; 