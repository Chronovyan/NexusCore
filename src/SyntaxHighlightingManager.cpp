#ifdef _WIN32
#define NOMINMAX  // Prevent Windows min/max macros from conflicting with std::min/std::max
#endif

#include "LoggingCompatibility.h"
#include "SyntaxHighlightingManager.h"
#include <iostream>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif
#include "EditorError.h"
#include <queue>
#include <sstream>
#include "AppDebugLog.h"
#include "EditorErrorReporter.h"
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include "EditContext.h"
#include "TextUtils.h"

// Define the time constants
constexpr size_t RECENT_PROCESSED_TTL_SECONDS = 60; // 1 minute

// Initialize static members
bool SyntaxHighlightingManager::globalDebugLoggingEnabled_ = false;

// Global variable for test-mode logging control
// This is defined as extern in test headers and set by the test framework
bool DISABLE_ALL_LOGGING_FOR_TESTS = false;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // Disable warnings about potentially unsafe format strings
#endif
template<typename... Args>
void SyntaxHighlightingManager::logManagerMessage(EditorException::Severity severity, const char* location, const char* format, Args... args) const {
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
    if (!SyntaxHighlightingManager::getGlobalDebugLoggingState() && 
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
            case EditorException::Severity::EDITOR_ERROR:
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

SyntaxHighlightingManager::SyntaxHighlightingManager()
    : buffer_(nullptr)
    , enabled_(true)
    , highlightingTimeoutMs_(DEFAULT_HIGHLIGHTING_TIMEOUT_MS)
    , contextLines_(DEFAULT_CONTEXT_LINES)
    , debugLoggingEnabled_(globalDebugLoggingEnabled_)
    , threadPool_(std::make_unique<ThreadPool>(DEFAULT_THREAD_POOL_SIZE))
{
    LOG_DEBUG("SyntaxHighlightingManager created");
}

SyntaxHighlightingManager::~SyntaxHighlightingManager() {
    try {
        // Clear any pending tasks
        if (threadPool_) {
            threadPool_->shutdown();
        }
        
        // Clear all data structures to ensure clean release
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            cachedStyles_.clear();
            invalidatedLines_.clear();
            lineTimestamps_.clear();
            lineAccessTimes_.clear();
            processedRanges_.clear();
            recentlyProcessedLines_.clear();
            activeLineTasks_.clear();
            activeRangeTasks_.clear();
        }
        
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::~SyntaxHighlightingManager",
                         "SyntaxHighlightingManager destroyed");
    } catch (const std::exception& ex) {
        // Log but don't throw from destructor
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::~SyntaxHighlightingManager",
                         "Exception in destructor: %s", ex.what());
    } catch (...) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::~SyntaxHighlightingManager",
                         "Unknown exception in destructor");
    }
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR,
                        "SyntaxHighlightingManager::logVectorAccess",
                        "INVALID VECTOR ACCESS in %s: index %zu >= size %zu",
                     location, index, vectorSize);
    }
}

// Thread-safe access to buffer_
const ITextBuffer* SyntaxHighlightingManager::getBuffer() const {
    return buffer_.load(std::memory_order_acquire);
}

// Get highlighter pointer WITHOUT locking - for internal use when mutex is already held
SyntaxHighlighter* SyntaxHighlightingManager::getHighlighterPtr_nolock() const {
    return highlighter_.get();
}

void SyntaxHighlightingManager::setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) {
    try {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        
        // Set the new highlighter
        highlighter_ = highlighter;
        
        // Invalidate all cached styles
        invalidateAllLines_nolock();
        
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setHighlighter", 
                          "Highlighter set to %p", static_cast<void*>(highlighter.get()));
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::setHighlighter",
                         "Exception setting highlighter: %s", ex.what());
    }
}

std::shared_ptr<SyntaxHighlighter> SyntaxHighlightingManager::getHighlighter() const {
    try {
        std::shared_lock lock(mutex_); // Using shared lock for read-only access
        return highlighter_;
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::getHighlighter",
                         "Exception getting highlighter: %s", ex.what());
        return nullptr;
    }
}

