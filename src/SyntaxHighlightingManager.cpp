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

        if (highlighter_) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Resetting highlighter_ manually. Current use_count: %ld", highlighter_.use_count());
            highlighter_.reset();
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ has been reset.");
        } else {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "highlighter_ was already null before explicit reset attempt.");
        }
        
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Clearing cachedStyles_ manually.");
        cachedStyles_.clear(); 
        // cachedStyles_.shrink_to_fit(); // Consider if memory should be explicitly returned
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "cachedStyles_ cleared. New size: %zu, capacity: %zu", cachedStyles_.size(), cachedStyles_.capacity());

        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "Clearing invalidatedLines_ and lineTimestamps_ manually.");
        invalidatedLines_.clear();
        lineTimestamps_.clear();
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::~SyntaxHighlightingManager", "invalidatedLines_ and lineTimestamps_ cleared.");

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

// Thread-safe getter for buffer
const TextBuffer* SyntaxHighlightingManager::getBuffer() const {
    return buffer_.load(std::memory_order_acquire);
}

// Non-locking getter for highlighter - for internal use when mutex is already held
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
void SyntaxHighlightingManager::highlightLine_nolock(size_t line) {
    // Assumes mutex_ is already locked by the caller (e.g., public highlightLine or highlightLines_nolock)
    // Assumes cachedStyles_ has been resized appropriately by the caller if needed.

    const TextBuffer* buffer = getBuffer(); // Atomic read
    SyntaxHighlighter* highlighter = getHighlighterPtr_nolock(); // Uses internal getter, assumes lock held
    bool enabled = enabled_.load(std::memory_order_acquire); // Atomic read

    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine_nolock", "Called for line %zu.", line);

    // Basic checks already done by public highlightLine, but can be re-checked for safety if called from other _nolock contexts.
    // For this refactor, we assume public highlightLine did the essential checks (buffer, highlighter, enabled, line bounds)
    // before calling this _nolock version.
    // However, direct access to buffer->getLine requires buffer to be non-null and line to be in bounds.
    if (!buffer || !highlighter || !enabled || line >= buffer->lineCount()) {
         logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine_nolock", "Skipping line %zu due to unmet preconditions (buffer=%p, highlighter=%p, enabled=%s, line_bounds=%s)", 
line, static_cast<const void*>(buffer), static_cast<void*>(highlighter), enabled?"true":"false", (buffer && line < buffer->lineCount())?"ok":"bad");
        return; // Caller should have ensured this.
    }

    std::vector<SyntaxStyle> lineStyles;
    bool exception_during_highlight = false;

    try {
        const std::string& lineText = buffer->getLine(line);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine_nolock", "Highlighting line %zu text: \"%s\"", line, lineText.substr(0, 50).c_str());
        lineStyles = highlighter->highlightLine(lineText, line);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine_nolock", "Got %zu styles for line %zu.", lineStyles.size(), line);
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::highlightLine_nolock", "EditorException caught highlighting line %zu: %s", line, ed_ex.what());
        exception_during_highlight = true;
    } catch (const std::runtime_error& e) {
        ErrorReporter::logError(std::string("Runtime error highlighting line ") + std::to_string(line) + ": " + e.what());
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::highlightLine_nolock", "Runtime error highlighting line %zu: %s", line, e.what());
        exception_during_highlight = true;
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("highlightLine_nolock: ") + ex.what() + " on line " + std::to_string(line), EditorException::Severity::Error));
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::highlightLine_nolock", "std::exception caught highlighting line %zu: %s", line, ex.what());
        exception_during_highlight = true;
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::highlightLine_nolock line " + std::to_string(line));
        logManagerMessage(EditorException::Severity::Critical, "SyntaxHighlightingManager::highlightLine_nolock", "Unknown exception caught highlighting line %zu.", line);
        exception_during_highlight = true;
    }

    // Update cache - this is safe as caller holds unique_lock and ensures cacheStyles_ is sized.
    if (line < cachedStyles_.size()) { // Should always be true due to caller's resize.
        if (exception_during_highlight) {
            cachedStyles_[line] = std::make_unique<CacheEntry>(std::vector<SyntaxStyle>()); 
        } else {
            cachedStyles_[line] = std::make_unique<CacheEntry>(std::move(lineStyles));
        }
        // Mark as valid (CacheEntry constructor sets valid=true by default)
        // cachedStyles_[line]->valid.store(true, std::memory_order_release); // Already true by CacheEntry constructor
        invalidatedLines_.erase(line); // No longer invalidated
        lineTimestamps_[line] = std::chrono::steady_clock::now(); // Update timestamp
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLine_nolock", "Cache updated for line %zu. Exception during highlight: %s", line, exception_during_highlight ? "true" : "false");
    } else {
        // This case should ideally not be reached if caller manages cache size correctly.
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::highlightLine_nolock", "Cache access out of bounds for line %zu after processing. Cache size: %zu. STYLES LOST.", line, cachedStyles_.size());
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
        if (cachedStyles_.size() <= line) {
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
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "[ENTRY] Called for lines %zu to %zu with timeout %lld ms.", startLine, endLine, timeout.count());

    const TextBuffer* buffer = getBuffer(); // Atomic read
    // SyntaxHighlighter* highlighter = getHighlighterPtr_nolock(); // Already got by getHighlightingStyles, not needed here directly
    // bool enabled = enabled_.load(std::memory_order_acquire); // Already checked by getHighlightingStyles
    
    // Basic checks for buffer should be done by caller (getHighlightingStyles)
    if (!buffer || buffer->isEmpty()) {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Skipping: No buffer or buffer is empty (should be caught by caller).");
        return false;
    }
    
    // Clamp end line to buffer size (caller should also do this, but defensive check)
    if (buffer->lineCount() > 0) {
        endLine = std::min(endLine, buffer->lineCount() - 1);
    } else {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Skipping: Buffer has 0 lines.");
        return false; // Should be caught by buffer->isEmpty() earlier
    }

    if (startLine > endLine) {
         logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Skipping: startLine %zu > endLine %zu.", startLine, endLine);
        return false; 
    }

    // Caller (getHighlightingStyles) ensures cachedStyles_ is adequately sized for effectiveEnd.
    // We iterate from startLine to endLine passed to this function (which are effectiveStart/End from caller)

    auto startTime = std::chrono::steady_clock::now();
    std::vector<size_t> linesToProcess;
    size_t visStart = visibleStartLine_.load(std::memory_order_acquire);
    size_t visEnd = visibleEndLine_.load(std::memory_order_acquire);

    // Determine lines that need processing based on invalidation or missing cache entries
    // This loop iterates over the *effective range* passed to this function.
    for (size_t line_iter = startLine; line_iter <= endLine; ++line_iter) {
        if (line_iter < cachedStyles_.size()) { // Check if line_iter is within current cache bounds
            if (invalidatedLines_.count(line_iter) || !cachedStyles_[line_iter] || !cachedStyles_[line_iter]->valid.load(std::memory_order_acquire)) {
                linesToProcess.push_back(line_iter);
            }
        } else {
            // Line is outside current cache size, definitely needs processing if we expect cache to grow up to `endLine`.
            // The caller (getHighlightingStyles) is responsible for resizing cachedStyles_ up to `effectiveEnd`.
            // So, if line_iter (which is part of effectiveRange) is >= cachedStyles_.size(), it implies it needs processing.
            linesToProcess.push_back(line_iter);
        }
    }
    
    // Simple reordering: put visible lines first if they are in linesToProcess
    std::vector<size_t> prioritizedLines;
    std::vector<size_t> otherLines;
    for (size_t line : linesToProcess) {
        if (line >= visStart && line <= visEnd) {
            prioritizedLines.push_back(line);
        } else {
            otherLines.push_back(line);
        }
    }
    linesToProcess.assign(prioritizedLines.begin(), prioritizedLines.end());
    linesToProcess.insert(linesToProcess.end(), otherLines.begin(), otherLines.end());

    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Found %zu lines to process (prioritized).", linesToProcess.size());
    bool anyVisibleProcessed = false;

    for (size_t currentLine : linesToProcess) {
        // Ensure currentLine is within the originally passed [startLine, endLine] for safety,
        // though linesToProcess should only contain these.
        if (currentLine < startLine || currentLine > endLine) continue; 

        auto currentTimeCheck = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTimeCheck - startTime) >= timeout) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Timeout BEFORE processing line %zu. Elapsed: %lld ms.", currentLine, std::chrono::duration_cast<std::chrono::milliseconds>(currentTimeCheck - startTime).count());
            return anyVisibleProcessed;
        }
        
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Processing line %zu", currentLine);
        // Caller (getHighlightingStyles) ensures cachedStyles_ is sized up to `effectiveEnd` (which is `endLine` for this func)
        // highlightLine_nolock also relies on this sizing.
        highlightLine_nolock(currentLine);
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Finished processing line %zu.", currentLine);
        
        if (currentLine >= visStart && currentLine <= visEnd) {
            anyVisibleProcessed = true;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime) >= timeout) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "Timeout AFTER processing line %zu. Elapsed: %lld ms.", currentLine, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
            return anyVisibleProcessed;
        }
    }
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::highlightLines_nolock", "All %zu lines processed within timeout.", linesToProcess.size());
    return true; // Or return anyVisibleProcessed if that's more meaningful for partial success.
}

