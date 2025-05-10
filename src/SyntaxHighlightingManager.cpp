#include "SyntaxHighlightingManager.h"
#include <iostream>
#include <thread>

namespace {
    // Logger function using the standardized error handling approach
    template<typename... Args>
    void logManager(const char* location, const char* format, Args... args) {
        try {
            // Get thread ID for better debugging
            std::stringstream ss;
            ss << "[Thread " << std::this_thread::get_id() << "] ";
            ss << location << ": ";
            
            // Format message - in real code, use std::format (C++20) or a formatting library
            char buffer[256];
            snprintf(buffer, sizeof(buffer), format, args...);
            ss << buffer;
            
            std::cerr << ss.str() << std::endl;
        } catch (...) {
            // Last resort - silently continue if we can't even log
        }
    }
}

SyntaxHighlightingManager::SyntaxHighlightingManager()
    : buffer_(nullptr),
      highlighter_(std::make_shared<std::atomic<SyntaxHighlighter*>>(nullptr)),
      enabled_(true),
      visibleStartLine_(0), 
      visibleEndLine_(0),
      highlightingTimeoutMs_(DEFAULT_HIGHLIGHTING_TIMEOUT_MS),
      contextLines_(DEFAULT_CONTEXT_LINES) {
}

// Thread-safe getter for buffer
const TextBuffer* SyntaxHighlightingManager::getBuffer() const {
    return buffer_.load(std::memory_order_acquire);
}

void SyntaxHighlightingManager::setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) {
    try {
        std::scoped_lock lock(mutex_);
        
        // Store raw pointer in the atomic
        SyntaxHighlighter* raw_ptr = highlighter.get();
        highlighter_->store(raw_ptr, std::memory_order_release);
        
        invalidateAllLines_nolock();
    } catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::setHighlighter", ex, ErrorSeverity::Major);
    } catch (...) {
        handleError("SyntaxHighlightingManager::setHighlighter", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Critical);
    }
}

std::shared_ptr<SyntaxHighlighter> SyntaxHighlightingManager::getHighlighter() const {
    try {
        // Use shared lock for reading
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        // We can only return the raw pointer - in a real implementation, we'd need
        // to maintain a map of raw pointers to shared_ptrs or use a different approach
        SyntaxHighlighter* raw_ptr = highlighter_->load(std::memory_order_acquire);
        
        // This is a simplification - in a real implementation, we'd need proper lifetime management
        // For now, treating this as a weak reference (non-owning)
        return std::shared_ptr<SyntaxHighlighter>(raw_ptr, [](SyntaxHighlighter*){});
    } catch (const std::exception& ex) {
        return handleError<std::shared_ptr<SyntaxHighlighter>>(
            "SyntaxHighlightingManager::getHighlighter", 
            ex, ErrorSeverity::Major, nullptr);
    } catch (...) {
        return handleError<std::shared_ptr<SyntaxHighlighter>>(
            "SyntaxHighlightingManager::getHighlighter", 
            std::runtime_error("Unknown exception"), ErrorSeverity::Critical, nullptr);
    }
}

void SyntaxHighlightingManager::setEnabled(bool enabled) {
    try {
        std::scoped_lock lock(mutex_);
        bool oldValue = enabled_.load(std::memory_order_acquire);
        if (oldValue != enabled) {
            enabled_.store(enabled, std::memory_order_release);
            invalidateAllLines_nolock();
        }
    } catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::setEnabled", ex, ErrorSeverity::Major);
    } catch (...) {
        handleError("SyntaxHighlightingManager::setEnabled", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Critical);
    }
}

bool SyntaxHighlightingManager::isEnabled() const {
    try {
        // Atomic read doesn't need a lock
        return enabled_.load(std::memory_order_acquire);
    } catch (const std::exception& ex) {
        return handleError<bool>("SyntaxHighlightingManager::isEnabled", 
                                ex, ErrorSeverity::Major, false);
    } catch (...) {
        return handleError<bool>("SyntaxHighlightingManager::isEnabled", 
                                std::runtime_error("Unknown exception"), ErrorSeverity::Critical, false);
    }
}

void SyntaxHighlightingManager::setBuffer(const TextBuffer* buffer) {
    try {
        std::scoped_lock lock(mutex_);
        buffer_.store(buffer, std::memory_order_release);
        invalidateAllLines_nolock();
    } catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::setBuffer", ex, ErrorSeverity::Major);
    } catch (...) {
        handleError("SyntaxHighlightingManager::setBuffer", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Critical);
    }
}