void SyntaxHighlightingManager::setEnabled(bool enabled) {
    try {
        enabled_.store(enabled, std::memory_order_release);
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setEnabled", 
                          "Syntax highlighting %s", enabled ? "enabled" : "disabled");
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::setEnabled",
                         "Exception setting enabled state: %s", ex.what());
    }
}

bool SyntaxHighlightingManager::isEnabled() const {
    return enabled_.load(std::memory_order_acquire);
}

void SyntaxHighlightingManager::setBuffer(const ITextBuffer* buffer) {
    try {
        // Cancel any pending tasks first
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            activeLineTasks_.clear();
            activeRangeTasks_.clear();
        }
        
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::setBuffer",
                         "Exception setting buffer: %s", ex.what());
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
    const ITextBuffer* buffer = getBuffer();
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::highlightLine_nolock", 
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::highlightLine", 
                          "Exception highlighting line %zu: %s", line, ex.what());
    }
}

bool SyntaxHighlightingManager::highlightLines_nolock(
    size_t startLine, size_t endLine, const std::chrono::milliseconds& timeout) {
    try {
        // Record the start time to enforce the timeout
        auto startTime = std::chrono::steady_clock::now();
        
        // Get buffer and highlighter
        const ITextBuffer* buffer = getBuffer();
        SyntaxHighlighter* highlighter = getHighlighterPtr_nolock();
        
        if (!buffer || !highlighter) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                         "Cannot highlight: buffer or highlighter is null");
            return false;
        }
        
        // Ensure the cache is large enough
        if (cachedStyles_.size() <= endLine) {
            cachedStyles_.resize(endLine + 1);
        }
        
        // Determine if this range includes the visible area
        bool containsVisibleArea = false;
        {
            size_t visibleStart = visibleStartLine_.load(std::memory_order_acquire);
            size_t visibleEnd = visibleEndLine_.load(std::memory_order_acquire);
            
            if (startLine <= visibleEnd && endLine >= visibleStart) {
                containsVisibleArea = true;
            }
        }
        
        // Process lines, prioritizing visible ones if this range contains the visible area
        if (containsVisibleArea) {
            // Get visible range
            size_t visibleStart = visibleStartLine_.load(std::memory_order_acquire);
            size_t visibleEnd = visibleEndLine_.load(std::memory_order_acquire);
            
            // Process visible lines first
            size_t visibleStartProcessing = std::max(startLine, visibleStart);
            size_t visibleEndProcessing = std::min(endLine, visibleEnd);
            
            // Process the visible range first (higher priority)
            for (size_t line = visibleStartProcessing; line <= visibleEndProcessing; ++line) {
                // Check timeout
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed > timeout) {
                    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                                     "Timeout reached after processing visible lines %zu-%zu (elapsed: %lld ms)",
                                     visibleStartProcessing, line - 1,
                                     std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                    return false; // Timeout reached
                }
                
                // Check if the line needs highlighting
                if (line >= cachedStyles_.size() || 
                    !cachedStyles_[line] || 
                    !cachedStyles_[line]->valid.load(std::memory_order_acquire) ||
                    invalidatedLines_.count(line) > 0) {
                    
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
                    }
                }
            }
            
            // Process lines before visible area
            for (size_t line = startLine; line < visibleStartProcessing; ++line) {
                // Check timeout
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed > timeout) {
                    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                                     "Timeout reached after processing before-visible lines %zu-%zu (elapsed: %lld ms)",
                                     startLine, line - 1,
                                     std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                    return false; // Timeout reached
                }
                
                // Check if the line needs highlighting
                if (line >= cachedStyles_.size() || 
                    !cachedStyles_[line] || 
                    !cachedStyles_[line]->valid.load(std::memory_order_acquire) ||
                    invalidatedLines_.count(line) > 0) {
                    
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
                    }
                }
            }
            
            // Process lines after visible area
            for (size_t line = visibleEndProcessing + 1; line <= endLine; ++line) {
                // Check timeout
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed > timeout) {
                    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                                     "Timeout reached after processing after-visible lines %zu-%zu (elapsed: %lld ms)",
                                     visibleEndProcessing + 1, line - 1,
                                     std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                    return false; // Timeout reached
                }
                
                // Check if the line needs highlighting
                if (line >= cachedStyles_.size() || 
                    !cachedStyles_[line] || 
                    !cachedStyles_[line]->valid.load(std::memory_order_acquire) ||
                    invalidatedLines_.count(line) > 0) {
                    
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
                    }
                }
            }
        } else {
            // Process lines sequentially
            for (size_t line = startLine; line <= endLine; ++line) {
                // Check timeout
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed > timeout) {
                    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                                     "Timeout reached after processing lines %zu-%zu (elapsed: %lld ms)",
                                     startLine, line - 1,
                                     std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                    return false; // Timeout reached
                }
                
                // Check if the line needs highlighting
                if (line >= cachedStyles_.size() || 
                    !cachedStyles_[line] || 
                    !cachedStyles_[line]->valid.load(std::memory_order_acquire) ||
                    invalidatedLines_.count(line) > 0) {
                    
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
                    }
                }
            }
        }
        
        if (debugLoggingEnabled_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);
            
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::highlightLines_nolock",
                             "Processed lines %zu-%zu in %lld ms (all lines processed)",
                             startLine, endLine, elapsed.count());
        }
        
        // Update processed range tracking
        ProcessedRange range;
        range.startLine = startLine;
        range.endLine = endLine;
        range.timestamp = std::chrono::steady_clock::now();
        processedRanges_.push_back(range);
        
        // Limit the size of the processed ranges queue
        while (processedRanges_.size() > MAX_PROCESSED_RANGES) {
            processedRanges_.pop_front();
        }
        
        return true; // All lines processed
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::highlightLines_nolock",
                         "Exception highlighting lines: %s", ex.what());
        return false; // Error occurred
    }
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
        const ITextBuffer* buffer = getBuffer();
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
        
        // If all lines are valid, we can return them without upgrading the lock
        if (allLinesValid) {
            // Collect results with shared lock
            std::vector<std::vector<SyntaxStyle>> result;
            result.reserve(effectiveEndLine - startLine + 1);
            
            for (size_t line = startLine; line <= effectiveEndLine; ++line) {
                result.push_back(cachedStyles_[line]->styles);
            }
            
            if (debugLoggingEnabled_) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
                logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::getHighlightingStyles",
                                 "Fast path: returned %zu lines in %lld ms",
                                 result.size(), duration.count());
            }
            
            // Schedule background processing for the context area around this range
            if (threadPool_ && shouldQueueTask(TaskType::CONTEXT_RANGE)) {
                // Release shared lock before spawning async task
                sharedLock.unlock();
                
                // Calculate optimal processing range including context
                auto optimalRange = calculateOptimalProcessingRange(startLine, effectiveEndLine);
                
                // Process asynchronously if range is larger than the requested range
                if (optimalRange.first < startLine || optimalRange.second > effectiveEndLine) {
                    const_cast<SyntaxHighlightingManager*>(this)->
                        processVisibleRangeAsync(optimalRange.first, optimalRange.second);
                }
            }
            
            return result;
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::getHighlightingStyles",
                         "Exception in shared lock section: %s", ex.what());
        return {};
    }
    
    // If we got here, some lines need highlighting, so we need to upgrade to a unique lock
    try {
        std::unique_lock uniqueLock(mutex_);
        
        // Get the buffer safely
        const ITextBuffer* buffer = getBuffer();
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
        
        // Process immediately with the current thread (we already have the lock)
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
        
        // Check if there are lines that still need highlighting
        bool needsBackgroundProcessing = false;
        for (size_t line = startLine; line <= effectiveEndLine; ++line) {
            if (line >= cachedStyles_.size() || 
                !cachedStyles_[line] || 
                !cachedStyles_[line]->valid.load(std::memory_order_acquire) ||
                invalidatedLines_.count(line) > 0) {
                needsBackgroundProcessing = true;
                break;
            }
        }
        
        // If we couldn't process all lines in time, schedule background processing
        if (needsBackgroundProcessing && threadPool_ && shouldQueueTask(TaskType::VISIBLE_RANGE)) {
            // Schedule the visible range for background processing
            processVisibleRangeAsync(startLine, effectiveEndLine);
        }
        
        // Also process the context area in the background
        if (threadPool_ && shouldQueueTask(TaskType::CONTEXT_RANGE)) {
            // Calculate optimal processing range including context
            auto optimalRange = calculateOptimalProcessingRange(startLine, effectiveEndLine);
            
            // Process asynchronously if range is larger than the requested range
            if (optimalRange.first < startLine || optimalRange.second > effectiveEndLine) {
                processVisibleRangeAsync(optimalRange.first, optimalRange.second);
            }
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::getHighlightingStyles",
                         "Exception in unique lock section: %s", ex.what());
        return {};
    }
}

