#include "SyntaxHighlightingManager.h"
#include <iostream>

// Utility to log critical errors during initialization
namespace {
    void logCriticalError(const char* location, const char* message) {
        try {
            std::cerr << "[CRITICAL ERROR] " << location << ": " << message << std::endl;
        } catch (...) {
            // Last resort - silently continue if we can't even log
        }
    }
}

SyntaxHighlightingManager::SyntaxHighlightingManager()
    : buffer_(nullptr), 
      highlighter_(nullptr), 
      enabled_(true),
      visibleStartLine_(0), 
      visibleEndLine_(0),
      highlightingTimeoutMs_(DEFAULT_HIGHLIGHTING_TIMEOUT_MS),
      contextLines_(DEFAULT_CONTEXT_LINES) {
}

void SyntaxHighlightingManager::setHighlighter(SyntaxHighlighter* highlighter) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        highlighter_ = highlighter;
        invalidateAllLines_nolock();
    } catch (const std::exception& ex) {
        logCriticalError("setHighlighter", ex.what());
    } catch (...) {
        logCriticalError("setHighlighter", "Unknown exception");
    }
}

SyntaxHighlighter* SyntaxHighlightingManager::getHighlighter() const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return highlighter_;
    } catch (const std::exception& ex) {
        logCriticalError("getHighlighter", ex.what());
        return nullptr;
    } catch (...) {
        logCriticalError("getHighlighter", "Unknown exception");
        return nullptr;
    }
}

void SyntaxHighlightingManager::setEnabled(bool enabled) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (enabled_ != enabled) {
            enabled_ = enabled;
            invalidateAllLines_nolock();
        }
    } catch (const std::exception& ex) {
        logCriticalError("setEnabled", ex.what());
    } catch (...) {
        logCriticalError("setEnabled", "Unknown exception");
    }
}

bool SyntaxHighlightingManager::isEnabled() const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return enabled_;
    } catch (const std::exception& ex) {
        logCriticalError("isEnabled", ex.what());
        return false;
    } catch (...) {
        logCriticalError("isEnabled", "Unknown exception");
        return false;
    }
}

void SyntaxHighlightingManager::setBuffer(const TextBuffer* buffer) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_ = buffer;
        invalidateAllLines_nolock();
    } catch (const std::exception& ex) {
        logCriticalError("setBuffer", ex.what());
    } catch (...) {
        logCriticalError("setBuffer", "Unknown exception");
    }
}

std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(
    size_t startLine, size_t endLine) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if highlighting is disabled or no buffer/highlighter
        if (!enabled_ || !buffer_ || !highlighter_) {
            return std::vector<std::vector<SyntaxStyle>>(
                endLine - startLine + 1, std::vector<SyntaxStyle>());
        }
        
        // Safety checks
        if (buffer_->isEmpty()) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        // Clamp end line to buffer size
        endLine = std::min(endLine, buffer_->lineCount() - 1);
        
        // If start line is beyond end line, return empty result
        if (startLine > endLine) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        // For non-const version, update the cache for the effective range
        auto effectiveRange = calculateEffectiveRange(startLine, endLine);
        size_t effectiveStart = effectiveRange.first;
        size_t effectiveEnd = effectiveRange.second;
        
        // Highlight invalidated lines within the effective range
        std::chrono::milliseconds timeout(highlightingTimeoutMs_);
        highlightLines(effectiveStart, effectiveEnd, timeout);
        
        // Periodically clean up the cache
        cleanupCache();
        
        // Extract the requested range from the cache
        std::vector<std::vector<SyntaxStyle>> result;
        result.reserve(endLine - startLine + 1);
        
        for (size_t line = startLine; line <= endLine; ++line) {
            if (isLineInCache(line) && isLineValid(line)) {
                result.push_back(cachedStyles_[line]);
            } else {
                // For lines that weren't highlighted (due to timeout), return empty styles
                result.push_back(std::vector<SyntaxStyle>());
            }
        }
        
        return result;
    } catch (const std::exception& ex) {
        logCriticalError("getHighlightingStyles", ex.what());
        return std::vector<std::vector<SyntaxStyle>>();
    } catch (...) {
        logCriticalError("getHighlightingStyles", "Unknown exception");
        return std::vector<std::vector<SyntaxStyle>>();
    }
}

void SyntaxHighlightingManager::invalidateLine(size_t line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (line < cachedStyles_.size()) {
        invalidatedLines_.insert(line);
        lineTimestamps_.erase(line);
    }
}

void SyntaxHighlightingManager::invalidateLines(size_t startLine, size_t endLine) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!buffer_) return;
    
    // Clamp to buffer size
    endLine = std::min(endLine, buffer_->lineCount() - 1);
    
    for (size_t line = startLine; line <= endLine; ++line) {
        invalidatedLines_.insert(line);
        lineTimestamps_.erase(line);
    }
}

void SyntaxHighlightingManager::invalidateAllLines() {
    std::lock_guard<std::mutex> lock(mutex_);
    invalidateAllLines_nolock();
}

