#include "SyntaxHighlightingManager.h"
#include <iostream>
#include <thread>
#include "EditorError.h"
#include <queue>

namespace {
    // New logger function using ErrorReporter
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    template<typename... Args>
    void logManagerMessage(EditorException::Severity severity, const char* location, const char* format, Args... args) {
        try {
            std::stringstream ss_prefix;
            ss_prefix << "[Thread " << std::this_thread::get_id() << "] ";
            ss_prefix << location << ": ";
            
            // Determine needed size for buffer
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

            if (severity == EditorException::Severity::Warning) {
                ErrorReporter::logWarning(full_message);
            } else { // Default to error for other severities (e.g., Error, Critical)
                ErrorReporter::logError(full_message);
            }
        } catch (const std::exception& log_ex) {
            // Fallback to cerr if ErrorReporter itself fails or if string stream fails
            std::cerr << "CRITICAL LOGGING FAILURE in logManagerMessage: " << log_ex.what() << std::endl;
        } catch (...) {
            std::cerr << "CRITICAL LOGGING FAILURE in logManagerMessage: Unknown exception." << std::endl;
        }
    }
#pragma clang diagnostic pop
}

SyntaxHighlightingManager::SyntaxHighlightingManager()
    : buffer_(nullptr),
      highlighter_(nullptr),
      enabled_(true),
      visibleStartLine_(0), 
      visibleEndLine_(0),
      highlightingTimeoutMs_(DEFAULT_HIGHLIGHTING_TIMEOUT_MS),
      contextLines_(DEFAULT_CONTEXT_LINES) {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::Constructor", "Instance created.");
}

SyntaxHighlightingManager::~SyntaxHighlightingManager() {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "[ENTRY] Destructor called.");

    if (highlighter_) {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ initial use_count: %ld", highlighter_.use_count());
    } else {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ is initially null.");
    }

    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Attempting to acquire mutex in destructor...");
    try {
        std::scoped_lock lock(mutex_); 
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Mutex acquired successfully in destructor.");

        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "cachedStyles_ size: %zu, capacity: %zu", cachedStyles_.size(), cachedStyles_.capacity());
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "invalidatedLines_ size: %zu", invalidatedLines_.size());
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "lineTimestamps_ size: %zu", lineTimestamps_.size());
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "lineAccessTimes_ size: %zu", lineAccessTimes_.size());

        if (highlighter_) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Resetting highlighter_ manually. Current use_count: %ld", highlighter_.use_count());
            highlighter_.reset();
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ has been reset.");
        } else {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ was already null before explicit reset attempt.");
        }
        
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Clearing cachedStyles_ manually.");
        cachedStyles_.clear(); 
        cachedStyles_.shrink_to_fit(); // Explicitly return memory
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "cachedStyles_ cleared. New size: %zu, capacity: %zu", cachedStyles_.size(), cachedStyles_.capacity());

        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Clearing all containers manually.");
        invalidatedLines_.clear();
        lineTimestamps_.clear();
        lineAccessTimes_.clear();
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "All containers cleared.");

        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Releasing mutex in destructor (via scoped_lock).");
    } catch (const std::system_error& e) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "std::system_error acquiring mutex in destructor: %s (code: %d)", e.what(), e.code().value());
    } catch (const std::exception& e) {
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "std::exception during cleanup in destructor: %s", e.what());
    } catch (...) {
        logManagerMessage(EditorException::Severity::Critical, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Unknown exception during cleanup in destructor.");
    }
    
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "[EXIT] Destructor finished.");
}

// Helper function to format duration for logging
std::string formatDuration(const std::chrono::steady_clock::time_point& start) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    return std::to_string(duration.count()) + " ms";
}

// Helper function to log vector access for debugging
void SyntaxHighlightingManager::logVectorAccess(const char* location, size_t index, size_t vectorSize) const {
    logManagerMessage(EditorException::Severity::Warning,
                     "DEBUG_VECTOR_ACCESS",
                     "[%s] Accessing cachedStyles_[%zu], vector size: %zu",
                     location, index, vectorSize);
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
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setHighlighter", "Attempting to set highlighter. Pointer: %p", static_cast<void*>(highlighter.get()));
        std::scoped_lock lock(mutex_);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setHighlighter", "Mutex acquired. Immediately invalidating all lines.");
        invalidateAllLines_nolock(); // Aggressive invalidation
        highlighter_ = highlighter; // Simple assignment
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setHighlighter", "Highlighter set and lines invalidated.");
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setHighlighter: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setHighlighter");
    }
}

std::shared_ptr<SyntaxHighlighter> SyntaxHighlightingManager::getHighlighter() const {
    try {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlighter", "Attempting to get highlighter.");
        std::scoped_lock<std::recursive_mutex> lock(mutex_); // Changed to scoped_lock
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlighter", "Scoped_lock (unique) on recursive_mutex acquired.");
        return highlighter_;
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        return nullptr;
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("getHighlighter: ") + ex.what(), EditorException::Severity::Error));
        return nullptr;
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::getHighlighter");
        return nullptr;
    }
}

void SyntaxHighlightingManager::setEnabled(bool enabled) {
    try {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setEnabled", "Attempting to set enabled to: %s", enabled ? "true" : "false");
        std::scoped_lock lock(mutex_);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setEnabled", "Mutex acquired.");
        bool oldValue = enabled_.load(std::memory_order_acquire);
        if (oldValue != enabled) {
            enabled_.store(enabled, std::memory_order_release);
            invalidateAllLines_nolock();
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setEnabled", "Enabled status changed to %s and lines invalidated.", enabled ? "true" : "false");
        } else {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setEnabled", "Enabled status already %s. No change.", enabled ? "true" : "false");
        }
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setEnabled: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setEnabled");
    }
}

bool SyntaxHighlightingManager::isEnabled() const {
    try {
        // Atomic read doesn't need a lock
        return enabled_.load(std::memory_order_acquire);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        return false; // Default value on error
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("isEnabled: ") + ex.what(), EditorException::Severity::Error));
        return false; // Default value on error
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::isEnabled");
        return false; // Default value on error
    }
}

void SyntaxHighlightingManager::setBuffer(const TextBuffer* buffer) {
    try {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setBuffer", "Attempting to set buffer. Pointer: %p", static_cast<const void*>(buffer));
        std::scoped_lock lock(mutex_);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setBuffer", "Mutex acquired. Immediately invalidating all lines.");
        invalidateAllLines_nolock(); // Aggressive invalidation
        buffer_.store(buffer, std::memory_order_release);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::setBuffer", "Buffer set and lines invalidated.");
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setBuffer: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setBuffer");
    }
}