// Get highlighting styles for a range of lines (non-const version)
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::invalidateLine",
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
        const ITextBuffer* buffer = getBuffer();
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::invalidateLines",
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
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::invalidateAllLines",
                         "Exception invalidating all lines: %s", ex.what());
    }
}

void SyntaxHighlightingManager::invalidateAllLines_nolock() {
    // This method assumes the caller already holds a unique (write) lock
    
    // Get maximum line count safely
    const ITextBuffer* buffer = getBuffer();
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
    // Update atomic values - no need for locking
    visibleStartLine_.store(startLine, std::memory_order_release);
    visibleEndLine_.store(endLine, std::memory_order_release);
    
    if (debugLoggingEnabled_) {
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setVisibleRange", 
                         "Visible range set to %zu-%zu", startLine, endLine);
    }
    
    // Process the visible range in the background using the thread pool
    if (threadPool_ && isEnabled()) {
        // Calculate optimal processing range to include context
        auto optimalRange = calculateOptimalProcessingRange(startLine, endLine);
        
        // Schedule background processing of the visible range with high priority
        processVisibleRangeAsync(optimalRange.first, optimalRange.second);
        
        if (debugLoggingEnabled_) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setVisibleRange",
                             "Scheduled background processing for visible range %zu-%zu (expanded to %zu-%zu)",
                             startLine, endLine, optimalRange.first, optimalRange.second);
        }
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
    std::shared_lock lock(this->mutex_);
    return line < this->cachedStyles_.size() && 
           this->cachedStyles_[line] != nullptr;
}

