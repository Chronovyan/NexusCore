#ifdef _WIN32
#define NOMINMAX  // Prevent Windows min/max macros from conflicting with std::min/std::max
#endif

#include "SyntaxHighlightingManager.h"
#include <iostream>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif
#include "EditorError.h"
#include <queue>
#include <sstream>

// Initialize static members
bool SyntaxHighlightingManager::debugLoggingEnabled_{false};

// Global variable for test-mode logging control
// This is defined as extern in test headers and set by the test framework
bool DISABLE_ALL_LOGGING_FOR_TESTS = false;

namespace {
    // Improved logger function using ErrorReporter with cleaner logic
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // Disable warnings about potentially unsafe format strings
#endif
    template<typename... Args>
    void logManagerMessage(EditorException::Severity severity, const char* location, const char* format, Args... args) {
        // Early exit conditions - check all suppressions first
        // 1. Check global test flag (defined at file level)
        if (DISABLE_ALL_LOGGING_FOR_TESTS) {
            return;
        }
        
        // 2. Check if warnings should be suppressed
        if (ErrorReporter::suppressAllWarnings && 
            (severity == EditorException::Severity::Warning || severity == EditorException::Severity::Debug)) {
            return;
        }
        
        // 3. Check if debug messages are enabled
        if (!SyntaxHighlightingManager::isDebugLoggingEnabled_static() && 
            (severity == EditorException::Severity::Warning || severity == EditorException::Severity::Debug)) {
            return;
        }

        try {
            // Format the thread ID and location prefix
            std::stringstream ss_prefix;
            ss_prefix << "[Thread " << GetCurrentThreadId() << "] ";
            ss_prefix << location << ": ";
            
            // Format the message content
            int needed = std::snprintf(nullptr, 0, format, args...);
            if (needed < 0) {
                ErrorReporter::logError("logManagerMessage: Error in snprintf determining size.");
                return;
            }

            std::string msg_content;
            msg_content.resize(needed + 1); // +1 for null terminator
            std::snprintf(&msg_content[0], msg_content.size(), format, args...);
            msg_content.resize(needed); // Remove null terminator for string operations

            std::string full_message = ss_prefix.str() + msg_content;

            // Route to appropriate logging method based on severity
            switch (severity) {
                case EditorException::Severity::Warning:
                    ErrorReporter::logWarning(full_message);
                    break;
                case EditorException::Severity::Debug:
                    // For Debug messages, use logWarning but with "Debug:" prefix
                    ErrorReporter::logWarning("Debug: " + full_message);
                    break;
                case EditorException::Severity::Error:
                case EditorException::Severity::Critical:
                default:
                    // All other severities go to error log
                    ErrorReporter::logError(full_message);
                    break;
            }
        } catch (const std::exception& log_ex) {
            // Fallback to cerr if ErrorReporter itself fails
            std::cerr << "CRITICAL LOGGING FAILURE in logManagerMessage: " << log_ex.what() << std::endl;
        } catch (...) {
            std::cerr << "CRITICAL LOGGING FAILURE in logManagerMessage: Unknown exception." << std::endl;
        }
    }
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
}

SyntaxHighlightingManager::SyntaxHighlightingManager() {
    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::Constructor", "Instance created.");
}

SyntaxHighlightingManager::~SyntaxHighlightingManager() {
    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "[ENTRY] Destructor called.");

    if (highlighter_) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ initial use_count: %ld", highlighter_.use_count());
    } else {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ is initially null.");
    }

    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Attempting to acquire mutex in destructor...");
    try {
        std::unique_lock lock(mutex_); 
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Mutex acquired successfully in destructor.");

        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "cachedStyles_ size: %zu, capacity: %zu", cachedStyles_.size(), cachedStyles_.capacity());
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "invalidatedLines_ size: %zu", invalidatedLines_.size());
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "lineTimestamps_ size: %zu", lineTimestamps_.size());
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "lineAccessTimes_ size: %zu", lineAccessTimes_.size());

        if (highlighter_) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Resetting highlighter_ manually. Current use_count: %ld", highlighter_.use_count());
            highlighter_.reset();
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ has been reset.");
        } else {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ was already null before explicit reset attempt.");
        }
        
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Clearing cachedStyles_ manually.");
        cachedStyles_.clear(); 
        cachedStyles_.shrink_to_fit(); // Explicitly return memory
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "cachedStyles_ cleared. New size: %zu, capacity: %zu", cachedStyles_.size(), cachedStyles_.capacity());

        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Clearing all containers manually.");
        invalidatedLines_.clear();
        lineTimestamps_.clear();
        lineAccessTimes_.clear();
        recentlyProcessedLines_.clear();
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "All containers cleared.");
    } catch (const std::system_error& e) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "std::system_error acquiring mutex in destructor: %s (code: %d)", e.what(), e.code().value());
    } catch (const std::exception& e) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "std::exception during cleanup in destructor: %s", e.what());
    } catch (...) {
        logManagerMessage(EditorException::Severity::Critical, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Unknown exception during cleanup in destructor.");
    }
    
    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "[EXIT] Destructor finished.");
}