// Definition of highlightLine_nolock (private)
std::unique_ptr<std::vector<SyntaxStyle>> SyntaxHighlightingManager::highlightLine_nolock(size_t line) {
    const TextBuffer* buffer = getBuffer();
    SyntaxHighlighter* highlighter = getHighlighterPtr_nolock();
    
    if (!buffer || !highlighter || !enabled_.load(std::memory_order_acquire)) {
        logManagerMessage(EditorException::Severity::Warning, 
                         "SyntaxHighlightingManager::highlightLine_nolock",
                         "Skipping highlighting for line %zu due to null buffer/highlighter or disabled state",
                         line);
        return std::make_unique<std::vector<SyntaxStyle>>();
    }
    
    try {
        // Check if we need to perform cache cleanup
        auto currentTime = std::chrono::steady_clock::now();
        auto timeSinceLastCleanup = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastCleanupTime_);
        if (timeSinceLastCleanup.count() > CACHE_ENTRY_LIFETIME_MS) {
            cleanupCache_nolock();
        }
        
        // Get the line content
        const std::string& lineContent = buffer->getLine(line);
        
        // Request highlighting from the syntax highlighter
        auto styles = highlighter->highlightLine(lineContent, line);
        
        // If highlighter returned nullptr, create empty styles
        if (!styles) {
            styles = std::make_unique<std::vector<SyntaxStyle>>();
        }
        
        // Update the cache if we have space or can make space
        bool shouldCache = true;
        if (line >= cachedStyles_.size()) {
            if (cachedStyles_.size() >= MAX_CACHE_LINES) {
                // Try to clean up the cache first
                cleanupCache_nolock();
                
                // If we still don't have space, we can't cache this line
                if (cachedStyles_.size() >= MAX_CACHE_LINES) {
                    shouldCache = false;
                }
            }
            
            if (shouldCache) {
                // Only resize if we're not already at the limit
                size_t newSize = std::min(line + 1, MAX_CACHE_LINES);
                if (newSize > cachedStyles_.size()) {
                    try {
                        logManagerMessage(EditorException::Severity::Warning,
                                         "SyntaxHighlightingManager::highlightLine_nolock",
                                         "Resizing cachedStyles_ from %zu to %zu for line %zu",
                                         cachedStyles_.size(), newSize, line);
                        
                        // Reserve capacity first to reduce potential allocation failures
                        if (newSize > cachedStyles_.capacity()) {
                            cachedStyles_.reserve(newSize);
                        }
                        
                        cachedStyles_.resize(newSize);
                        
                        logManagerMessage(EditorException::Severity::Warning,
                                         "SyntaxHighlightingManager::highlightLine_nolock",
                                         "Resize completed. New size: %zu, capacity: %zu",
                                         cachedStyles_.size(), cachedStyles_.capacity());
                    } catch (const std::exception& ex) {
                        logManagerMessage(EditorException::Severity::Error,
                                         "SyntaxHighlightingManager::highlightLine_nolock",
                                         "Exception during resize for line %zu: %s",
                                         line, ex.what());
                        shouldCache = false;
                    } catch (...) {
                        logManagerMessage(EditorException::Severity::Error,
                                         "SyntaxHighlightingManager::highlightLine_nolock",
                                         "Unknown exception during resize for line %zu",
                                         line);
                        shouldCache = false;
                    }
                }
            }
        }
        
        // Only cache if we have space and should cache
        if (shouldCache && line < cachedStyles_.size()) {
            // Create new cache entry with the styles
            auto currentTime = std::chrono::steady_clock::now();
            
            try {
                cachedStyles_[line] = std::make_shared<CacheEntry>(std::move(*styles));
                
                // Verify the cache entry was created before accessing it
                logVectorAccess("highlightLine_nolock:270", line, cachedStyles_.size());
                
                if (line < cachedStyles_.size() && cachedStyles_[line]) {
                    logVectorAccess("highlightLine_nolock:272", line, cachedStyles_.size());
                    cachedStyles_[line]->timestamp = currentTime;
                    cachedStyles_[line]->valid.store(true, std::memory_order_release);
                    
                    // Update access time for cache management
                    lineAccessTimes_[line] = currentTime;
                    lineTimestamps_[line] = currentTime;
                    
                    // Remove from invalidated lines if present
                    invalidatedLines_.erase(line);
                    
                    // Return a copy of the cached styles
                    return std::make_unique<std::vector<SyntaxStyle>>(cachedStyles_[line]->styles);
                } else {
                    logManagerMessage(EditorException::Severity::Error,
                                     "SyntaxHighlightingManager::highlightLine_nolock",
                                     "Cache entry for line %zu is null or out of bounds after creation",
                                     line);
                }
            } catch (const std::exception& ex) {
                logManagerMessage(EditorException::Severity::Error,
                                 "SyntaxHighlightingManager::highlightLine_nolock",
                                 "Exception creating/accessing cache entry for line %zu: %s",
                                 line, ex.what());
            } catch (...) {
                logManagerMessage(EditorException::Severity::Error,
                                 "SyntaxHighlightingManager::highlightLine_nolock",
                                 "Unknown exception creating/accessing cache entry for line %zu",
                                 line);
            }
        }
        
        // If we couldn't cache, just return the original styles
        return std::move(styles);
        
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error,
                         "SyntaxHighlightingManager::highlightLine_nolock",
                         "Exception while highlighting line %zu: %s",
                         line, ex.what());
        return std::make_unique<std::vector<SyntaxStyle>>();
    }
}

// Public highlightLine method - acquires lock, ensures cache size, calls _nolock version
void SyntaxHighlightingManager::highlightLine(size_t line) {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine (public)", "Called for line %zu.", line);
    try {
        std::unique_lock<std::recursive_mutex> lock(mutex_); // Acquire UNIQUE lock
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine (public)", "Acquired unique_lock for line %zu.", line);

        const TextBuffer* buffer = getBuffer(); // Atomic read
        SyntaxHighlighter* highlighter = getHighlighterPtr_nolock(); // Uses internal getter, lock held
        bool enabled = enabled_.load(std::memory_order_acquire); // Atomic read

        if (!buffer || !highlighter || !enabled) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine (public)", "Skipping for line %zu: Preliminaries not met (buffer/highlighter/disabled).", line);
            return;
        }
        if (line >= buffer->lineCount()) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine (public)", "Line %zu out of bounds (%zu).", line, buffer->lineCount());
            return;
        }

        // Ensure cache is large enough for this specific line under the unique lock
        if (line >= cachedStyles_.size()) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine (public)", "Resizing cache from %zu to %zu for line %zu.", cachedStyles_.size(), line + 1, line);
            cachedStyles_.resize(line + 1);
        }

        highlightLine_nolock(line); // Call the no-lock version to do the work

        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine (public)", "Finished call to highlightLine_nolock for line %zu.", line);

    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        // No return here, let it fall through if needed or handle specific to public API
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("highlightLine (public): ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::highlightLine (public)");
    }
}

