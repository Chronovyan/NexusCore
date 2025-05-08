#include "SyntaxHighlighter.h"
#include "TextBuffer.h"

// Implementation of highlightBuffer
std::vector<std::vector<SyntaxStyle>> PatternBasedHighlighter::highlightBuffer(const TextBuffer& buffer) {
    std::vector<std::vector<SyntaxStyle>> bufferStyles;
    
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        const std::string& line = buffer.getLine(i);
        bufferStyles.push_back(highlightLine(line, i));
    }
    
    return bufferStyles;
} 