// Helper function to format duration for logging
std::string formatDuration(const std::chrono::steady_clock::time_point& start) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    return std::to_string(duration.count()) + " ms";
}

// Fix the logVectorAccess method to handle location and index correctly
void SyntaxHighlightingManager::logVectorAccess(const char* location, size_t index, size_t vectorSize) const {
    if (index >= vectorSize) {
        logManagerMessage(EditorException::Severity::Error,
                        "SyntaxHighlightingManager::logVectorAccess",
                        "INVALID VECTOR ACCESS in %s: index %zu >= size %zu",
                     location, index, vectorSize);
    }
}

// Thread-safe access to buffer_
const TextBuffer* SyntaxHighlightingManager::getBuffer() const {
    return buffer_.load(std::memory_order_acquire);
}

// Get highlighter pointer WITHOUT locking - for internal use when mutex is already held
SyntaxHighlighter* SyntaxHighlightingManager::getHighlighterPtr_nolock() const {
    return highlighter_.get();
}

void SyntaxHighlightingManager::setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) {
    try {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setHighlighter", "Attempting to set highlighter. Pointer: %p", static_cast<void*>(highlighter.get()));
        std::unique_lock lock(mutex_);
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setHighlighter", "Mutex acquired. Immediately invalidating all lines.");
        invalidateAllLines_nolock(); // Aggressive invalidation
        highlighter_ = highlighter; // Simple assignment
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setHighlighter", "Highlighter set and lines invalidated.");
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setHighlighter: ") + ex.what(), EditorException::Severity::Error));
    }
}

std::shared_ptr<SyntaxHighlighter> SyntaxHighlightingManager::getHighlighter() const {
    try {
        std::shared_lock lock(mutex_); // Using shared lock for read-only access
        return highlighter_;
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("getHighlighter: ") + ex.what(), EditorException::Severity::Error));
        return nullptr;
    }
}

void SyntaxHighlightingManager::setEnabled(bool enabled) {
    try {
        enabled_.store(enabled, std::memory_order_release);
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setEnabled", 
                          "Syntax highlighting %s", enabled ? "enabled" : "disabled");
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setEnabled: ") + ex.what(), EditorException::Severity::Error));
    }
}

bool SyntaxHighlightingManager::isEnabled() const {
    return enabled_.load(std::memory_order_acquire);
}

void SyntaxHighlightingManager::setBuffer(const TextBuffer* buffer) {
    try {
        // Atomically update the buffer pointer
        buffer_.store(buffer, std::memory_order_release);
        
        // Then invalidate all lines with the full lock
        if (buffer) {
            std::unique_lock lock(mutex_);
            invalidateAllLines_nolock();
        }
        
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setBuffer", 
                          "Buffer set to %p", static_cast<const void*>(buffer));
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setBuffer: ") + ex.what(), EditorException::Severity::Error));
    }
}