bool SyntaxHighlightingManager::isLineValid(size_t line) const {
    std::shared_lock lock(this->mutex_);
    
    if (line >= this->cachedStyles_.size() || 
        !this->cachedStyles_[line]) {
        return false;
    }
    
    return this->cachedStyles_[line]->valid.load(std::memory_order_acquire) && 
           this->invalidatedLines_.count(line) == 0;
}

// Calculate the optimal processing range based on a given visible range
std::pair<size_t, size_t> SyntaxHighlightingManager::calculateOptimalProcessingRange(
    size_t startLine, size_t endLine) const {
    
    try {
        // Get context lines size
        size_t contextSize = contextLines_.load(std::memory_order_acquire);
        
        // Get buffer safely
        const ITextBuffer* buffer = getBuffer();
        if (!buffer) {
            return {startLine, endLine}; // No buffer, just return original range
        }
        
        // Get buffer size
        size_t bufferLineCount = buffer->lineCount();
        if (bufferLineCount == 0) {
            return {0, 0}; // Empty buffer
        }
        
        // Calculate optimal start line (include context before)
        size_t optimalStart = (startLine > contextSize) ? (startLine - contextSize) : 0;
        
        // Calculate optimal end line (include context after)
        size_t optimalEnd = std::min(endLine + contextSize, bufferLineCount - 1);
        
        return {optimalStart, optimalEnd};
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::calculateOptimalProcessingRange",
                         "Exception calculating optimal range: %s", ex.what());
        return {startLine, endLine}; // Fall back to original range
    }
}