void SyntaxHighlightingManager::invalidateAllLines_nolock() {
    if (!buffer_) return;
    
    // Clear the cache and invalidate all lines
    cachedStyles_.clear();
    invalidatedLines_.clear();
    lineTimestamps_.clear();
    
    if (buffer_->lineCount() > 0) {
        for (size_t i = 0; i < buffer_->lineCount(); ++i) {
            invalidatedLines_.insert(i);
        }
    }
}

void SyntaxHighlightingManager::setVisibleRange(size_t startLine, size_t endLine) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    visibleStartLine_ = startLine;
    visibleEndLine_ = endLine;
}

void SyntaxHighlightingManager::setHighlightingTimeout(size_t timeoutMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    highlightingTimeoutMs_ = timeoutMs;
}

size_t SyntaxHighlightingManager::getHighlightingTimeout() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return highlightingTimeoutMs_;
}

void SyntaxHighlightingManager::setContextLines(size_t contextLines) {
    std::lock_guard<std::mutex> lock(mutex_);
    contextLines_ = contextLines;
}

size_t SyntaxHighlightingManager::getContextLines() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return contextLines_;
}

void SyntaxHighlightingManager::highlightLine(size_t line) {
    if (!buffer_ || !highlighter_ || !enabled_) return;
    
    // Safety check
    if (line >= buffer_->lineCount()) return;
    
    try {
        // Get the line text
        const std::string& lineText = buffer_->getLine(line);
        
        // Highlight the line
        std::vector<SyntaxStyle> lineStyles = highlighter_->highlightLine(lineText, line);
        
        // Ensure cache is large enough
        if (cachedStyles_.size() <= line) {
            cachedStyles_.resize(line + 1);
        }
        
        // Store in cache
        cachedStyles_[line] = std::move(lineStyles);
        
        // Update timestamp
        lineTimestamps_[line] = std::chrono::steady_clock::now();
        
        // Remove from invalidated set
        invalidatedLines_.erase(line);
    }
    catch (const SyntaxHighlightingException& ex) {
        ErrorReporter::logException(ex);
    }
    catch (const std::exception& ex) {
        ErrorReporter::logException(ex);
    }
    catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::highlightLine");
    }
}

bool SyntaxHighlightingManager::highlightLines(size_t startLine, size_t endLine, 
                                               const std::chrono::milliseconds& timeout) {
    if (!buffer_ || !highlighter_ || !enabled_) return false;
    
    // Safety checks
    if (buffer_->isEmpty()) return false;
    
    // Clamp end line to buffer size
    endLine = std::min(endLine, buffer_->lineCount() - 1);
    
    // If start line is beyond end line, return
    if (startLine > endLine) return false;
    
    // Start timing
    auto startTime = std::chrono::steady_clock::now();
    
    // Create a priority list of lines to process
    std::vector<size_t> linesToProcess;
    
    // First add visible invalidated lines
    for (size_t line = visibleStartLine_; line <= visibleEndLine_ && line <= endLine; ++line) {
        if (invalidatedLines_.find(line) != invalidatedLines_.end()) {
            linesToProcess.push_back(line);
        }
    }
    
    // Then add non-visible invalidated lines
    for (size_t line = startLine; line <= endLine; ++line) {
        if (line >= visibleStartLine_ && line <= visibleEndLine_) {
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
            return i >= (visibleEndLine_ - visibleStartLine_ + 1);
        }
    }
    
    return true; // All lines processed
}

bool SyntaxHighlightingManager::isLineInCache(size_t line) const {
    return line < cachedStyles_.size();
}

bool SyntaxHighlightingManager::isLineValid(size_t line) const {
    return isLineInCache(line) && invalidatedLines_.find(line) == invalidatedLines_.end();
}

std::pair<size_t, size_t> SyntaxHighlightingManager::calculateEffectiveRange(
    size_t startLine, size_t endLine) const {
    
    if (!buffer_) return {startLine, endLine};
    
    // Start with the requested range
    size_t effectiveStart = startLine;
    size_t effectiveEnd = endLine;
    
    // Add context lines before
    if (startLine >= contextLines_) {
        effectiveStart = startLine - contextLines_;
    } else {
        effectiveStart = 0;
    }
    
    // Add context lines after
    effectiveEnd = std::min(endLine + contextLines_, buffer_->lineCount() - 1);
    
    return {effectiveStart, effectiveEnd};
}

void SyntaxHighlightingManager::cleanupCache() {
    if (lineTimestamps_.empty()) return;
    
    auto now = std::chrono::steady_clock::now();
    std::vector<size_t> linesToInvalidate;
    
    // Find old entries
    for (const auto& [line, timestamp] : lineTimestamps_) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
        
        // Skip lines in visible range
        if (line >= visibleStartLine_ && line <= visibleEndLine_) {
            continue;
        }
        
        // Invalidate if too old
        if (age.count() > CACHE_ENTRY_LIFETIME_MS) {
            linesToInvalidate.push_back(line);
        }
    }
    
    // Remove from cache and timestamp tracking
    for (size_t line : linesToInvalidate) {
        if (line < cachedStyles_.size()) {
            cachedStyles_[line].clear();
            cachedStyles_[line].shrink_to_fit();
        }
        lineTimestamps_.erase(line);
        invalidatedLines_.insert(line);
    }
} 