// Private method, assumes caller (getHighlightingStyles) holds unique_lock
bool SyntaxHighlightingManager::highlightLines_nolock(size_t startLine, size_t endLine, 
                                                      const std::chrono::milliseconds& timeout) {
    auto highlightStart = std::chrono::steady_clock::now();
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::highlightLines_nolock",
                     "[ENTRY] Called for lines %zu to %zu with timeout %lld ms",
                     startLine, endLine, timeout.count());
    
    const TextBuffer* buffer = getBuffer();
    if (!buffer || buffer->isEmpty()) {
        return false;
    }
    
    // Make sure we don't exceed buffer bounds
    size_t maxLine = buffer->lineCount() - 1;
    if (endLine > maxLine) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::highlightLines_nolock",
                         "Adjusting endLine from %zu to %zu (buffer size)",
                         endLine, maxLine);
        endLine = maxLine;
    }
    
    // If startLine is now greater than endLine after adjustment, return
    if (startLine > endLine) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::highlightLines_nolock",
                         "Invalid range after adjustment: startLine %zu > endLine %zu",
                         startLine, endLine);
        return false;
    }
    
    // Collect lines that need processing
    std::vector<size_t> linesToProcess;
    linesToProcess.reserve(endLine - startLine + 1);
    
    // First, add the requested line range
    for (size_t line = startLine; line <= endLine; ++line) {
        if (line >= cachedStyles_.size() || 
            (logVectorAccess("highlightLines_nolock:355", line, cachedStyles_.size()), !cachedStyles_[line]) || 
            (logVectorAccess("highlightLines_nolock:356", line, cachedStyles_.size()), 
             !cachedStyles_[line]->valid.load(std::memory_order_acquire))) {
            linesToProcess.push_back(line);
        }
    }
    
    if (linesToProcess.empty()) {
        return true;
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::highlightLines_nolock",
                     "Found %zu lines to process (prioritized).",
                     linesToProcess.size());
    
    // Process lines in small batches to better handle timeouts
    static constexpr size_t BATCH_SIZE = 25;
    size_t processedCount = 0;
    bool timedOut = false;
    
    for (size_t i = 0; i < linesToProcess.size() && !timedOut; i += BATCH_SIZE) {
        // Check if we've exceeded the timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - highlightStart);
        if (elapsed >= timeout) {
            timedOut = true;
            break;
        }
        
        // Process a batch of lines with additional safety bounds check
        size_t batchEnd = std::min(i + BATCH_SIZE, linesToProcess.size());
        for (size_t j = i; j < batchEnd; ++j) {
            // Double-check j is within bounds (defensive)
            if (j >= linesToProcess.size()) {
                logManagerMessage(EditorException::Severity::Error,
                             "SyntaxHighlightingManager::highlightLines_nolock",
                             "Array index %zu out of bounds (size: %zu)",
                             j, linesToProcess.size());
                break;
            }
            
            size_t line = linesToProcess[j];
            logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::highlightLines_nolock",
                         "Processing line %zu", line);
            
            // Verify line is still valid (buffer might have changed size)
            if (line > maxLine) {
                logManagerMessage(EditorException::Severity::Warning,
                                 "SyntaxHighlightingManager::highlightLines_nolock",
                                 "Skipping line %zu which is now beyond buffer size %zu",
                                 line, maxLine + 1);
                continue;
            }
            
            // Get the line content and highlight it - ALWAYS DO THIS regardless of cache status
            std::string lineContent = buffer->getLine(line);
            auto styles = highlighter_->highlightLine(lineContent, line);
            
            // Define whether we can cache this line
            bool canCacheThisLine = (line < MAX_CACHE_LINES);
            
            if (canCacheThisLine) {
                // Handle caching logic for lines within MAX_CACHE_LINES limit
                try {
                    // Resize cachedStyles_ if needed
                    if (line >= cachedStyles_.size()) {
                        // If we're close to MAX_CACHE_LINES, evict some entries proactively
                        if (line > MAX_CACHE_LINES * 9 / 10) {
                            logManagerMessage(EditorException::Severity::Warning,
                                            "SyntaxHighlightingManager::highlightLines_nolock",
                                            "Proactively evicting entries as we approach MAX_CACHE_LINES");
                            
                            // Evict entries to make room, target 80% of max to avoid frequent evictions
                            size_t targetSize = MAX_CACHE_LINES * 8 / 10;
                            evictLRUEntries_nolock(targetSize);
                        }
                        
                        size_t newSize = line + 1; // Safe because we verified line < MAX_CACHE_LINES
                        
                        logManagerMessage(EditorException::Severity::Warning,
                                        "SyntaxHighlightingManager::highlightLines_nolock",
                                        "Resizing cachedStyles_ from %zu to %zu for line %zu (max: %zu)",
                                        cachedStyles_.size(), newSize, line, MAX_CACHE_LINES);
                        
                        // Reserve capacity first to reduce potential allocation failures
                        if (newSize > cachedStyles_.capacity()) {
                            cachedStyles_.reserve(newSize);
                        }
                        
                        // Now do the actual resize
                        cachedStyles_.resize(newSize);
                        
                        logManagerMessage(EditorException::Severity::Warning,
                                        "SyntaxHighlightingManager::highlightLines_nolock",
                                        "Resize completed. New size: %zu, capacity: %zu",
                                        cachedStyles_.size(), cachedStyles_.capacity());
                    }
                    
                    // Now line should be a valid index within cachedStyles_
                    if (!cachedStyles_[line]) {
                        cachedStyles_[line] = std::make_unique<CacheEntry>();
                    }
                    
                    // Store the styles in the cache entry
                    if (styles) {
                        cachedStyles_[line]->styles = std::move(*styles);
                    } else {
                        cachedStyles_[line]->styles.clear();
                    }
                    cachedStyles_[line]->valid.store(true, std::memory_order_release);
                    
                    // Update timestamps
                    auto now = std::chrono::steady_clock::now();
                    lineTimestamps_[line] = now;
                    lineAccessTimes_[line] = now;
                    
                    // Remove from invalidated set if present
                    invalidatedLines_.erase(line);
                    
                    logManagerMessage(EditorException::Severity::Warning,
                                    "SyntaxHighlightingManager::highlightLines_nolock",
                                    "Cached result for line %zu.", line);
                } catch (const std::exception& ex) {
                    logManagerMessage(EditorException::Severity::Error, 
                                    "SyntaxHighlightingManager::highlightLines_nolock",
                                    "Exception during caching for line %zu: %s", 
                                    line, ex.what());
                    // Continue processing - don't skip, as we already highlighted the line
                } catch (...) {
                    logManagerMessage(EditorException::Severity::Error,
                                    "SyntaxHighlightingManager::highlightLines_nolock",
                                    "Unknown exception during caching for line %zu", line);
                    // Continue processing - don't skip, as we already highlighted the line
                }
            } else {
                // Line is beyond MAX_CACHE_LINES limit - highlighted but not cached
                logManagerMessage(EditorException::Severity::Warning,
                                "SyntaxHighlightingManager::highlightLines_nolock",
                                "Line %zu was highlighted but not cached (exceeds MAX_CACHE_LINES %zu)",
                                line, MAX_CACHE_LINES);
            }
            
            logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::highlightLines_nolock",
                         "Finished processing line %zu.", line);
            
            processedCount++;
            
            // Check timeout after each line
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - highlightStart);
            if (elapsed >= timeout) {
                timedOut = true;
                break;
            }
        }
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - highlightStart);
    
    if (timedOut) {
        if (processedCount > 0) {
            // Safe access to linesToProcess
            logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::highlightLines_nolock",
                         "Timeout AFTER processing line %zu. Elapsed: %lld ms.",
                         linesToProcess[processedCount - 1], elapsed.count());
        } else {
            // No lines were processed before timeout
            logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::highlightLines_nolock",
                         "Timeout BEFORE processing any lines. Elapsed: %lld ms.",
                         elapsed.count());
        }
    } else {
        logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::highlightLines_nolock",
                     "All %zu lines processed within timeout.",
                     processedCount);
    }
    
    // Return true only if we processed all requested lines
    return processedCount == linesToProcess.size();
}