bool SyntaxHighlightingManager::isEntryExpired_nolock(size_t line) const {
    // Check if it's in the invalidated set
    if (invalidatedLines_.count(line) > 0) {
        return true;  // Explicitly invalidated
    }
    
    // Check if the timestamp exists for this line
    auto timestampIt = lineTimestamps_.find(line);
    if (timestampIt == lineTimestamps_.end()) {
        return true;  // No timestamp, consider expired
    }
    
    // Calculate how old the entry is
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestampIt->second).count();
    
    // Return true if older than cache lifetime (cast to unsigned for comparison)
    return static_cast<size_t>(age) > 60000; // 1 minute lifetime
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
    
    // Cast to unsigned for comparison
    return static_cast<size_t>(age) <= 5000; // 5 seconds window
}

void SyntaxHighlightingManager::markAsRecentlyProcessed(size_t line) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Add timestamp for the processed line
    recentlyProcessedLines_[line] = std::chrono::steady_clock::now();
    
    // If we have too many entries, remove old ones
    if (recentlyProcessedLines_.size() > 1000) {
        auto now = std::chrono::steady_clock::now();
        for (auto it = recentlyProcessedLines_.begin(); it != recentlyProcessedLines_.end();) {
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
            if (static_cast<size_t>(elapsedSeconds) > RECENT_PROCESSED_TTL_SECONDS) {
                it = recentlyProcessedLines_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void SyntaxHighlightingManager::logCacheMetrics(const char* context, size_t visibleLines, size_t totalProcessed) const {
    logManagerMessage(EditorException::Severity::Debug, context,
                     "Cache metrics: visible=%zu, processed=%zu, timestamps=%zu, access=%zu",
                     visibleLines, totalProcessed, lineTimestamps_.size(), lineAccessTimes_.size());
}

// Process a visible range asynchronously
void SyntaxHighlightingManager::processVisibleRangeAsync(size_t startLine, size_t endLine) const {
    // Skip processing if highlighting is disabled
    if (!isEnabled()) {
        return;
    }
    
    try {
        // Check if we have a thread pool
        if (!threadPool_) {
            if (debugLoggingEnabled_) {
                logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::processVisibleRangeAsync",
                                 "No thread pool available for async processing");
            }
            return;
        }
        
        // Take a shared lock to check if we already have an active task for this range
        std::shared_lock lock(mutex_);
        
        // Create a unique key for this range
        std::string rangeKey = std::to_string(startLine) + "-" + std::to_string(endLine);
        
        // Check if we already have a task for this range
        auto taskIt = activeRangeTasks_.find(rangeKey);
        if (taskIt != activeRangeTasks_.end()) {
            // Task already exists
            auto& future = taskIt->second;
            
            // Check if it's still running
            if (future.valid() && future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
                if (debugLoggingEnabled_) {
                    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::processVisibleRangeAsync",
                                     "Task already running for range %zu-%zu", startLine, endLine);
                }
                return;  // Let the existing task complete
            }
        }
        
        // Convert to unique lock for modifying the task map
        lock.unlock();
        std::unique_lock uniqueLock(mutex_);
        
        // Determine task type - visible range gets highest priority
        TaskType taskType = TaskType::VISIBLE_RANGE;
        
        // Submit task to thread pool using the submit method
        auto future = threadPool_->submit(
            getTaskPriority(taskType),
            &SyntaxHighlightingManager::taskHighlightLines,
            const_cast<SyntaxHighlightingManager*>(this), // Required because taskHighlightLines is const but threadPool->submit expects non-const
            startLine, 
            endLine, 
            taskType
        );
        
        // Store the future in the active tasks map
        activeRangeTasks_[rangeKey] = std::move(future);
        
        if (debugLoggingEnabled_) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::processVisibleRangeAsync",
                             "Submitted task for range %zu-%zu with priority %d", 
                             startLine, endLine, static_cast<int>(getTaskPriority(taskType)));
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::processVisibleRangeAsync",
                         "Exception submitting task: %s", ex.what());
    }
}

