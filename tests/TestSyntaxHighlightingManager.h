#ifndef TEST_SYNTAX_HIGHLIGHTING_MANAGER_H
#define TEST_SYNTAX_HIGHLIGHTING_MANAGER_H

#include "SyntaxHighlighter.h"
#include "TextBuffer.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <iostream>

// Forward declarations
class TextBuffer;

/**
 * Simplified SyntaxHighlightingManager for testing purposes.
 * Removes excessive thread safety and logging from the production version.
 */
class TestSyntaxHighlightingManager {
public:
    TestSyntaxHighlightingManager() :
        buffer_(nullptr),
        highlighter_(nullptr),
        enabled_(false) {
    }
    
    ~TestSyntaxHighlightingManager() = default;
    
    // Set the active highlighter
    void setHighlighter(std::shared_ptr<SyntaxHighlighter> highlighter) {
        highlighter_ = highlighter;
        invalidateAllLines();
    }
    
    // Get the current highlighter
    std::shared_ptr<SyntaxHighlighter> getHighlighter() const {
        return highlighter_;
    }
    
    // Enable or disable syntax highlighting
    void setEnabled(bool enabled) {
        enabled_ = enabled;
    }
    
    // Check if syntax highlighting is enabled
    bool isEnabled() const {
        return enabled_;
    }
    
    // Set the buffer to highlight (non-owning pointer)
    void setBuffer(const TextBuffer* buffer) {
        buffer_ = buffer;
        invalidateAllLines();
    }
    
    // Get highlighting styles for a range of lines
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles(size_t startLine, size_t endLine) {
        if (!enabled_ || !buffer_ || !highlighter_ || buffer_->isEmpty()) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        if (startLine > endLine) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        size_t bufferLineCount = buffer_->lineCount();
        if (startLine >= bufferLineCount) {
            return std::vector<std::vector<SyntaxStyle>>();
        }
        
        // Clamp endLine to buffer size
        endLine = std::min(endLine, bufferLineCount - 1);
        
        std::vector<std::vector<SyntaxStyle>> styles;
        styles.reserve(endLine - startLine + 1);
        
        for (size_t line = startLine; line <= endLine; ++line) {
            try {
                const std::string& lineText = buffer_->getLine(line);
                styles.push_back(*(highlighter_->highlightLine(lineText, line)));
            } catch (...) {
                // If any exception occurs, add empty styles for this line
                styles.push_back(std::vector<SyntaxStyle>());
            }
        }
        
        return styles;
    }
    
    // Invalidate all lines
    void invalidateAllLines() {
        // In this simplified version, we don't need to do anything
        // as we don't maintain a cache
    }
    
    // Invalidate specific lines
    void invalidateLines([[maybe_unused]] size_t startLine, [[maybe_unused]] size_t endLine) {
        // In this simplified version, we don't need to do anything
        // as we don't maintain a cache
    }
    
    // Invalidate a single line
    void invalidateLine([[maybe_unused]] size_t line) {
        // In this simplified version, we don't need to do anything
        // as we don't maintain a cache
    }
    
    // Set visible range for highlighting prioritization (no-op in test version)
    void setVisibleRange([[maybe_unused]] size_t startLine, [[maybe_unused]] size_t endLine) const {
        // No operation needed in test version
    }
    
private:
    const TextBuffer* buffer_;
    std::shared_ptr<SyntaxHighlighter> highlighter_;
    bool enabled_;
};

#endif // TEST_SYNTAX_HIGHLIGHTING_MANAGER_H 