#include "SyntaxHighlighter.h"
#include "TextBuffer.h"

// Implementation of PatternBasedHighlighter::highlightBuffer
std::vector<std::vector<SyntaxStyle>> PatternBasedHighlighter::highlightBuffer(
    const TextBuffer& buffer) const {
    
    std::vector<std::vector<SyntaxStyle>> result;
    if (buffer.isEmpty()) {
        return result;
    }
    
    result.reserve(buffer.lineCount());
    
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        const std::string& line = buffer.getLine(i);
        result.push_back(this->highlightLine(line, i));
    }
    
    return result;
} 