// Task function for highlighting a range of lines
void SyntaxHighlightingManager::taskHighlightLines(size_t startLine, size_t endLine, TaskType taskType) const {
    if (!isEnabled()) {
        return;
    }
    
    try {
        auto startTime = std::chrono::steady_clock::now();
        
        // Get timeout based on task type
        size_t timeoutMs = highlightingTimeoutMs_.load(std::memory_order_acquire);
        if (taskType == TaskType::BACKGROUND_RANGE) {
            // Use longer timeout for background tasks
            timeoutMs *= 2;
        } else if (taskType == TaskType::CONTEXT_RANGE) {
            // Use medium timeout for context tasks
            timeoutMs = static_cast<size_t>(timeoutMs * 1.5);
        }
        
        // Acquire lock and process lines
        std::unique_lock lock(mutex_);
        
        // Highlight the lines with appropriate timeout
        auto timeout = std::chrono::milliseconds(timeoutMs);
        
        // Use a non-const version of this to call highlightLines_nolock
        SyntaxHighlightingManager* nonConstThis = const_cast<SyntaxHighlightingManager*>(this);
        bool allProcessed = nonConstThis->highlightLines_nolock(startLine, endLine, timeout);
        
        if (debugLoggingEnabled_) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);
            
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::taskHighlightLines",
                             "Background processing for lines %zu-%zu completed in %lld ms (%s)",
                             startLine, endLine, duration.count(),
                             allProcessed ? "all processed" : "timeout reached");
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::taskHighlightLines",
                         "Exception in background task: %s", ex.what());
    } catch (...) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::taskHighlightLines",
                         "Unknown exception in background task");
    }
}

// Thread pool management methods
void SyntaxHighlightingManager::setThreadPoolSize(size_t numThreads) {
    try {
        // Create a new thread pool with the specified size
        auto newThreadPool = std::make_unique<ThreadPool>(numThreads);
        
        // Replace the old thread pool
        std::unique_lock lock(mutex_);
        threadPool_ = std::move(newThreadPool);
        
        // Clear active tasks
        activeLineTasks_.clear();
        activeRangeTasks_.clear();
        
        logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::setThreadPoolSize",
                         "Thread pool size set to %zu", numThreads);
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::setThreadPoolSize",
                         "Exception setting thread pool size: %s", ex.what());
    }
}

size_t SyntaxHighlightingManager::getThreadPoolSize() const {
    if (!threadPool_) {
        return 0;
    }
    return threadPool_->getThreadCount();
}

size_t SyntaxHighlightingManager::getActiveThreadCount() const {
    if (!threadPool_) {
        return 0;
    }
    return threadPool_->getActiveThreadCount();
}

size_t SyntaxHighlightingManager::getQueuedTaskCount() const {
    if (!threadPool_) {
        return 0;
    }
    return threadPool_->getQueueSize();
}

// Get the current cache size
size_t SyntaxHighlightingManager::getCacheSize() const {
    std::shared_lock lock(mutex_);
    size_t validCount = 0;
    for (const auto& entry : cachedStyles_) {
        if (entry && entry->valid.load(std::memory_order_acquire)) {
            validCount++;
        }
    }
    return validCount;
}

// Get task priority based on task type
ThreadPool::Priority SyntaxHighlightingManager::getTaskPriority(TaskType taskType) const {
    switch (taskType) {
        case TaskType::VISIBLE_RANGE:
            return ThreadPool::Priority::HIGH;
        case TaskType::SINGLE_LINE:
            return ThreadPool::Priority::HIGH;
        case TaskType::CONTEXT_RANGE:
            return ThreadPool::Priority::NORMAL;
        case TaskType::BACKGROUND_RANGE:
            return ThreadPool::Priority::LOW;
        default:
            return ThreadPool::Priority::NORMAL;
    }
}

// Determine if a task should be queued based on current load
bool SyntaxHighlightingManager::shouldQueueTask(TaskType taskType) const {
    if (!threadPool_) {
        return false;
    }
    
    // Get the current queue size
    size_t queueSize = threadPool_->getQueueSize();
    
    // Apply throttling based on task type and current load
    switch (taskType) {
        case TaskType::VISIBLE_RANGE:
            // Always process visible range tasks, but limit to a reasonable number
            return queueSize < MAX_WORK_QUEUE_SIZE / 2;
        
        case TaskType::SINGLE_LINE:
            // Process single line tasks if queue isn't too large
            return queueSize < MAX_WORK_QUEUE_SIZE / 2;
        
        case TaskType::CONTEXT_RANGE:
            // Only process context range tasks if queue is relatively small
            return queueSize < MAX_WORK_QUEUE_SIZE / 4;
        
        case TaskType::BACKGROUND_RANGE:
            // Only process background tasks if queue is nearly empty
            return queueSize < MAX_WORK_QUEUE_SIZE / 8;
        
        default:
            return queueSize < MAX_WORK_QUEUE_SIZE / 4;
    }
}