std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(
    size_t startLine, size_t endLine) {
    try {
        std::scoped_lock lock(mutex_);
        
        // Get buffer and check enabled status
        const TextBuffer* buffer = getBuffer();
        SyntaxHighlighter* highlighter = highlighter_->load(std::memory_order_acquire);
        bool enabled = enabled_.load(std::memory_order_acquire);
        
        // Check if highlighting is disabled or no buffer/highlighter
        if (!enabled || !buffer || !highlighter) {
            return std::vector<std::vector<SyntaxStyle>>(
                endLine - startLine + 1, std::vector<SyntaxStyle>());
        }
        
        // Safety checks
        if (buffer->isEmpty()) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        // Clamp end line to buffer size
        endLine = std::min(endLine, buffer->lineCount() - 1);
        
        // If start line is beyond end line, return empty result
        if (startLine > endLine) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        // For non-const version, update the cache for the effective range
        auto effectiveRange = calculateEffectiveRange(startLine, endLine);
        size_t effectiveStart = effectiveRange.first;
        size_t effectiveEnd = effectiveRange.second;
        
        // Highlight invalidated lines within the effective range
        std::chrono::milliseconds timeout(highlightingTimeoutMs_.load(std::memory_order_acquire));
        highlightLines(effectiveStart, effectiveEnd, timeout);
        
        // Periodically clean up the cache
        cleanupCache();
        
        // Extract the requested range from the cache
        std::vector<std::vector<SyntaxStyle>> result;
        result.reserve(endLine - startLine + 1);
        
        for (size_t line = startLine; line <= endLine; ++line) {
            if (isLineInCache(line) && isLineValid(line)) {
                // Use the styles from the cache entry
                auto& cacheEntry = cachedStyles_[line];
                if (cacheEntry && cacheEntry->valid.load(std::memory_order_acquire)) {
                    result.push_back(cacheEntry->styles);
                } else {
                    result.push_back(std::vector<SyntaxStyle>());
                }
            } else {
                // For lines that weren't highlighted (due to timeout), return empty styles
                result.push_back(std::vector<SyntaxStyle>());
            }
        }
        
        return result;
    } catch (const std::exception& ex) {
        return handleError<std::vector<std::vector<SyntaxStyle>>>(
            "SyntaxHighlightingManager::getHighlightingStyles", 
            ex, ErrorSeverity::Major, std::vector<std::vector<SyntaxStyle>>());
    } catch (...) {
        return handleError<std::vector<std::vector<SyntaxStyle>>>(
            "SyntaxHighlightingManager::getHighlightingStyles", 
            std::runtime_error("Unknown exception"), ErrorSeverity::Critical,
            std::vector<std::vector<SyntaxStyle>>());
    }
}

void SyntaxHighlightingManager::invalidateLine(size_t line) {
    std::scoped_lock lock(mutex_);
    
    if (line < cachedStyles_.size() && cachedStyles_[line]) {
        // Mark the cache entry as invalid
        cachedStyles_[line]->valid.store(false, std::memory_order_release);
        invalidatedLines_.insert(line);
        lineTimestamps_.erase(line);
    }
}

void SyntaxHighlightingManager::invalidateLines(size_t startLine, size_t endLine) {
    std::scoped_lock lock(mutex_);
    
    const TextBuffer* buffer = getBuffer();
    if (!buffer) return;
    
    // Clamp to buffer size
    endLine = std::min(endLine, buffer->lineCount() - 1);
    
    for (size_t line = startLine; line <= endLine; ++line) {
        if (line < cachedStyles_.size() && cachedStyles_[line]) {
            // Mark the cache entry as invalid
            cachedStyles_[line]->valid.store(false, std::memory_order_release);
        }
        invalidatedLines_.insert(line);
        lineTimestamps_.erase(line);
    }
}

void SyntaxHighlightingManager::invalidateAllLines() {
    std::scoped_lock lock(mutex_);
    invalidateAllLines_nolock();
}