// Definition of the CONST version of getHighlightingStyles
std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(
    size_t startLine, size_t endLine) const {
    
    std::scoped_lock<std::recursive_mutex> lock(mutex_); 
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Getting styles for lines %zu to %zu.", startLine, endLine);

    const TextBuffer* currentBuffer = getBuffer(); 
    if (!currentBuffer || !isEnabled() || !highlighter_ || (currentBuffer->lineCount() > 0 && (startLine > endLine || endLine >= currentBuffer->lineCount())) || (currentBuffer->lineCount() == 0 && (startLine != 0 || endLine != 0)) ) {
        if (!currentBuffer) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "No buffer.");
        else if (!isEnabled()) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Not enabled.");
        else if (!highlighter_) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "No highlighter.");
        else if (currentBuffer->lineCount() > 0 && (startLine > endLine || endLine >= currentBuffer->lineCount())) {
             logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Invalid line range %zu-%zu for buffer size %zu.", startLine, endLine, currentBuffer->lineCount());
        } else if (currentBuffer->lineCount() == 0 && (startLine !=0 || endLine != 0)) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Invalid line range %zu-%zu for empty buffer.", startLine, endLine);
        }
        
        std::vector<std::vector<SyntaxStyle>> emptyStyles;
        // If range is numerically valid (e.g. 0-0 for empty buffer, or valid sub-range for non-empty)
        // provide a correctly sized vector of empty style lists.
        if (startLine <= endLine) { // Check if the range itself is non-negative
            size_t count = endLine - startLine + 1;
            // If buffer is empty, only line 0 count 1 (0-0) is sensible for "empty line style"
            if (currentBuffer && currentBuffer->lineCount() == 0 && (startLine == 0 && endLine == 0)) {
                emptyStyles.resize(1);
            } else if (currentBuffer && currentBuffer->lineCount() > 0 && endLine < currentBuffer->lineCount()) {
                emptyStyles.resize(count);
            } else if (!currentBuffer) { // No buffer, but range given, provide empty styles for count
                 emptyStyles.resize(count);
            } // else, range is invalid for buffer, return truly empty (size 0) vector.
        }
        return emptyStyles;
    }

    std::vector<std::vector<SyntaxStyle>> resultStyles;
    resultStyles.reserve(endLine - startLine + 1);

    for (size_t i = startLine; i <= endLine; ++i) {
        if (i < cachedStyles_.size()) {
            logVectorAccess("getHighlightingStyles(const):498", i, cachedStyles_.size());
            if (cachedStyles_[i]) {
                // Check validity atomically and safely
                bool isValid = false;
                try {
                    isValid = cachedStyles_[i]->valid.load(std::memory_order_acquire);
                }
                catch (const std::exception& ex) {
                    logManagerMessage(EditorException::Severity::Error,
                                     "SyntaxHighlightingManager::getHighlightingStyles (CONST)",
                                     "Exception checking validity for line %zu: %s",
                                     i, ex.what());
                    resultStyles.emplace_back();
                    continue;
                }
                
                if (isValid) {
                    if constexpr (CACHE_ENTRY_LIFETIME_MS > 0) {
                        try {
                            // Make a safe copy of the timestamp
                            auto timestamp = cachedStyles_[i]->timestamp;
                            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - timestamp);
                            if (age.count() < static_cast<long long>(CACHE_ENTRY_LIFETIME_MS)) {
                                // Make a safe copy of the styles
                                resultStyles.push_back(cachedStyles_[i]->styles);
                            } else {
                                resultStyles.emplace_back(); 
                                logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Cache entry for line %zu expired.", i);
                            }
                        }
                        catch (const std::exception& ex) {
                            logManagerMessage(EditorException::Severity::Error,
                                             "SyntaxHighlightingManager::getHighlightingStyles (CONST)",
                                             "Exception accessing timestamp for line %zu: %s",
                                             i, ex.what());
                            resultStyles.emplace_back();
                        }
                    } else { 
                        try {
                            // Make a safe copy of the styles
                            resultStyles.push_back(cachedStyles_[i]->styles);
                        }
                        catch (const std::exception& ex) {
                            logManagerMessage(EditorException::Severity::Error,
                                             "SyntaxHighlightingManager::getHighlightingStyles (CONST)",
                                             "Exception copying styles for line %zu: %s",
                                             i, ex.what());
                            resultStyles.emplace_back();
                        }
                    }
                } else {
                    resultStyles.emplace_back(); 
                    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Cache entry for line %zu not valid.", i);
                }
            } else {
                resultStyles.emplace_back(); 
                if (i >= cachedStyles_.size()) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Line %zu out of cachedStyles_ bounds (%zu).", i, cachedStyles_.size());
                else if (!cachedStyles_[i]) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "No cache entry for line %zu.", i);
            }
        } else {
            resultStyles.emplace_back(); 
            if (i >= cachedStyles_.size()) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Line %zu out of cachedStyles_ bounds (%zu).", i, cachedStyles_.size());
            else if (!cachedStyles_[i]) logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "No cache entry for line %zu.", i);
        }
    }
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (CONST)", "Returning %zu style sets.", resultStyles.size());
    return resultStyles;
}

// Non-const version - this was existing
std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(size_t startLine, size_t endLine) {
    logLockAcquisition("SyntaxHighlightingManager::getHighlightingStyles");
    auto lockStart = std::chrono::steady_clock::now();
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    
        logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::getHighlightingStyles",
                     "[ENTRY] Called for lines %zu to %zu.",
                     startLine, endLine);
    
    logCacheMetrics("SyntaxHighlightingManager::getHighlightingStyles");
    
    const TextBuffer* buffer = getBuffer();
    if (!buffer) {
        logLockRelease("SyntaxHighlightingManager::getHighlightingStyles", lockStart);
        return std::vector<std::vector<SyntaxStyle>>();
    }
    
    // Calculate optimal processing range based on access pattern
    auto [processStart, processEnd] = calculateOptimalProcessingRange(startLine, endLine);
    
        logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::getHighlightingStyles",
                     "Optimal processing range: [%zu, %zu] for request [%zu, %zu]",
                     processStart, processEnd, startLine, endLine);
    
    // Process the calculated range
    bool rangeProcessed = highlightLines_nolock(processStart, processEnd,
        std::chrono::milliseconds(DEFAULT_HIGHLIGHTING_TIMEOUT_MS));
    
    if (rangeProcessed) {
        // Update last processed range with the ORIGINAL requested range, not the expanded processing range
        // This is critical for proper sequential access detection
        lastProcessedRange_.update(startLine, endLine);
    } else {
        // On timeout, invalidate tracking to avoid making assumptions
        lastProcessedRange_.invalidate();
    }
    
    // Return styles for the originally requested range only
    std::vector<std::vector<SyntaxStyle>> result;
    result.reserve(endLine - startLine + 1);
    
    for (size_t line = startLine; line <= endLine; ++line) {
        if (line < cachedStyles_.size()) {
            logVectorAccess("getHighlightingStyles:586", line, cachedStyles_.size());
            if (cachedStyles_[line]) {
                // Check validity atomically and safely
                bool isValid = false;
                try {
                    isValid = cachedStyles_[line]->valid.load(std::memory_order_acquire);
                }
                catch (const std::exception& ex) {
                    logManagerMessage(EditorException::Severity::Error,
                                     "SyntaxHighlightingManager::getHighlightingStyles",
                                     "Exception checking validity for line %zu: %s",
                                     line, ex.what());
                    result.push_back(std::vector<SyntaxStyle>());
                    continue;
                }
                
                if (isValid) {
                    try {
                        // Make a safe copy of the styles
                        result.push_back(cachedStyles_[line]->styles);
                    }
                    catch (const std::exception& ex) {
                        logManagerMessage(EditorException::Severity::Error,
                                         "SyntaxHighlightingManager::getHighlightingStyles",
                                         "Exception copying styles for line %zu: %s",
                                         line, ex.what());
                        result.push_back(std::vector<SyntaxStyle>());
                    }
                } else {
                    result.push_back(std::vector<SyntaxStyle>());
                }
            } else {
                result.push_back(std::vector<SyntaxStyle>());
            }
        } else {
            result.push_back(std::vector<SyntaxStyle>());
        }
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::getHighlightingStyles",
                     "Returning %zu style sets for requested range [%zu, %zu].",
                     result.size(), startLine, endLine);
    
    logLockRelease("SyntaxHighlightingManager::getHighlightingStyles", lockStart);
    return result;
}

