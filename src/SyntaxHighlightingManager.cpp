#include "SyntaxHighlightingManager.h"
#include <iostream>
#include <thread>
#include "EditorError.h"

namespace {
    // New logger function using ErrorReporter
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
        // Use shared lock for reading
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        // We can only return the raw pointer - in a real implementation, we'd need
        // to maintain a map of raw pointers to shared_ptrs or use a different approach
        SyntaxHighlighter* raw_ptr = highlighter_->load(std::memory_order_acquire);
        
        // This is a simplification - in a real implementation, we'd need proper lifetime management
        // For now, treating this as a weak reference (non-owning)
        // To correctly return a shared_ptr, we need to ensure the object remains alive.
        // This requires a more complex setup if the manager doesn't own the highlighter.
        // For now, assuming the caller or context ensures lifetime if raw_ptr is not null.
        // If the goal is to truly share ownership, highlighter_ itself should be std::shared_ptr<SyntaxHighlighter>
        // and not std::shared_ptr<std::atomic<SyntaxHighlighter*>>.
        // Given the current structure, returning a shared_ptr that observes the raw pointer is risky.
        // A raw pointer or a weak_ptr (if highlighter_ was a shared_ptr) would be safer.
        // Sticking to the previous return type but acknowledging the risk.
        if(raw_ptr) {
             // This is problematic as the shared_ptr doesn't own the object unless highlighter itself is a shared_ptr to the object.
             // This custom deleter does nothing, meaning it doesn't extend lifetime.
             // This was how it was before, so retaining for now.
            return std::shared_ptr<SyntaxHighlighter>(raw_ptr, [](SyntaxHighlighter*){});
        }
        return nullptr;
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
        std::scoped_lock lock(mutex_);
        bool oldValue = enabled_.load(std::memory_order_acquire);
        if (oldValue != enabled) {
            enabled_.store(enabled, std::memory_order_release);
            invalidateAllLines_nolock();
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
        std::scoped_lock lock(mutex_);
        buffer_.store(buffer, std::memory_order_release);
        invalidateAllLines_nolock();
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("setBuffer: ") + ex.what(), EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::setBuffer");
    }
}

std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(
    size_t startLine, size_t endLine) {
    std::vector<std::vector<SyntaxStyle>> empty_result;
    if (startLine > endLine) return empty_result; // Basic sanity check
    size_t num_lines = endLine - startLine + 1;
    empty_result.resize(num_lines);


    try {
        std::scoped_lock lock(mutex_);
        
        // Get buffer and check enabled status
        const TextBuffer* buffer = getBuffer();
        SyntaxHighlighter* highlighter = highlighter_->load(std::memory_order_acquire);
        bool enabled = enabled_.load(std::memory_order_acquire);
        
        // Check if highlighting is disabled or no buffer/highlighter
        if (!enabled || !buffer || !highlighter) {
            return empty_result;
        }
        
        // Safety checks
        if (buffer->isEmpty()) {
            return std::vector<std::vector<SyntaxStyle>>(); // Return truly empty, not sized
        }
        
        // Clamp end line to buffer size
        size_t actualEndLine = std::min(endLine, buffer->lineCount() - 1);
         if (startLine >= buffer->lineCount()) { // If startline is out of bounds
            return empty_result;
        }
        if (startLine > actualEndLine && buffer->lineCount() > 0) { // if startLine valid but became > actualEndLine due to clamping
             empty_result.resize(0); // No lines to return styles for
             return empty_result;
        }
        if (buffer->lineCount() == 0) { // handles case where buffer has 0 lines
             empty_result.resize(0);
             return empty_result;
        }


        // For non-const version, update the cache for the effective range
        auto effectiveRange = calculateEffectiveRange(startLine, actualEndLine);
        size_t effectiveStart = effectiveRange.first;
        size_t effectiveEnd = effectiveRange.second;
        
        // Highlight invalidated lines within the effective range
        std::chrono::milliseconds timeout(highlightingTimeoutMs_.load(std::memory_order_acquire));
        highlightLines(effectiveStart, effectiveEnd, timeout);
        
        // Periodically clean up the cache
        cleanupCache_nolock();
        
        // Extract the requested range from the cache
        std::vector<std::vector<SyntaxStyle>> result;
        if (startLine > actualEndLine) { // if after all calculations, range is invalid
            return result; // empty result
        }
        result.reserve(actualEndLine - startLine + 1);
        
        for (size_t line = startLine; line <= actualEndLine; ++line) {
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
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        return empty_result; // Return appropriately sized empty result
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("getHighlightingStyles: ") + ex.what(), EditorException::Severity::Error));
        return empty_result; // Return appropriately sized empty result
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::getHighlightingStyles");
        return empty_result; // Return appropriately sized empty result
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
    catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        if (line < cachedStyles_.size() && cachedStyles_[line]) {
            cachedStyles_[line]->valid.store(false, std::memory_order_release);
        }
    }
    catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("highlightLine: line ") + std::to_string(line) + ": " + ex.what(), EditorException::Severity::Error));
        if (line < cachedStyles_.size() && cachedStyles_[line]) {
            cachedStyles_[line]->valid.store(false, std::memory_order_release);
        }
    }
    catch (...) {
        ErrorReporter::logUnknownException(std::string("SyntaxHighlightingManager::highlightLine for line ") + std::to_string(line));
        if (line < cachedStyles_.size() && cachedStyles_[line]) {
            cachedStyles_[line]->valid.store(false, std::memory_order_release);
        }
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

void SyntaxHighlightingManager::cleanupCache_nolock() {
    auto now = std::chrono::steady_clock::now();
    size_t removedCount = 0;
    size_t initialCacheSize = lineTimestamps_.size();

    for (auto it = lineTimestamps_.begin(); it != lineTimestamps_.end(); /* manual increment */) {
        size_t line = it->first;
        auto timestamp = it->second;
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);

        if (age.count() > CACHE_ENTRY_LIFETIME_MS) {
            if (line < cachedStyles_.size() && cachedStyles_[line]) {
                cachedStyles_[line]->valid.store(false, std::memory_order_release); // Mark as invalid
                // Optionally, could reset the shared_ptr to release memory: cachedStyles_[line].reset();
                // but be cautious if other parts might hold a reference briefly.
                // For now, just marking as invalid is safer.
            }
            it = lineTimestamps_.erase(it); // Remove from timestamp map
            invalidatedLines_.insert(line); // Mark as needing re-highlight if requested again
            removedCount++;
        } else {
            ++it;
        }
    }

    if (removedCount > 0) {
        logManagerMessage(EditorException::Severity::Warning, // Or Info if we had it; Warning for now.
                          "SyntaxHighlightingManager::cleanupCache_nolock",
                          "Cache cleanup: %zu entries before, %zu removed due to age, %zu timestamps remaining.",
                          initialCacheSize, removedCount, lineTimestamps_.size());
    }
} 