std::vector<std::vector<SyntaxStyle>> SyntaxHighlightingManager::getHighlightingStyles(
    size_t startLine, size_t endLine) {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Called for lines %zu to %zu.", startLine, endLine);
    std::vector<std::vector<SyntaxStyle>> empty_result_shell;
    if (startLine > endLine) {
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Start line %zu > end line %zu. Returning empty.", startLine, endLine);
        return empty_result_shell; 
    }
    // Pre-size for failure cases or if range is valid but all lines are empty/invalidated later
    empty_result_shell.resize(std::max(0, (int)endLine - (int)startLine + 1), std::vector<SyntaxStyle>());

    try {
        std::scoped_lock lock(mutex_); // Acquire UNIQUE lock for the duration of this method
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Acquired unique_lock. Processing for lines %zu to %zu.", startLine, endLine);
        
        const TextBuffer* buffer = getBuffer();
        SyntaxHighlighter* highlighter = getHighlighterPtr_nolock();
        bool enabled = enabled_.load(std::memory_order_acquire);
        
        if (!enabled || !buffer || !highlighter) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Disabled or no buffer/highlighter. Returning empty shell.");
            return empty_result_shell;
        }
        if (buffer->isEmpty()) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Buffer is empty. Returning actual empty vector (not shell).");
            return std::vector<std::vector<SyntaxStyle>>(); // Return truly empty, not shell of empty vectors
        }
        
        size_t actualEndLine = endLine;
        size_t currentBufferLineCount = buffer->lineCount();
        if (currentBufferLineCount > 0) {
            actualEndLine = std::min(endLine, currentBufferLineCount - 1);
        } else { 
             logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Buffer line count is 0 post-check. Returning actual empty vector.");
            return std::vector<std::vector<SyntaxStyle>>();
        }

        // Adjust startLine if it's out of bounds relative to the (potentially now empty) buffer
        if (startLine >= currentBufferLineCount && currentBufferLineCount > 0) {
             logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "startLine %zu >= lineCount %zu. Returning actual empty vector.", startLine, currentBufferLineCount);
            return std::vector<std::vector<SyntaxStyle>>();
        }
         // if startLine is still > actualEndLine after clamping actualEndLine, the range is invalid.
        if (startLine > actualEndLine) {
             logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "startLine %zu > actualEndLine %zu. Returning actual empty vector.", startLine, actualEndLine);
            return std::vector<std::vector<SyntaxStyle>>();
        }

        auto effectiveRange = calculateEffectiveRange(startLine, actualEndLine);
        size_t effectiveStart = effectiveRange.first;
        size_t effectiveEnd = effectiveRange.second;
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Effective range for processing: [%zu, %zu]. Requested range for result: [%zu, %zu]", effectiveStart, effectiveEnd, startLine, actualEndLine);
        
        // Ensure cache is large enough for the entire effective range to be processed by _nolock methods
        if (cachedStyles_.size() <= effectiveEnd) {
            logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Resizing cache from %zu to %zu for effectiveEnd %zu.", cachedStyles_.size(), effectiveEnd + 1, effectiveEnd);
            cachedStyles_.resize(effectiveEnd + 1);
        }
        
        std::chrono::milliseconds timeout(highlightingTimeoutMs_.load(std::memory_order_acquire));
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Calling highlightLines_nolock for effective range [%zu, %zu].", effectiveStart, effectiveEnd);
        
        // Call the new _nolock version
        highlightLines_nolock(effectiveStart, effectiveEnd, timeout);
        
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Returned from highlightLines_nolock.");
        
        cleanupCache_nolock();
        
        // Prepare result for the originally requested range [startLine, actualEndLine]
        std::vector<std::vector<SyntaxStyle>> result;
        result.reserve(actualEndLine - startLine + 1);
        
        for (size_t line_idx = startLine; line_idx <= actualEndLine; ++line_idx) {
            // Ensure line_idx is within the bounds of the potentially resized cachedStyles_
            if (line_idx < cachedStyles_.size() && cachedStyles_[line_idx] && cachedStyles_[line_idx]->valid.load(std::memory_order_acquire)) {
                result.push_back(cachedStyles_[line_idx]->styles);
            } else {
                result.push_back(std::vector<SyntaxStyle>()); // Empty styles for invalid/missing/out-of-bounds cache entry for this line_idx
            }
        }
        logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Returning %zu style sets for requested range [%zu, %zu].", result.size(), startLine, actualEndLine);
        return result;

    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::getHighlightingStyles (public)", "EditorException caught: %s. Returning empty shell.", ed_ex.what());
        return empty_result_shell;
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(std::string("getHighlightingStyles (public): ") + ex.what(), EditorException::Severity::Error));
        logManagerMessage(EditorException::Severity::Error, "SyntaxHighlightingManager::getHighlightingStyles (public)", "std::exception caught: %s. Returning empty shell.", ex.what());
        return empty_result_shell;
    } catch (...) {
        ErrorReporter::logUnknownException("SyntaxHighlightingManager::getHighlightingStyles (public)");
        logManagerMessage(EditorException::Severity::Critical, "SyntaxHighlightingManager::getHighlightingStyles (public)", "Unknown exception caught. Returning empty shell.");
        return empty_result_shell;
    }
}