void SyntaxHighlightingManager::invalidateLine(size_t line) {
    logLockAcquisition("SyntaxHighlightingManager::invalidateLine");
    auto lockStart = std::chrono::steady_clock::now();
    std::scoped_lock lock(mutex_);
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateLine",
                     "[ENTRY] Called for line %zu", line);
    logCacheMetrics("SyntaxHighlightingManager::invalidateLine");
    
    if (line < cachedStyles_.size() && cachedStyles_[line]) {
        // Mark the cache entry as invalid
        cachedStyles_[line]->valid.store(false, std::memory_order_release);
        invalidatedLines_.insert(line);
        lineTimestamps_.erase(line);
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateLine",
                     "[EXIT] Line %zu invalidated", line);
    logCacheMetrics("SyntaxHighlightingManager::invalidateLine");
    logLockRelease("SyntaxHighlightingManager::invalidateLine", lockStart);
}

void SyntaxHighlightingManager::invalidateLines(size_t startLine, size_t endLine) {
    logLockAcquisition("SyntaxHighlightingManager::invalidateLines");
    auto lockStart = std::chrono::steady_clock::now();
    std::scoped_lock lock(mutex_);
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateLines",
                     "[ENTRY] Called for lines %zu to %zu",
                     startLine, endLine);
    logCacheMetrics("SyntaxHighlightingManager::invalidateLines");
    
    const TextBuffer* buffer = getBuffer();
    if (!buffer) {
        logManagerMessage(EditorException::Severity::Warning,
                        "SyntaxHighlightingManager::invalidateLines",
                        "[EXIT] No buffer available");
        logLockRelease("SyntaxHighlightingManager::invalidateLines", lockStart);
        return;
    }
    
    // Clamp to buffer size
    if (buffer->lineCount() > 0) {
      endLine = std::min(endLine, buffer->lineCount() - 1);
    } else {
        logManagerMessage(EditorException::Severity::Warning,
                        "SyntaxHighlightingManager::invalidateLines",
                        "[EXIT] Buffer is empty");
        logLockRelease("SyntaxHighlightingManager::invalidateLines", lockStart);
        return;
    }
    
    if (startLine > endLine) {
        logManagerMessage(EditorException::Severity::Warning,
                        "SyntaxHighlightingManager::invalidateLines",
                        "[EXIT] Invalid range: start %zu > end %zu",
                        startLine, endLine);
        logLockRelease("SyntaxHighlightingManager::invalidateLines", lockStart);
        return;
    }

    size_t invalidatedCount = 0;
    for (size_t line_idx = startLine; line_idx <= endLine; ++line_idx) {
        if (line_idx < cachedStyles_.size() && cachedStyles_[line_idx]) {
            // Mark the cache entry as invalid
            cachedStyles_[line_idx]->valid.store(false, std::memory_order_release);
            invalidatedCount++;
        }
        invalidatedLines_.insert(line_idx);
        lineTimestamps_.erase(line_idx);
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateLines",
                     "[EXIT] Invalidated %zu lines in range [%zu, %zu]",
                     invalidatedCount, startLine, endLine);
    logCacheMetrics("SyntaxHighlightingManager::invalidateLines");
    logLockRelease("SyntaxHighlightingManager::invalidateLines", lockStart);
}

void SyntaxHighlightingManager::invalidateAllLines() {
    logLockAcquisition("SyntaxHighlightingManager::invalidateAllLines");
    auto lockStart = std::chrono::steady_clock::now();
    std::scoped_lock lock(mutex_);
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateAllLines",
                     "[ENTRY] Called");
    logCacheMetrics("SyntaxHighlightingManager::invalidateAllLines");
    
    auto invalidateStart = std::chrono::steady_clock::now();
    invalidateAllLines_nolock();
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateAllLines",
                     "[EXIT] All lines invalidated in %s",
                     formatDuration(invalidateStart).c_str());
    logCacheMetrics("SyntaxHighlightingManager::invalidateAllLines");
    logLockRelease("SyntaxHighlightingManager::invalidateAllLines", lockStart);
}

void SyntaxHighlightingManager::invalidateAllLines_nolock() {
    auto invalidateStart = std::chrono::steady_clock::now();
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::invalidateAllLines_nolock",
                     "[ENTRY] Called");
    logCacheMetrics("SyntaxHighlightingManager::invalidateAllLines_nolock");
    
    const TextBuffer* buffer = getBuffer();
    
    try {
        // First mark all cache entries as invalid
        auto markStart = std::chrono::steady_clock::now();
        size_t markedCount = 0;
        for (auto& entry : cachedStyles_) {
            if (entry) {
                entry->valid.store(false, std::memory_order_release);
                markedCount++;
            }
        }
        
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::invalidateAllLines_nolock",
                         "[MARK] Marked %zu entries as invalid in %s",
                         markedCount, formatDuration(markStart).c_str());
        
        // Then explicitly release all entries
        auto releaseStart = std::chrono::steady_clock::now();
        size_t releasedCount = 0;
        for (size_t i = 0; i < cachedStyles_.size(); ++i) {
            if (cachedStyles_[i]) {
                cachedStyles_[i].reset();
                releasedCount++;
            }
        }
        
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::invalidateAllLines_nolock",
                         "[RELEASE] Released %zu entries in %s",
                         releasedCount, formatDuration(releaseStart).c_str());
        
        // Clear the containers
        auto clearStart = std::chrono::steady_clock::now();
        cachedStyles_.clear();
        invalidatedLines_.clear();
        lineTimestamps_.clear();
        lineAccessTimes_.clear();
        
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::invalidateAllLines_nolock",
                         "[CLEAR] Cleared all containers in %s",
                         formatDuration(clearStart).c_str());
        
        // If buffer exists and has lines, mark them for re-highlight upon next request
        if (buffer && buffer->lineCount() > 0) {
            auto markLinesStart = std::chrono::steady_clock::now();
            size_t numLines = std::min(buffer->lineCount(), MAX_CACHE_LINES);
            for (size_t i = 0; i < numLines; ++i) {
                invalidatedLines_.insert(i);
            }
            
            logManagerMessage(EditorException::Severity::Warning,
                            "SyntaxHighlightingManager::invalidateAllLines_nolock",
                            "[MARK_LINES] Marked %zu lines for re-highlight in %s",
                            numLines, formatDuration(markLinesStart).c_str());
        }
        
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::invalidateAllLines_nolock",
                         "[EXIT] Completed in %s",
                         formatDuration(invalidateStart).c_str());
        logCacheMetrics("SyntaxHighlightingManager::invalidateAllLines_nolock");
        
    } catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error, 
                        "SyntaxHighlightingManager::invalidateAllLines_nolock", 
                         "[ERROR] Exception during invalidation: %s",
                         ex.what());
        throw;
    }
}

void SyntaxHighlightingManager::setVisibleRange(size_t startLine, size_t endLine) const {
    try {
        // Atomic stores don't need a lock
        visibleStartLine_.store(startLine, std::memory_order_release);
        visibleEndLine_.store(endLine, std::memory_order_release);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setVisibleRange: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setVisibleRange");
    }
}

void SyntaxHighlightingManager::setHighlightingTimeout(size_t timeoutMs) {
    try {
        // Atomic store doesn't need a lock
        highlightingTimeoutMs_.store(timeoutMs, std::memory_order_release);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setHighlightingTimeout: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setHighlightingTimeout");
    }
}

size_t SyntaxHighlightingManager::getHighlightingTimeout() const {
    try {
        // Atomic load doesn't need a lock
        return highlightingTimeoutMs_.load(std::memory_order_acquire);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        return DEFAULT_HIGHLIGHTING_TIMEOUT_MS;
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("getHighlightingTimeout: ") + ex.what(), EditorException::Severity::Error));
        return DEFAULT_HIGHLIGHTING_TIMEOUT_MS;
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::getHighlightingTimeout");
        return DEFAULT_HIGHLIGHTING_TIMEOUT_MS;
    }
}