// Process a single line asynchronously
void SyntaxHighlightingManager::processSingleLineAsync(size_t line) {
    // Skip processing if highlighting is disabled
    if (!isEnabled()) {
        return;
    }
    
    try {
        // Check if we have a thread pool
        if (!threadPool_) {
            if (debugLoggingEnabled_) {
                logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::processSingleLineAsync",
                                 "No thread pool available for async processing");
            }
            return;
        }
        
        // Check if we should queue the task based on current load
        if (!shouldQueueTask(TaskType::SINGLE_LINE)) {
            if (debugLoggingEnabled_) {
                logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::processSingleLineAsync",
                                 "Task throttled due to high load");
            }
            return;
        }
        
        // Take a shared lock to check if we already have an active task for this line
        std::shared_lock lock(mutex_);
        
        // Check if we already have a task for this line
        auto taskIt = activeLineTasks_.find(line);
        if (taskIt != activeLineTasks_.end()) {
            // Task already exists
            auto& future = taskIt->second;
            
            // Check if it's still running
            if (future.valid() && future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
                if (debugLoggingEnabled_) {
                    logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::processSingleLineAsync",
                                     "Task already running for line %zu", line);
                }
                return;  // Let the existing task complete
            }
        }
        
        // Convert to unique lock for modifying the task map
        lock.unlock();
        std::unique_lock uniqueLock(mutex_);
        
        // Submit task to thread pool
        auto future = threadPool_->submit(
            getTaskPriority(TaskType::SINGLE_LINE),
            &SyntaxHighlightingManager::taskHighlightLine,
            this,
            line
        );
        
        // Store the future in the active tasks map
        activeLineTasks_[line] = std::move(future);
        
        if (debugLoggingEnabled_) {
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::processSingleLineAsync",
                             "Submitted task for line %zu with priority %d",
                             line, static_cast<int>(getTaskPriority(TaskType::SINGLE_LINE)));
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::processSingleLineAsync",
                         "Exception submitting task: %s", ex.what());
    }
}

// Task function for highlighting a single line
void SyntaxHighlightingManager::taskHighlightLine(size_t line) {
    if (!isEnabled()) {
        return;
    }
    
    try {
        auto startTime = std::chrono::steady_clock::now();
        
        // Acquire lock and process the line
        std::unique_lock lock(mutex_);
        
        // Check if the line is still invalid or needs processing
        if (line < cachedStyles_.size() && 
            cachedStyles_[line] && 
            cachedStyles_[line]->valid.load(std::memory_order_acquire) &&
            invalidatedLines_.count(line) == 0) {
            // Line is already valid, nothing to do
            return;
        }
        
        // Highlight the line
        auto styles = highlightLine_nolock(line);
        
        if (styles) {
            // Ensure cache has enough space
            if (cachedStyles_.size() <= line) {
                cachedStyles_.resize(line + 1);
            }
            
            // Create cache entry with the new styles
            cachedStyles_[line] = std::make_unique<CacheEntry>(std::move(*styles));
            
            // Update timestamps
            auto now = std::chrono::steady_clock::now();
            lineTimestamps_[line] = now;
            lineAccessTimes_[line] = now;
            
            // Remove from invalidated lines set
            invalidatedLines_.erase(line);
        }
        
        if (debugLoggingEnabled_) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startTime);
            
            logManagerMessage(EditorException::Severity::Debug, "SyntaxHighlightingManager::taskHighlightLine",
                             "Background processing for line %zu completed in %lld μs",
                             line, duration.count());
        }
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::taskHighlightLine",
                         "Exception in background task: %s", ex.what());
    } catch (...) {
        logManagerMessage(EditorException::Severity::EDITOR_ERROR, "SyntaxHighlightingManager::taskHighlightLine",
                         "Unknown exception in background task");
    }
}