std::unique_ptr<std::vector<SyntaxStyle>> SyntaxHighlightingManager::highlightLine_nolock(size_t line) {
    // This method assumes the caller already holds a unique (write) lock
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Get the current highlighter
    SyntaxHighlighter* highlighter = getHighlighterPtr_nolock();
    if (!highlighter) {
        return nullptr;
    }
    
    // Get the buffer safely (this is thread-safe because it uses atomics internally)
    const TextBuffer* buffer = getBuffer();
    if (!buffer) {
        return nullptr;
    }
    
    try {
        // Check if line is within valid range
        if (line >= buffer->lineCount()) {
            if (debugLoggingEnabled_) {
                logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLine_nolock", 
                                  "Line %zu is out of range (buffer has %zu lines)", line, buffer->lineCount());
            }
            return nullptr;
        }
        
        // Get the line text from the buffer
        const std::string& lineText = buffer->getLine(line);
        
        // Create a new vector for the highlighting styles
        auto styles = std::make_unique<std::vector<SyntaxStyle>>();
        
        // Highlight the line
        auto result = highlighter->highlightLine(lineText, line);
        if (result) {
            *styles = std::move(*result);
        }
        
        // Update timestamp and mark as processed
        markAsRecentlyProcessed(line);
        
        if (debugLoggingEnabled_) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startTime);
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLine_nolock", 
                              "Highlighted line %zu in %lld μs", line, duration.count());
        }
        
        return styles;
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::highlightLine_nolock", 
                          "Exception highlighting line %zu: %s", line, ex.what());
        return nullptr;
    }
}

void SyntaxHighlightingManager::highlightLine(size_t line) {
    auto startTime = std::chrono::steady_clock::now();
    
    if (!isEnabled()) {
        return;
    }
    
    try {
        // Acquire a unique lock for writing
        std::unique_lock lock(mutex_);
        
        // Ensure line is in cache range
        if (cachedStyles_.size() <= line) {
            cachedStyles_.resize(line + 1);
        }
        
        // Highlight the line and update cache
        auto styles = highlightLine_nolock(line);
        if (styles) {
            // Create cache entry with the new styles
            cachedStyles_[line] = std::make_unique<CacheEntry>(std::move(*styles));
            
            // Update timestamps
            auto now = std::chrono::steady_clock::now();
            lineTimestamps_[line] = now;
            lineAccessTimes_[line] = now;
            
            // Remove from invalidated lines set if present
            invalidatedLines_.erase(line);
        }
        
        if (debugLoggingEnabled_) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startTime);
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLine", 
                              "Total line %zu highlighting operation took %lld μs", line, duration.count());
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::highlightLine", 
                          "Exception highlighting line %zu: %s", line, ex.what());
    }
}

