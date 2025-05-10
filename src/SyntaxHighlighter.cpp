#include "SyntaxHighlighter.h"
#include "TextBuffer.h"

// Implementation of PatternBasedHighlighter::highlightBuffer
std::vector<std::vector<SyntaxStyle>> PatternBasedHighlighter::highlightBuffer(
    const TextBuffer& buffer) const {
    
    std::vector<std::vector<SyntaxStyle>> result;
    
    try {
        if (buffer.isEmpty()) {
            return result;
        }
        
        result.reserve(buffer.lineCount());
        
        for (size_t i = 0; i < buffer.lineCount(); ++i) {
            try {
                const std::string& line = buffer.getLine(i);
                auto lineStyles = this->highlightLine(line, i);
                result.push_back(std::move(lineStyles));
            } catch (const EditorException& ed_ex) {
                ErrorReporter::logException(ed_ex);
                result.push_back(std::vector<SyntaxStyle>()); // Add empty styles for this line
            } catch (const std::exception& ex) {
                ErrorReporter::logException(SyntaxHighlightingException(
                    std::string("PatternBasedHighlighter::highlightBuffer line ") + 
                    std::to_string(i) + ": " + ex.what(), 
                    EditorException::Severity::Error));
                result.push_back(std::vector<SyntaxStyle>()); // Add empty styles for this line
            } catch (...) {
                ErrorReporter::logUnknownException(
                    std::string("PatternBasedHighlighter::highlightBuffer line ") + 
                    std::to_string(i));
                result.push_back(std::vector<SyntaxStyle>()); // Add empty styles for this line
            }
        }
    } catch (const EditorException& ed_ex) {
        ErrorReporter::logException(ed_ex);
    } catch (const std::exception& ex) {
        ErrorReporter::logException(SyntaxHighlightingException(
            std::string("PatternBasedHighlighter::highlightBuffer: ") + ex.what(), 
            EditorException::Severity::Error));
    } catch (...) {
        ErrorReporter::logUnknownException("PatternBasedHighlighter::highlightBuffer");
    }
    
    return result;
} 