void SyntaxHighlightingManager::invalidateAllLines_nolock() {
    const TextBuffer* buffer = getBuffer();
    if (!buffer) return;
    
    // Mark all cache entries as invalid
    for (auto& entry : cachedStyles_) {
        if (entry) {
            entry->valid.store(false, std::memory_order_release);
        }
    }
    
    // Clear the cache and invalidate all lines
    cachedStyles_.clear();
    invalidatedLines_.clear();
    lineTimestamps_.clear();
    
    if (buffer->lineCount() > 0) {
        for (size_t i = 0; i < buffer->lineCount(); ++i) {
            invalidatedLines_.insert(i);
        }
    }
}

void SyntaxHighlightingManager::setVisibleRange(size_t startLine, size_t endLine) {
    try {
        // Atomic stores don't need a lock
        visibleStartLine_.store(startLine, std::memory_order_release);
        visibleEndLine_.store(endLine, std::memory_order_release);
    } catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::setVisibleRange", ex, ErrorSeverity::Minor);
    } catch (...) {
        handleError("SyntaxHighlightingManager::setVisibleRange", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Major);
    }
}

void SyntaxHighlightingManager::setHighlightingTimeout(size_t timeoutMs) {
    try {
        // Atomic store doesn't need a lock
        highlightingTimeoutMs_.store(timeoutMs, std::memory_order_release);
    } catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::setHighlightingTimeout", ex, ErrorSeverity::Minor);
    } catch (...) {
        handleError("SyntaxHighlightingManager::setHighlightingTimeout", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Major);
    }
}

size_t SyntaxHighlightingManager::getHighlightingTimeout() const {
    try {
        // Atomic load doesn't need a lock
        return highlightingTimeoutMs_.load(std::memory_order_acquire);
    } catch (const std::exception& ex) {
        return handleError<size_t>("SyntaxHighlightingManager::getHighlightingTimeout", 
                                 ex, ErrorSeverity::Minor, DEFAULT_HIGHLIGHTING_TIMEOUT_MS);
    } catch (...) {
        return handleError<size_t>("SyntaxHighlightingManager::getHighlightingTimeout", 
                                 std::runtime_error("Unknown exception"), 
                                 ErrorSeverity::Major, DEFAULT_HIGHLIGHTING_TIMEOUT_MS);
    }
}

void SyntaxHighlightingManager::setContextLines(size_t contextLines) {
    try {
        // Atomic store doesn't need a lock
        contextLines_.store(contextLines, std::memory_order_release);
    } catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::setContextLines", ex, ErrorSeverity::Minor);
    } catch (...) {
        handleError("SyntaxHighlightingManager::setContextLines", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Major);
    }
}

size_t SyntaxHighlightingManager::getContextLines() const {
    try {
        // Atomic load doesn't need a lock
        return contextLines_.load(std::memory_order_acquire);
    } catch (const std::exception& ex) {
        return handleError<size_t>("SyntaxHighlightingManager::getContextLines", 
                                 ex, ErrorSeverity::Minor, DEFAULT_CONTEXT_LINES);
    } catch (...) {
        return handleError<size_t>("SyntaxHighlightingManager::getContextLines", 
                                 std::runtime_error("Unknown exception"), 
                                 ErrorSeverity::Major, DEFAULT_CONTEXT_LINES);
    }
}

void SyntaxHighlightingManager::highlightLine(size_t line) {
    const TextBuffer* buffer = getBuffer();
    SyntaxHighlighter* highlighter = highlighter_->load(std::memory_order_acquire);
    bool enabled = enabled_.load(std::memory_order_acquire);
    
    if (!buffer || !highlighter || !enabled) return;
    
    // Safety check
    if (line >= buffer->lineCount()) return;
    
    try {
        // Get the line text
        const std::string& lineText = buffer->getLine(line);
        
        // Highlight the line
        std::vector<SyntaxStyle> lineStyles = highlighter->highlightLine(lineText, line);
        
        // Ensure cache is large enough
        if (cachedStyles_.size() <= line) {
            cachedStyles_.resize(line + 1);
        }
        
        // Create a new cache entry with the highlighted styles
        auto newEntry = std::make_shared<CacheEntry>(std::move(lineStyles));
        cachedStyles_[line] = newEntry;
        
        // Update timestamp
        lineTimestamps_[line] = newEntry->timestamp;
        
        // Remove from invalidated set
        invalidatedLines_.erase(line);
    }
    catch (const std::exception& ex) {
        handleError("SyntaxHighlightingManager::highlightLine", ex, ErrorSeverity::Minor);
    }
    catch (...) {
        handleError("SyntaxHighlightingManager::highlightLine", 
                   std::runtime_error("Unknown exception"), ErrorSeverity::Major);
    }
}