bool SyntaxHighlightingManager::highlightLines_nolock(size_t startLine, size_t endLine, 
                                                    const std::chrono::milliseconds& timeout) {
    // This method assumes the caller already holds a unique (write) lock
    
    auto startTime = std::chrono::steady_clock::now();
    auto timeoutPoint = startTime + timeout;
    
    // Get the buffer safely
    const TextBuffer* buffer = getBuffer();
    if (!buffer) {
        return false;
    }
    
    // Clamp endLine to buffer size
    size_t bufferLineCount = buffer->lineCount();
    if (bufferLineCount == 0) {
        return true; // Nothing to do for empty buffer
    }
    
    size_t effectiveEndLine = std::min(endLine, bufferLineCount - 1);
    if (startLine > effectiveEndLine) {
        return true; // Invalid range, nothing to do
    }
    
    // Prepare for highlighting
    size_t totalLinesRequested = effectiveEndLine - startLine + 1;
    size_t linesProcessed = 0;
    size_t validLineCount = 0;
    
    // Ensure cache has enough space
    if (cachedStyles_.size() <= effectiveEndLine) {
        cachedStyles_.resize(effectiveEndLine + 1);
    }
    
    // Get highlighter
    SyntaxHighlighter* highlighter = getHighlighterPtr_nolock();
    if (!highlighter) {
        return false;
    }
    
    // Process lines in priority order:
    // 1. Visible lines
    // 2. Lines around visible area
    // 3. Other requested lines
    
    // Track lines that need highlighting
    std::vector<size_t> linesToHighlight;
    linesToHighlight.reserve(totalLinesRequested);
    
    // First collect all invalid lines in the range
    for (size_t line = startLine; line <= effectiveEndLine; ++line) {
        bool isInvalidated = invalidatedLines_.count(line) > 0;
        bool isCached = line < cachedStyles_.size() && 
                      cachedStyles_[line] && 
                      cachedStyles_[line]->valid.load(std::memory_order_acquire);
        
        if (isInvalidated || !isCached) {
            linesToHighlight.push_back(line);
        } else {
            validLineCount++;
        }
    }
    
    // Sort lines by priority (visible first, then by distance from visible area)
    size_t visibleStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visibleEnd = visibleEndLine_.load(std::memory_order_acquire);
    
    auto lineComparator = [visibleStart, visibleEnd](size_t a, size_t b) {
        bool aVisible = (a >= visibleStart && a <= visibleEnd);
        bool bVisible = (b >= visibleStart && b <= visibleEnd);
        
        if (aVisible && !bVisible) return true;
        if (!aVisible && bVisible) return false;
        if (aVisible && bVisible) return a < b; // In visible area, process in order
        
        // Both outside visible area, process by distance from visible area
        size_t aDist = std::min(a < visibleStart ? visibleStart - a : a - visibleEnd, 
                               a > visibleEnd ? a - visibleEnd : visibleStart - a);
        size_t bDist = std::min(b < visibleStart ? visibleStart - b : b - visibleEnd,
                               b > visibleEnd ? b - visibleEnd : visibleStart - b);
        
        return aDist < bDist;
    };
    
    std::sort(linesToHighlight.begin(), linesToHighlight.end(), lineComparator);
    
    // Process lines until timeout
    size_t processedInBatch = 0;
    bool timeoutReached = false;
    
    for (size_t line : linesToHighlight) {
        // Process at least MIN_LINES_PER_BATCH lines before checking timeout
        if (processedInBatch >= MIN_LINES_PER_BATCH && 
            std::chrono::steady_clock::now() > timeoutPoint) {
            timeoutReached = true;
            break;
        }
        
        // Highlight the line
        auto styles = highlightLine_nolock(line);
        if (styles) {
            // Create cache entry with the new styles
            cachedStyles_[line] = std::make_unique<CacheEntry>(std::move(*styles));
            
            // Update timestamps
            auto now = std::chrono::steady_clock::now();
            lineTimestamps_[line] = now;
            lineAccessTimes_[line] = now;
            
            // Remove from invalidated lines set
            invalidatedLines_.erase(line);
            
            // Update tracking
            linesProcessed++;
            processedInBatch++;
            validLineCount++;
        }
    }
    
    // Update the last processed range for optimization
    if (linesProcessed > 0) {
        lastProcessedRange_.update(startLine, effectiveEndLine);
    }
    
    // Check if we should clean up the cache
    if (cachedStyles_.size() > MAX_CACHE_LINES) {
        evictLRUEntries_nolock(static_cast<size_t>(MAX_CACHE_LINES * CACHE_CLEANUP_RATIO));
    }
    
    // Log performance metrics if debug logging is enabled
    if (debugLoggingEnabled_) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
        
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                         "Processed %zu/%zu lines in %lld ms (%zu valid, %s)",
                         linesProcessed, totalLinesRequested, duration.count(), validLineCount,
                         timeoutReached ? "timeout reached" : "completed");
    }
    
    // Return true if all requested lines were processed
    return linesProcessed == linesToHighlight.size();
}