void SyntaxHighlightingManager::invalidateLine(size_t line) {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::invalidateLine", "Called for line %zu.", line);
    std::scoped_lock lock(mutex_);
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::invalidateLine", "Mutex acquired for line %zu.", line);
    
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
    if (buffer->lineCount() > 0) { // Ensure not to underflow with lineCount() - 1 if lineCount is 0
      endLine = std::min(endLine, buffer->lineCount() - 1);
    } else {
      return; // No lines to invalidate if buffer is empty
    }
    
    if (startLine > endLine) return; // After clamping, if start is past end

    for (size_t line_idx = startLine; line_idx <= endLine; ++line_idx) {
        if (line_idx < cachedStyles_.size() && cachedStyles_[line_idx]) {
            // Mark the cache entry as invalid
            cachedStyles_[line_idx]->valid.store(false, std::memory_order_release);
        }
        invalidatedLines_.insert(line_idx);
        lineTimestamps_.erase(line_idx);
    }
}

void SyntaxHighlightingManager::invalidateAllLines() {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::invalidateAllLines", "Called.");
    std::scoped_lock lock(mutex_);
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::invalidateAllLines", "Mutex acquired.");
    invalidateAllLines_nolock();
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::invalidateAllLines", "All lines invalidated (nolock called).");
}