void SyntaxHighlightingManager::setContextLines(size_t contextLines) {
    try {
        // Atomic store doesn't need a lock
        contextLines_.store(contextLines, std::memory_order_release);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setContextLines: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setContextLines");
    }
}

size_t SyntaxHighlightingManager::getContextLines() const {
    try {
        // Atomic load doesn't need a lock
        return contextLines_.load(std::memory_order_acquire);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        return DEFAULT_CONTEXT_LINES;
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("getContextLines: ") + ex.what(), EditorException::Severity::Error));
        return DEFAULT_CONTEXT_LINES;
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::getContextLines");
        return DEFAULT_CONTEXT_LINES;
    }
}

bool SyntaxHighlightingManager::isLineInCache(size_t line) const {
    std::scoped_lock<std::recursive_mutex> lock(mutex_); // Changed to scoped_lock
    return line < cachedStyles_.size() && cachedStyles_[line] != nullptr;
}

bool SyntaxHighlightingManager::isLineValid(size_t line) const {
    std::scoped_lock<std::recursive_mutex> lock(mutex_); // Changed to scoped_lock
    return line < cachedStyles_.size() && 
           cachedStyles_[line] &&
           cachedStyles_[line]->valid.load(std::memory_order_acquire) &&
           invalidatedLines_.count(line) == 0; 
}

// Calculate the effective range to highlight (with context)
std::pair<size_t, size_t> SyntaxHighlightingManager::calculateEffectiveRange(size_t startLine, size_t endLine) const {
    size_t visStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visEnd = visibleEndLine_.load(std::memory_order_acquire);
    
    // Use smaller context for non-visible lines
    size_t contextSize = (startLine >= visStart && endLine <= visEnd) ? 
                       DEFAULT_CONTEXT_LINES : NON_VISIBLE_CONTEXT_LINES;
    
    const TextBuffer* buffer = getBuffer();
    size_t effectiveStart = (startLine < contextSize) ? 0 : startLine - contextSize;
    size_t effectiveEnd = std::min(endLine + contextSize, 
                                 buffer ? buffer->lineCount() - 1 : endLine);
    
    return {effectiveStart, effectiveEnd};
}

void SyntaxHighlightingManager::cleanupCache_nolock() {
    auto cleanupStart = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    lastCleanupTime_ = currentTime;
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::cleanupCache_nolock",
                     "[ENTRY] Starting cache cleanup");
    logCacheMetrics("SyntaxHighlightingManager::cleanupCache_nolock");
    
    // First pass: Count valid entries and collect expired ones
    auto countStart = std::chrono::steady_clock::now();
    size_t validCount = 0;
    std::vector<size_t> expiredLines;
    expiredLines.reserve(cachedStyles_.size());
    
    for (size_t i = 0; i < cachedStyles_.size(); ++i) {
        logVectorAccess("cleanupCache_nolock:917", i, cachedStyles_.size());
        if (cachedStyles_[i]) {
            logVectorAccess("cleanupCache_nolock:919", i, cachedStyles_.size());
            if (cachedStyles_[i]->valid.load(std::memory_order_acquire)) {
                if (isEntryExpired_nolock(i)) {
                    expiredLines.push_back(i);
                } else {
                    validCount++;
                }
            }
        }
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::cleanupCache_nolock",
                     "[COUNT] Found %zu valid entries and %zu expired entries in %s",
                     validCount, expiredLines.size(), formatDuration(countStart).c_str());
    
    // Remove expired entries
    auto removeStart = std::chrono::steady_clock::now();
    for (size_t line : expiredLines) {
        if (line < cachedStyles_.size()) {  // Add bounds check
            cachedStyles_[line].reset();
            lineTimestamps_.erase(line);
            lineAccessTimes_.erase(line);
            invalidatedLines_.insert(line);
        }
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::cleanupCache_nolock",
                     "[REMOVE] Removed %zu expired entries in %s",
                     expiredLines.size(), formatDuration(removeStart).c_str());
    
    // If we're still over the limit after removing expired entries, use LRU eviction
    if (validCount > MAX_CACHE_LINES) {
        auto evictStart = std::chrono::steady_clock::now();
        evictLRUEntries_nolock(MAX_CACHE_LINES);
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::cleanupCache_nolock",
                         "[EVICT] LRU eviction took %s",
                         formatDuration(evictStart).c_str());
    }
    
    // Compact the cache if it's too sparse (more than 50% empty)
    size_t emptyCount = 0;
    for (const auto& entry : cachedStyles_) {
        if (!entry || !entry->valid.load(std::memory_order_acquire)) {
            emptyCount++;
        }
    }
    
    if (emptyCount > cachedStyles_.size() / 2) {
        auto compactStart = std::chrono::steady_clock::now();
        std::vector<std::shared_ptr<CacheEntry>> compactedCache;
        compactedCache.reserve(cachedStyles_.size() - emptyCount);
        
        // Create a mapping from old line numbers to new line numbers
        std::unordered_map<size_t, size_t> lineNumberMap;
        size_t newLineNumber = 0;
        
        // First pass - count valid entries and build mapping
        for (size_t i = 0; i < cachedStyles_.size(); ++i) {
            logVectorAccess("cleanupCache_nolock:964", i, cachedStyles_.size());
            if (cachedStyles_[i]) {
                logVectorAccess("cleanupCache_nolock:966", i, cachedStyles_.size());
                if (cachedStyles_[i]->valid.load(std::memory_order_acquire)) {
                    // Only record the mapping at this stage
                    lineNumberMap[i] = newLineNumber++;
                }
            }
        }
        
        // Second pass - actually move the entries after we have a complete mapping
        compactedCache.resize(newLineNumber);
        for (const auto& [oldLine, newLine] : lineNumberMap) {
            if (oldLine < cachedStyles_.size() && newLine < compactedCache.size()) {
                compactedCache[newLine] = std::move(cachedStyles_[oldLine]);
            }
        }
        
        // Update the other data structures with new line numbers
        std::unordered_map<size_t, std::chrono::steady_clock::time_point> newLineTimestamps;
        std::unordered_map<size_t, std::chrono::steady_clock::time_point> newLineAccessTimes;
        std::unordered_set<size_t> newInvalidatedLines;
        
        // Build new data structures with the new line numbers
        for (const auto& [oldLine, newLine] : lineNumberMap) {
            auto tsIt = lineTimestamps_.find(oldLine);
            if (tsIt != lineTimestamps_.end()) {
                newLineTimestamps[newLine] = tsIt->second;
            }
            
            auto accIt = lineAccessTimes_.find(oldLine);
            if (accIt != lineAccessTimes_.end()) {
                newLineAccessTimes[newLine] = accIt->second;
            }
            
            if (invalidatedLines_.count(oldLine) > 0) {
                newInvalidatedLines.insert(newLine);
            }
        }
        
        // Replace the old containers with the new ones
        cachedStyles_ = std::move(compactedCache);
        lineTimestamps_ = std::move(newLineTimestamps);
        lineAccessTimes_ = std::move(newLineAccessTimes);
        invalidatedLines_ = std::move(newInvalidatedLines);
        
        cachedStyles_.shrink_to_fit();
        
        // Validation step: ensure no invalid references remain
        std::vector<size_t> invalidKeys;
        for (const auto& [line, _] : lineTimestamps_) {
            if (line >= cachedStyles_.size()) {
                invalidKeys.push_back(line);
                logManagerMessage(EditorException::Severity::Error,
                                 "SyntaxHighlightingManager::compactCache",
                                 "Found invalid line reference in timestamps: %zu >= %zu",
                                 line, cachedStyles_.size());
            }
        }
        for (size_t key : invalidKeys) {
            lineTimestamps_.erase(key);
        }
        
        invalidKeys.clear();
        for (const auto& [line, _] : lineAccessTimes_) {
            if (line >= cachedStyles_.size()) {
                invalidKeys.push_back(line);
                logManagerMessage(EditorException::Severity::Error,
                                 "SyntaxHighlightingManager::compactCache",
                                 "Found invalid line reference in access times: %zu >= %zu",
                                 line, cachedStyles_.size());
            }
        }
        for (size_t key : invalidKeys) {
            lineAccessTimes_.erase(key);
        }
        
        std::vector<size_t> invalidLines;
        for (size_t line : invalidatedLines_) {
            if (line >= cachedStyles_.size()) {
                invalidLines.push_back(line);
                logManagerMessage(EditorException::Severity::Error,
                                 "SyntaxHighlightingManager::compactCache",
                                 "Found invalid line reference in invalidated lines: %zu >= %zu",
                                 line, cachedStyles_.size());
            }
        }
        for (size_t line : invalidLines) {
            invalidatedLines_.erase(line);
        }
        
        logManagerMessage(EditorException::Severity::Warning, 
                        "SyntaxHighlightingManager::cleanupCache_nolock",
                         "[COMPACT] Cache compaction took %s, new size: %zu",
                         formatDuration(compactStart).c_str(), cachedStyles_.size());
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::cleanupCache_nolock",
                     "[EXIT] Cache cleanup completed in %s",
                     formatDuration(cleanupStart).c_str());
    logCacheMetrics("SyntaxHighlightingManager::cleanupCache_nolock");
}