std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(
    size_t startLine, size_t endLine) const {
    
    // Fast path: highlighting disabled
    if (!isEnabled()) {
        return {};
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        // First try with a shared lock (read-only)
        std::shared_lock sharedLock(mutex_);
        
        // Get the buffer safely
        const TextBuffer* buffer = getBuffer();
        if (!buffer) {
            return {};
        }
        
        // Check if all requested lines are valid in the cache
        bool allLinesValid = true;
        size_t bufferLineCount = buffer->lineCount();
        
        // Clamp endLine to buffer size
        size_t effectiveEndLine = std::min(endLine, bufferLineCount > 0 ? bufferLineCount - 1 : 0);
        if (startLine > effectiveEndLine) {
            return {}; // Empty range
        }
        
        // Check if all lines are valid in cache
        for (size_t line = startLine; line <= effectiveEndLine; ++line) {
            // Update access time to prevent cache eviction
            lineAccessTimes_[line] = std::chrono::steady_clock::now();
            
            if (line >= cachedStyles_.size() || 
                !cachedStyles_[line] || 
                !cachedStyles_[line]->valid.load(std::memory_order_acquire) ||
                invalidatedLines_.count(line) > 0) {
                allLinesValid = false;
                break;
            }
        }
        
        if (allLinesValid) {
            // All lines are valid, just return them
            std::vector<std::vector<SyntaxStyle>> result;
            result.reserve(effectiveEndLine - startLine + 1);
            
            for (size_t line = startLine; line <= effectiveEndLine; ++line) {
                result.push_back(cachedStyles_[line]->styles);
            }
            
            if (debugLoggingEnabled_) {
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - startTime);
                logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::getHighlightingStyles",
                                 "Fast path: returned %zu cached lines in %lld μs",
                                 result.size(), duration.count());
            }
            
            return result;
        }
        
        // We need to update some lines, release shared lock and acquire unique lock
        sharedLock.unlock();
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::getHighlightingStyles",
                         "Exception in shared lock section: %s", ex.what());
    }
    
    // Slow path: acquire unique lock and update cache as needed
    try {
        std::unique_lock uniqueLock(mutex_);
        
        // Get the buffer safely
        const TextBuffer* buffer = getBuffer();
        if (!buffer) {
            return {};
        }
        
        // Clamp endLine to buffer size
        size_t bufferLineCount = buffer->lineCount();
        if (bufferLineCount == 0) {
            return {}; // Empty buffer
        }
        
        size_t effectiveEndLine = std::min(endLine, bufferLineCount - 1);
        if (startLine > effectiveEndLine) {
            return {}; // Empty range
        }
        
        // Ensure cache has enough space
        if (cachedStyles_.size() <= effectiveEndLine) {
            cachedStyles_.resize(effectiveEndLine + 1);
        }
        
        // Highlight lines as needed with timeout
        auto timeout = std::chrono::milliseconds(highlightingTimeoutMs_.load(std::memory_order_acquire));
        const_cast<SyntaxHighlightingManager*>(this)->highlightLines_nolock(startLine, effectiveEndLine, timeout);
        
        // Collect results
        std::vector<std::vector<SyntaxStyle>> result;
        result.reserve(effectiveEndLine - startLine + 1);
        
        for (size_t line = startLine; line <= effectiveEndLine; ++line) {
            if (line < cachedStyles_.size() && 
                cachedStyles_[line] && 
                cachedStyles_[line]->valid.load(std::memory_order_acquire)) {
                // Use cached styles
                result.push_back(cachedStyles_[line]->styles);
            } else {
                // Line not highlighted, use empty styles
                result.push_back({});
            }
            
            // Update access time to prevent cache eviction
            lineAccessTimes_[line] = std::chrono::steady_clock::now();
        }
        
        if (debugLoggingEnabled_) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::getHighlightingStyles",
                             "Slow path: returned %zu lines in %lld ms",
                             result.size(), duration.count());
        }
        
        return result;
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::getHighlightingStyles",
                         "Exception in unique lock section: %s", ex.what());
        return {};
    }
}

// Implementation for non-const getHighlightingStyles
std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(size_t startLine, size_t endLine) {
    // Same implementation as const version, but will update cache as needed
    return const_cast<const SyntaxHighlightingManager*>(this)->getHighlightingStyles(startLine, endLine);
}

void SyntaxHighlightingManager::invalidateLine(size_t line) {
    if (!isEnabled()) {
        return;
    }
    
    try {
        std::unique_lock lock(mutex_);
        
        // Add to invalidated lines set
        invalidatedLines_.insert(line);
        
        // If line is in cache, mark it as invalid
        if (line < cachedStyles_.size() && cachedStyles_[line]) {
            cachedStyles_[line]->valid.store(false, std::memory_order_release);
        }
        
        // Invalidate the last processed range if it contains this line
        if (lastProcessedRange_.valid && 
            line >= lastProcessedRange_.startLine && 
            line <= lastProcessedRange_.endLine) {
            lastProcessedRange_.invalidate();
        }
        
        if (debugLoggingEnabled_) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::invalidateLine",
                             "Invalidated line %zu", line);
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::invalidateLine",
                         "Exception invalidating line %zu: %s", line, ex.what());
    }
}