void SyntaxHighlightingManager::invalidateAllLines_nolock() {
    logManagerMessage(EditorException::Severity::Warning, "SyntaxHighlightingManager::invalidateAllLines_nolock", "Called.");
    const TextBuffer* buffer = getBuffer(); // Safe atomic load
    // No buffer check needed here, as operations below are safe with nullptr buffer
    
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
    
    // If buffer exists and has lines, mark them for re-highlight upon next request.
    if (buffer && buffer->lineCount() > 0) {
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

std::pair<size_t, size_t> SyntaxHighlightingManager::calculateEffectiveRange(
    size_t startLine, size_t endLine) const {
    const TextBuffer* buffer = getBuffer(); 
    if (!buffer) return {startLine, endLine};
    
    size_t contextL = contextLines_.load(std::memory_order_acquire);
    size_t lineCount = buffer->lineCount();
    if (lineCount == 0) return {0,0}; 
    
    size_t effectiveStart = startLine > contextL ? startLine - contextL : 0;
    size_t effectiveEnd = std::min(endLine + contextL, lineCount - 1);

    if (effectiveStart > effectiveEnd && lineCount > 0) effectiveStart = effectiveEnd; 

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
                cachedStyles_[line]->valid.store(false, std::memory_order_release);
            }
            it = lineTimestamps_.erase(it); 
            invalidatedLines_.insert(line); 
            removedCount++;
        } else {
            ++it;
        }
    }

    if (removedCount > 0) {
        logManagerMessage(EditorException::Severity::Warning, 
                          "SyntaxHighlightingManager::cleanupCache_nolock",
                          "Cache cleanup: %zu entries before, %zu removed, %zu timestamps remaining.",
                          initialCacheSize, removedCount, lineTimestamps_.size());
    }
} 