void SyntaxHighlightingManager::evictLRUEntries_nolock(size_t targetSize) {
    auto evictStart = std::chrono::steady_clock::now();
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::evictLRUEntries_nolock",
                     "[ENTRY] Target size: %zu", targetSize);
    logCacheMetrics("SyntaxHighlightingManager::evictLRUEntries_nolock");
    
    // Calculate how many entries to evict
    size_t currentSize = 0;
    for (const auto& entry : cachedStyles_) {
        if (entry && entry->valid.load(std::memory_order_acquire)) {
            currentSize++;
        }
    }
    
    if (currentSize <= targetSize) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::evictLRUEntries_nolock",
                         "[EXIT] No eviction needed, current size %zu <= target size %zu",
                         currentSize, targetSize);
        return;
    }
    
    size_t numToEvict = currentSize - targetSize;
    
    // Use a min-heap to efficiently find the least recently used entries
    using HeapEntry = std::pair<std::chrono::steady_clock::time_point, size_t>;
    std::priority_queue<HeapEntry,
                       std::vector<HeapEntry>,
                       std::greater<>> lruHeap;
    
            // Build heap more efficiently from lineAccessTimes_ directly (O(n) time)
    auto heapBuildStart = std::chrono::steady_clock::now();
    
    // Only consider entries that are within the cache size limit and are valid
    for (const auto& [line, accessTime] : lineAccessTimes_) {
        if (line < cachedStyles_.size() && cachedStyles_[line] && 
            cachedStyles_[line]->valid.load(std::memory_order_acquire)) {
            lruHeap.emplace(accessTime, line);
        }
    }
    
    // Log heap building progress but with less verbosity
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::evictLRUEntries_nolock",
                     "Built LRU heap with %zu entries from %zu total access times",
                     lruHeap.size(), lineAccessTimes_.size());
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::evictLRUEntries_nolock",
                     "[HEAP] Built heap of %zu entries in %s",
                     lruHeap.size(), formatDuration(heapBuildStart).c_str());
    
    // Process all entries at once instead of batches for efficiency
    size_t totalEvicted = 0;
    auto evictProcessStart = std::chrono::steady_clock::now();
    
    // Extract all necessary entries from the heap
    std::vector<size_t> linesToEvict;
    linesToEvict.reserve(numToEvict);
    
    for (size_t i = 0; i < numToEvict && !lruHeap.empty(); ++i) {
        linesToEvict.push_back(lruHeap.top().second);
        lruHeap.pop();
    }
    
    // Evict all at once
    for (size_t line : linesToEvict) {
        if (line < cachedStyles_.size()) {
            cachedStyles_[line].reset();
            lineTimestamps_.erase(line);
            lineAccessTimes_.erase(line);
            invalidatedLines_.insert(line);
            totalEvicted++;
        }
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::evictLRUEntries_nolock",
                     "Evicted %zu entries in one batch",
                     totalEvicted);
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::evictLRUEntries_nolock",
                     "[EVICT] Evicted %zu entries in %s",
                     totalEvicted, formatDuration(evictProcessStart).c_str());
    
    // Compact the cache if we evicted a significant number of entries
    if (totalEvicted > cachedStyles_.size() / 4) {
        auto compactStart = std::chrono::steady_clock::now();
        
        // Create a new vector for the compacted cache with appropriate capacity
        std::vector<std::shared_ptr<CacheEntry>> compactedCache;
        size_t estimatedSize = std::max(targetSize, cachedStyles_.size() - totalEvicted);
        compactedCache.reserve(estimatedSize);
        
        // Create a mapping from old line numbers to new line numbers
        std::unordered_map<size_t, size_t> lineNumberMap;
        size_t newLineNumber = 0;
        
        // First pass - count valid entries and build mapping
        for (size_t i = 0; i < cachedStyles_.size(); ++i) {
            logVectorAccess("evictLRUEntries_nolock:1129", i, cachedStyles_.size());
            if (cachedStyles_[i]) {
                logVectorAccess("evictLRUEntries_nolock:1131", i, cachedStyles_.size());
                if (cachedStyles_[i]->valid.load(std::memory_order_acquire)) {
                    // Only record the mapping at this stage
                    lineNumberMap[i] = newLineNumber++;
                }
            }
        }
        
        // Second pass - actually move the entries after we have a complete mapping
        compactedCache.resize(newLineNumber);
        for (const auto& [oldLine, newLine] : lineNumberMap) {
            if (oldLine < cachedStyles_.size() && newLine < compactedCache.size()) {
                compactedCache[newLine] = std::move(cachedStyles_[oldLine]);
            }
        }
        
        // Update the other data structures with new line numbers
        std::unordered_map<size_t, std::chrono::steady_clock::time_point> newLineTimestamps;
        std::unordered_map<size_t, std::chrono::steady_clock::time_point> newLineAccessTimes;
        std::unordered_set<size_t> newInvalidatedLines;
        
        // Build new data structures with the new line numbers
        for (const auto& [oldLine, newLine] : lineNumberMap) {
            auto tsIt = lineTimestamps_.find(oldLine);
            if (tsIt != lineTimestamps_.end()) {
                newLineTimestamps[newLine] = tsIt->second;
            }
            
            auto accIt = lineAccessTimes_.find(oldLine);
            if (accIt != lineAccessTimes_.end()) {
                newLineAccessTimes[newLine] = accIt->second;
            }
            
            if (invalidatedLines_.count(oldLine) > 0) {
                newInvalidatedLines.insert(newLine);
            }
        }
        
        // Replace the old containers with the new ones
        cachedStyles_ = std::move(compactedCache);
        lineTimestamps_ = std::move(newLineTimestamps);
        lineAccessTimes_ = std::move(newLineAccessTimes);
        invalidatedLines_ = std::move(newInvalidatedLines);
        
        cachedStyles_.shrink_to_fit();
        
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::evictLRUEntries_nolock",
                         "[COMPACT] Cache compaction took %s, new size: %zu",
                         formatDuration(compactStart).c_str(), cachedStyles_.size());
    }
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::evictLRUEntries_nolock",
                     "[EXIT] LRU eviction completed in %s",
                     formatDuration(evictStart).c_str());
    logCacheMetrics("SyntaxHighlightingManager::evictLRUEntries_nolock");
}