void SyntaxHighlightingManager::invalidateLines(size_t startLine, size_t endLine) {
    if (!isEnabled() || startLine > endLine) {
        return;
    }
    
    try {
        std::unique_lock lock(mutex_);
        
        // Get maximum line count safely
        const TextBuffer* buffer = getBuffer();
        if (buffer) {
            size_t bufferLineCount = buffer->lineCount();
            if (bufferLineCount > 0) {
                endLine = std::min(endLine, bufferLineCount - 1);
            } else {
                return; // Empty buffer
            }
        }
        
        // Invalidate each line in the range
        for (size_t line = startLine; line <= endLine; ++line) {
            // Add to invalidated lines set
            invalidatedLines_.insert(line);
            
            // If line is in cache, mark it as invalid
            if (line < cachedStyles_.size() && cachedStyles_[line]) {
                cachedStyles_[line]->valid.store(false, std::memory_order_release);
            }
        }
        
        // Invalidate the last processed range if it overlaps with this range
        if (lastProcessedRange_.valid && 
            !(endLine < lastProcessedRange_.startLine || startLine > lastProcessedRange_.endLine)) {
            lastProcessedRange_.invalidate();
        }
        
        if (debugLoggingEnabled_) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::invalidateLines",
                             "Invalidated lines %zu-%zu", startLine, endLine);
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::invalidateLines",
                         "Exception invalidating lines %zu-%zu: %s", startLine, endLine, ex.what());
    }
}

void SyntaxHighlightingManager::invalidateAllLines() {
    if (!isEnabled()) {
        return;
    }
    
    try {
        std::unique_lock lock(mutex_);
        invalidateAllLines_nolock();
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::invalidateAllLines",
                         "Exception invalidating all lines: %s", ex.what());
    }
}

void SyntaxHighlightingManager::invalidateAllLines_nolock() {
    // This method assumes the caller already holds a unique (write) lock
    
    // Get maximum line count safely
    const TextBuffer* buffer = getBuffer();
    if (buffer) {
        size_t bufferLineCount = buffer->lineCount();
        
        // Add all lines to invalidated lines set
        for (size_t line = 0; line < bufferLineCount; ++line) {
            invalidatedLines_.insert(line);
        }
        
        // Mark all cached lines as invalid
        for (size_t line = 0; line < cachedStyles_.size(); ++line) {
            if (cachedStyles_[line]) {
                cachedStyles_[line]->valid.store(false, std::memory_order_release);
            }
        }
    }
    
    // Invalidate the last processed range
    lastProcessedRange_.invalidate();
    
    if (debugLoggingEnabled_) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::invalidateAllLines_nolock",
                         "Invalidated all lines");
    }
}

void SyntaxHighlightingManager::setVisibleRange(size_t startLine, size_t endLine) const {
    visibleStartLine_.store(startLine, std::memory_order_release);
    visibleEndLine_.store(endLine, std::memory_order_release);
    
    // Update last processed range if needed
    if (lastProcessedRange_.valid) {
        const_cast<SyntaxHighlightingManager*>(this)->lastProcessedRange_.update(startLine, endLine);
    }
}

void SyntaxHighlightingManager::setHighlightingTimeout(size_t timeoutMs) {
    // Atomic update, no need for lock
    highlightingTimeoutMs_.store(timeoutMs, std::memory_order_release);
    
    if (debugLoggingEnabled_) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setHighlightingTimeout",
                         "Set highlighting timeout to %zu ms", timeoutMs);
    }
}

size_t SyntaxHighlightingManager::getHighlightingTimeout() const {
    return highlightingTimeoutMs_.load(std::memory_order_acquire);
}

void SyntaxHighlightingManager::setContextLines(size_t contextLines) {
    // Atomic update, no need for lock
    contextLines_.store(contextLines, std::memory_order_release);
    
    if (debugLoggingEnabled_) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setContextLines",
                         "Set context lines to %zu", contextLines);
    }
}

size_t SyntaxHighlightingManager::getContextLines() const {
    return contextLines_.load(std::memory_order_acquire);
}

bool SyntaxHighlightingManager::isLineInCache(size_t line) const {
    std::shared_lock lock(mutex_);
    return line < cachedStyles_.size() && cachedStyles_[line] != nullptr;
}

bool SyntaxHighlightingManager::isLineValid(size_t line) const {
    std::shared_lock lock(mutex_);
    
    if (line < cachedStyles_.size() && cachedStyles_[line]) {
        return cachedStyles_[line]->valid.load(std::memory_order_acquire) && 
               invalidatedLines_.count(line) == 0;
    }
    
    return false;
}