bool SyntaxHighlightingManager::highlightLines(size_t startLine, size_t endLine, 
                                               const std::chrono::milliseconds& timeout) {
    const TextBuffer* buffer = getBuffer();
    SyntaxHighlighter* highlighter = highlighter_->load(std::memory_order_acquire);
    bool enabled = enabled_.load(std::memory_order_acquire);
    
    if (!buffer || !highlighter || !enabled) return false;
    
    // Safety checks
    if (buffer->isEmpty()) return false;
    
    // Clamp end line to buffer size
    endLine = std::min(endLine, buffer->lineCount() - 1);
    
    // If start line is beyond end line, return
    if (startLine > endLine) return false;
    
    // Start timing
    auto startTime = std::chrono::steady_clock::now();
    
    // Create a priority list of lines to process
    std::vector<size_t> linesToProcess;
    
    // First add visible invalidated lines
    size_t visStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visEnd = visibleEndLine_.load(std::memory_order_acquire);
    
    for (size_t line = visStart; line <= visEnd && line <= endLine; ++line) {
        if (invalidatedLines_.find(line) != invalidatedLines_.end()) {
            linesToProcess.push_back(line);
        }
    }
    
    // Then add non-visible invalidated lines
    for (size_t line = startLine; line <= endLine; ++line) {
        if (line >= visStart && line <= visEnd) {
            continue; // Skip visible lines already added
        }
        
        if (invalidatedLines_.find(line) != invalidatedLines_.end()) {
            linesToProcess.push_back(line);
        }
    }
    
    // Process the lines in priority order with timeout
    for (size_t i = 0; i < linesToProcess.size(); ++i) {
        highlightLine(linesToProcess[i]);
        
        // Check timeout
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime);
        
        if (elapsed >= timeout) {
            // Return partial success if we processed visible lines
            return i >= (visEnd - visStart + 1);
        }
    }
    
    return true; // All lines processed
}

bool SyntaxHighlightingManager::isLineInCache(size_t line) const {
    return line < cachedStyles_.size() && cachedStyles_[line] != nullptr;
}

bool SyntaxHighlightingManager::isLineValid(size_t line) const {
    return isLineInCache(line) && 
           cachedStyles_[line]->valid.load(std::memory_order_acquire) &&
           invalidatedLines_.find(line) == invalidatedLines_.end();
}

std::pair<size_t, size_t> SyntaxHighlightingManager::calculateEffectiveRange(
    size_t startLine, size_t endLine) const {
    
    const TextBuffer* buffer = getBuffer();
    if (!buffer) return {startLine, endLine};
    
    size_t contextL = contextLines_.load(std::memory_order_acquire);
    
    // Start with the requested range
    size_t effectiveStart = startLine;
    size_t effectiveEnd = endLine;
    
    // Add context lines before
    if (startLine >= contextL) {
        effectiveStart = startLine - contextL;
    } else {
        effectiveStart = 0;
    }
    
    // Add context lines after
    effectiveEnd = std::min(endLine + contextL, buffer->lineCount() - 1);
    
    return {effectiveStart, effectiveEnd};
}

void SyntaxHighlightingManager::cleanupCache() {
    if (lineTimestamps_.empty()) return;
    
    auto now = std::chrono::steady_clock::now();
    std::vector<size_t> linesToInvalidate;
    
    size_t visStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visEnd = visibleEndLine_.load(std::memory_order_acquire);
    
    // Find old entries
    for (const auto& entry : lineTimestamps_) {
        size_t line = entry.first;
        auto timestamp = entry.second;
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
        
        // Skip lines in visible range
        if (line >= visStart && line <= visEnd) {
            continue;
        }
        
        // Invalidate if too old
        if (age.count() > CACHE_ENTRY_LIFETIME_MS) {
            linesToInvalidate.push_back(line);
        }
    }
    
    // Remove from cache and timestamp tracking
    for (size_t line : linesToInvalidate) {
        if (line < cachedStyles_.size() && cachedStyles_[line]) {
            // Mark as invalid rather than clearing (lock-free readers can still access it)
            cachedStyles_[line]->valid.store(false, std::memory_order_release);
            // Consider: cachedStyles_[line].reset(); // Free memory
        }
        lineTimestamps_.erase(line);
        invalidatedLines_.insert(line);
    }
} 