bool SyntaxHighlightingManager::isEntryExpired_nolock(size_t line) const {
    // Check if the entry exists and is valid
    if (line >= cachedStyles_.size()) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::isEntryExpired_nolock",
                         "Line %zu is out of bounds (size: %zu)",
                         line, cachedStyles_.size());
        return true;
    }
    
    logVectorAccess("isEntryExpired_nolock:1179", line, cachedStyles_.size());
    if (!cachedStyles_[line]) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::isEntryExpired_nolock",
                         "Cache entry for line %zu is null",
                         line);
        return true;
    }
    
    logVectorAccess("isEntryExpired_nolock:1184", line, cachedStyles_.size());
    
    // Recheck bounds again to guard against potential race conditions
    if (line >= cachedStyles_.size()) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::isEntryExpired_nolock",
                         "Race condition: Line %zu became out of bounds (size: %zu)",
                         line, cachedStyles_.size());
        return true;
    }
    
    if (!cachedStyles_[line]->valid.load(std::memory_order_acquire)) {
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::isEntryExpired_nolock",
                         "Cache entry for line %zu is invalid",
                         line);
        return true;
    }
    
    // Make a safe local copy of the timestamp to avoid potential race conditions
    std::chrono::steady_clock::time_point timestamp;
    try {
        // Final bounds check before accessing timestamp
        if (line >= cachedStyles_.size() || !cachedStyles_[line]) {
            logManagerMessage(EditorException::Severity::Warning,
                             "SyntaxHighlightingManager::isEntryExpired_nolock",
                             "Race condition: Cache entry for line %zu became null or out of bounds",
                             line);
            return true;
        }
        
        logVectorAccess("isEntryExpired_nolock:1195", line, cachedStyles_.size());
        timestamp = cachedStyles_[line]->timestamp;
    }
    catch (const std::exception& ex) {
        logManagerMessage(EditorException::Severity::Error,
                         "SyntaxHighlightingManager::isEntryExpired_nolock",
                         "Exception accessing timestamp for line %zu: %s",
                         line, ex.what());
        return true;
    }
    
    // Check if the entry has exceeded its lifetime
    auto currentTime = std::chrono::steady_clock::now();
    auto entryAge = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - timestamp);
    
    return entryAge.count() > CACHE_ENTRY_LIFETIME_MS;
}

bool SyntaxHighlightingManager::wasRecentlyProcessed(size_t line) const {
    auto it = recentlyProcessed_.find(line);
    if (it == recentlyProcessed_.end()) {
        return false;
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - it->second);
    return elapsed.count() < RECENTLY_PROCESSED_WINDOW_MS;
}

void SyntaxHighlightingManager::markAsRecentlyProcessed(size_t line) {
    recentlyProcessed_[line] = std::chrono::steady_clock::now();
    
    // Cleanup old entries periodically
    static const size_t CLEANUP_THRESHOLD = 1000;
    if (recentlyProcessed_.size() > CLEANUP_THRESHOLD) {
    auto now = std::chrono::steady_clock::now();
        for (auto it = recentlyProcessed_.begin(); it != recentlyProcessed_.end();) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - it->second);
            if (elapsed.count() >= RECENTLY_PROCESSED_WINDOW_MS) {
                it = recentlyProcessed_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::pair<size_t, size_t> SyntaxHighlightingManager::calculateOptimalProcessingRange(
    size_t requestedStart, size_t requestedEnd) const {
    
    // Get buffer bounds
    const TextBuffer* buffer = getBuffer();
    size_t maxLine = buffer ? buffer->lineCount() - 1 : requestedEnd;
    
    // Make sure we're not exceeding buffer size
    if (requestedEnd > maxLine) {
        requestedEnd = maxLine;
    }
    
    // Log the current state of lastProcessedRange_ for debugging
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::calculateOptimalProcessingRange",
                     "Last processed range: valid=%s, startLine=%zu, endLine=%zu. Current request: [%zu, %zu]",
                     lastProcessedRange_.valid ? "true" : "false",
                     lastProcessedRange_.startLine,
                     lastProcessedRange_.endLine,
                     requestedStart, requestedEnd);
    
    // Check for sequential access pattern - strict sequential check
    // Only consider it sequential if we're requesting the EXACT next line
    bool isSequential = lastProcessedRange_.valid && 
                       requestedStart == lastProcessedRange_.endLine + 1;
    
    // For sequential access, use VERY minimal range - only process exactly what's needed
    // plus a small lookahead to anticipate the next few lines
    if (isSequential) {
        // Minimal lookahead for sequential access - just a few lines
        static constexpr size_t SEQUENTIAL_LOOKAHEAD = 5;
        
        // For sequential access, ONLY process the exact requested range plus small lookahead
        // NO BACKWARD CONTEXT at all for sequential access
        size_t effectiveEnd = std::min(requestedEnd + SEQUENTIAL_LOOKAHEAD, maxLine);
        
        logManagerMessage(EditorException::Severity::Warning,
                         "SyntaxHighlightingManager::calculateOptimalProcessingRange",
                         "Sequential access detected: [%zu, %zu] -> [%zu, %zu] (STRICT MINIMAL mode)",
                         lastProcessedRange_.endLine, requestedStart,
                         requestedStart, effectiveEnd);
                         
        return {requestedStart, effectiveEnd};
    }
    
    // For non-sequential access, calculate visible-aware context
    size_t visStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visEnd = visibleEndLine_.load(std::memory_order_acquire);
    
    // Use smaller context for non-visible lines
    size_t contextSize = (requestedStart >= visStart && requestedEnd <= visEnd) ? 
                      DEFAULT_CONTEXT_LINES : NON_VISIBLE_CONTEXT_LINES;
    
    // Calculate effective range with context
    size_t effectiveStart = (requestedStart < contextSize) ? 0 : requestedStart - contextSize;
    size_t effectiveEnd = std::min(requestedEnd + contextSize, maxLine);
    
    logManagerMessage(EditorException::Severity::Warning,
                     "SyntaxHighlightingManager::calculateOptimalProcessingRange",
                     "Non-sequential access: [%zu, %zu] -> [%zu, %zu] (context: %zu, maxLine: %zu)",
                     requestedStart, requestedEnd,
                     effectiveStart, effectiveEnd,
                     contextSize, maxLine);
    
    return {effectiveStart, effectiveEnd};
}

// Logging helpers
void SyntaxHighlightingManager::logLockAcquisition(const char* method) const {
    auto now = std::chrono::steady_clock::now();
    logManagerMessage(EditorException::Severity::Warning, method,
                     "Acquiring lock at %s",
                     std::to_string(now.time_since_epoch().count()).c_str());
}

void SyntaxHighlightingManager::logLockRelease(const char* method, const std::chrono::steady_clock::time_point& start) const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    logManagerMessage(EditorException::Severity::Warning, method,
                     "Releasing lock after %lld ms",
                     duration.count());
}

void SyntaxHighlightingManager::logCacheMetrics(const char* method) const {
    size_t validCount = 0;
    size_t invalidCount = 0;
    for (const auto& entry : cachedStyles_) {
        if (entry) {
            if (entry->valid.load(std::memory_order_acquire)) {
                validCount++;
            } else {
                invalidCount++;
            }
        }
    }
    logManagerMessage(EditorException::Severity::Warning, method,
                     "Cache metrics - Valid: %zu, Invalid: %zu, Total: %zu",
                     validCount, invalidCount, cachedStyles_.size());
}