std::pair<size_t, size_t> SyntaxHighlightingManager::calculateOptimalProcessingRange(
    size_t requestedStart, size_t requestedEnd) const {
    
    // Get buffer line count
    const TextBuffer* buffer = getBuffer();
    if (!buffer) {
        return {requestedStart, requestedEnd};
    }
    
    size_t bufferLineCount = buffer->lineCount();
    if (bufferLineCount == 0) {
        return {0, 0}; // Empty buffer
    }
    
    // Clamp requested range to buffer size
    size_t clampedStart = std::min(requestedStart, bufferLineCount - 1);
    size_t clampedEnd = std::min(requestedEnd, bufferLineCount - 1);
    
    // If requested range is empty, return as-is
    if (clampedStart > clampedEnd) {
        return {clampedStart, clampedEnd};
    }
    
    // Get visible range
    size_t visibleStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visibleEnd = visibleEndLine_.load(std::memory_order_acquire);
    
    // Clamp visible range to buffer size
    visibleStart = std::min(visibleStart, bufferLineCount - 1);
    visibleEnd = std::min(visibleEnd, bufferLineCount - 1);
    
    // Calculate context range around visible area
    size_t contextSize = contextLines_.load(std::memory_order_acquire);
    size_t contextStart = (visibleStart > contextSize) ? (visibleStart - contextSize) : 0;
    size_t contextEnd = std::min(visibleEnd + contextSize, bufferLineCount - 1);
    
    // If requested range overlaps with context range, expand to cover both
    if (clampedStart <= contextEnd && clampedEnd >= contextStart) {
        return {std::min(clampedStart, contextStart), std::max(clampedEnd, contextEnd)};
    }
    
    // Otherwise, return requested range as-is
    return {clampedStart, clampedEnd};
}

bool SyntaxHighlightingManager::isEntryExpired_nolock(size_t line) const {
    // This method assumes the caller already holds a lock
    
    if (line >= cachedStyles_.size() || !cachedStyles_[line]) {
        return true;
    }
    
    // Check if the line is in the invalidated set
    if (invalidatedLines_.count(line) > 0) {
        return true;
    }
    
    // Check if the cache entry is marked as invalid
    if (!cachedStyles_[line]->valid.load(std::memory_order_acquire)) {
        return true;
    }
    
    // Check if the cache entry has expired
    auto now = std::chrono::steady_clock::now();
    auto timestamp = cachedStyles_[line]->timestamp;
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
    
    return age > CACHE_ENTRY_LIFETIME_MS;
}

void SyntaxHighlightingManager::evictLRUEntries_nolock(size_t targetSize) const {
    // This method assumes the caller already holds a unique (write) lock
    
    // No need to do anything if we're under target
    if (cachedStyles_.size() <= targetSize) {
        return;
    }
    
    // Copy access times to a vector for sorting
    std::vector<std::pair<size_t, std::chrono::steady_clock::time_point>> lineAccessPairs;
    lineAccessPairs.reserve(lineAccessTimes_.size());
    
    for (const auto& pair : lineAccessTimes_) {
        lineAccessPairs.push_back(pair);
    }
    
    // Sort by access time (oldest first)
    std::sort(lineAccessPairs.begin(), lineAccessPairs.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });
    
    // Number of entries to remove
    size_t entriesToRemove = cachedStyles_.size() - targetSize;
    size_t removedCount = 0;
    
    // Remove entries from cache
    for (const auto& pair : lineAccessPairs) {
        size_t line = pair.first;
        
        // Skip visible lines
        size_t visibleStart = visibleStartLine_.load(std::memory_order_acquire);
        size_t visibleEnd = visibleEndLine_.load(std::memory_order_acquire);
        if (line >= visibleStart && line <= visibleEnd) {
            continue;
        }
        
        // Skip lines that are already invalid or nullptr
        if (line >= cachedStyles_.size() || !cachedStyles_[line]) {
            continue;
        }
        
        // Mark entry as invalid
        cachedStyles_[line]->valid.store(false, std::memory_order_release);
        
        // In debug mode, we keep the data but mark as invalid
        // In release mode, we can free the memory
        #ifndef _DEBUG
        cachedStyles_[line].reset();
        #endif
        
        removedCount++;
        if (removedCount >= entriesToRemove) {
            break;
        }
    }
    
    // Log cache eviction if debug logging is enabled
    if (debugLoggingEnabled_) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::evictLRUEntries_nolock", 
                          "Evicted %zu entries from highlighting cache", removedCount);
    }
    
    // If we couldn't evict enough entries through LRU, we may need to force eviction
    if (removedCount < entriesToRemove && cachedStyles_.size() > targetSize) {
        size_t remainingToRemove = entriesToRemove - removedCount;
        size_t forceRemoved = 0;
        
        for (size_t i = 0; i < cachedStyles_.size() && forceRemoved < remainingToRemove; ++i) {
            // Skip visible lines and already invalid entries
            size_t visibleStart = visibleStartLine_.load(std::memory_order_acquire);
            size_t visibleEnd = visibleEndLine_.load(std::memory_order_acquire);
            if ((i >= visibleStart && i <= visibleEnd) || !cachedStyles_[i]) {
                continue;
            }
            
            // Mark entry as invalid
            cachedStyles_[i]->valid.store(false, std::memory_order_release);
            
            // In debug mode, we keep the data but mark as invalid
            // In release mode, we can free the memory
            #ifndef _DEBUG
            cachedStyles_[i].reset();
            #endif
            
            forceRemoved++;
        }
        
        // Log forced eviction if debug logging is enabled
        if (debugLoggingEnabled_ && forceRemoved > 0) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::evictLRUEntries_nolock", 
                              "Forced eviction of %zu additional entries", forceRemoved);
        }
    }
}

bool SyntaxHighlightingManager::wasRecentlyProcessed(size_t line) const {
    std::shared_lock lock(mutex_);
    
    auto it = recentlyProcessedLines_.find(line);
    if (it == recentlyProcessedLines_.end()) {
        return false;
    }
    
    // Check if the timestamp is recent enough
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    
    return age <= RECENTLY_PROCESSED_WINDOW_MS;
}

void SyntaxHighlightingManager::markAsRecentlyProcessed(size_t line) {
    auto now = std::chrono::steady_clock::now();
    recentlyProcessedLines_[line] = now;
    
    // Clean up old entries (no lock needed as this is called within a locked context)
    if (recentlyProcessedLines_.size() > MAX_CACHE_LINES) {
        std::vector<size_t> linesToRemove;
        
        for (const auto& [l, timestamp] : recentlyProcessedLines_) {
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
            if (age > RECENTLY_PROCESSED_WINDOW_MS) {
                linesToRemove.push_back(l);
            }
        }
        
        for (size_t l : linesToRemove) {
            recentlyProcessedLines_.erase(l);
        }
    }
}

void SyntaxHighlightingManager::logCacheMetrics(const char* context, size_t visibleLines, size_t totalProcessed) const {
    if (!debugLoggingEnabled_) {
        return;
    }
    
    // This method assumes the caller already holds a lock
    
    size_t cacheSize = 0;
    size_t validEntries = 0;
    size_t invalidEntries = 0;
    
    for (size_t i = 0; i < cachedStyles_.size(); ++i) {
        if (cachedStyles_[i]) {
            cacheSize++;
            if (cachedStyles_[i]->valid.load(std::memory_order_acquire)) {
                validEntries++;
            } else {
                invalidEntries++;
            }
        }
    }
    
    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::logCacheMetrics",
                    "%s: cache stats - size: %zu, valid: %zu, invalid: %zu, invalidated set: %zu, "
                    "visible lines: %zu, processed: %zu, timestamps: %zu, access times: %zu",
                    context, cacheSize, validEntries, invalidEntries, invalidatedLines_.size(),
                    visibleLines, totalProcessed, lineTimestamps_.size(), lineAccessTimes_.size());
}

void SyntaxHighlightingManager::setDebugLoggingEnabled(bool enabled) {
    setDebugLoggingEnabled_static(enabled);
}

bool SyntaxHighlightingManager::isDebugLoggingEnabled() const {
    return isDebugLoggingEnabled_static();
}

void SyntaxHighlightingManager::setDebugLoggingEnabled_static(bool enabled) {
    debugLoggingEnabled_ = enabled;
    
    // Log state change - but only if we're enabling (to avoid paradox when disabling)
    if (enabled) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setDebugLoggingEnabled_static",
                        "Debug logging %s", enabled ? "enabled" : "disabled");
    }
}

bool SyntaxHighlightingManager::isDebugLoggingEnabled_static() {
    return debugLoggingEnabled_;
}

