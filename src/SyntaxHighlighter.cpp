#include "SyntaxHighlighter.h"
#include "TextBuffer.h"
#include "interfaces/ITextBuffer.hpp"
#include <vector> // Required for std::vector
#include <string> // Required for std::string
#include <iostream> // For THREAD_DEBUG, remove if not used or defined elsewhere
#include <algorithm> // For std::sort and potentially std::min
#include <regex> // For std::regex and std::smatch
#include <memory> // Required for std::unique_ptr
#include "EditorError.h"

// Add static debug flag for SyntaxHighlighter
bool SyntaxHighlighter::debugLoggingEnabled_ = false;

// Helper function to log debug information through ErrorReporter
void SyntaxHighlighter::logDebug(const std::string& message) {
    if (debugLoggingEnabled_) {
        ErrorReporter::logWarning("Debug: " + message);
    }
}

void SyntaxHighlighter::setDebugLoggingEnabled(bool enabled) {
    debugLoggingEnabled_ = enabled;
}

bool SyntaxHighlighter::isDebugLoggingEnabled() {
    return debugLoggingEnabled_;
}

// Static helper function to trim trailing whitespace
static std::string trimTrailingWhitespace(const std::string& str) {
    const std::string whitespace = " \\t\\n\\r\\f\\v";
    size_t end = str.find_last_not_of(whitespace);
    if (std::string::npos == end) {
        return ""; // String is all whitespace
    }
    return str.substr(0, end + 1);
}

// Implementation of PatternBasedHighlighter::highlightBuffer
std::vector<std::vector<SyntaxStyle>> PatternBasedHighlighter::highlightBuffer(
    const ITextBuffer& buffer) const {
    
    std::vector<std::vector<SyntaxStyle>> result;
    
    try {
        if (buffer.isEmpty()) {
            return result;
        }
        
        result.reserve(buffer.lineCount());
        
        for (size_t i = 0; i < buffer.lineCount(); ++i) {
            try {
                const std::string& line = buffer.getLine(i);
                auto lineStylesPtr = this->highlightLine(line, i); // Returns std::unique_ptr
                if (lineStylesPtr) {
                    result.push_back(std::move(*lineStylesPtr));
                } else {
                    result.push_back({}); // Add empty styles if null ptr (shouldn't happen with make_unique)
                }
            } catch (const EditorException& ed_ex) {
                ErrorReporter::logException(ed_ex);
                result.push_back(std::vector<SyntaxStyle>()); // Add empty styles for this line
            } catch (const std::exception& ex) {
                ErrorReporter::logException(SyntaxHighlightingException(
                    std::string("PatternBasedHighlighter::highlightBuffer line ") + 
                    std::to_string(i) + ": " + ex.what(), 
                    EditorException::Severity::EDITOR_ERROR));
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
            EditorException::Severity::EDITOR_ERROR));
    } catch (...) {
        ErrorReporter::logUnknownException("PatternBasedHighlighter::highlightBuffer");
    }
    
    return result;
}

// --- CppHighlighter Method Definitions ---

// Static helper function to merge overlapping/redundant styles
static std::vector<SyntaxStyle> mergeStyles(std::vector<SyntaxStyle>& styles) {
    if (styles.empty()) {
        return {};
    }

    // Sort by start position, then by end position (descending for longer styles first),
    // then prioritize important token types (strings, comments)
    std::sort(styles.begin(), styles.end(), [](const SyntaxStyle& a, const SyntaxStyle& b) {
        if (a.startCol != b.startCol) {
            return a.startCol < b.startCol;
        }
        if (a.endCol != b.endCol) {
            return a.endCol > b.endCol; // Longer one first
        }
        
        // Higher-priority tokens win when spans are identical - strings and comments should win
        if (a.color == SyntaxColor::String && b.color != SyntaxColor::String) {
            return true; // String takes precedence
        }
        if (a.color != SyntaxColor::String && b.color == SyntaxColor::String) {
            return false; // String takes precedence
        }
        if (a.color == SyntaxColor::Comment && b.color != SyntaxColor::Comment) {
            return true; // Comment takes precedence
        }
        if (a.color != SyntaxColor::Comment && b.color == SyntaxColor::Comment) {
            return false; // Comment takes precedence
        }
        
        // For other token types, use color as a tie-breaker
        return static_cast<int>(a.color) < static_cast<int>(b.color);
    });

    std::vector<SyntaxStyle> merged;
    merged.push_back(styles[0]);

    for (size_t i = 1; i < styles.size(); ++i) {
        SyntaxStyle& lastMerged = merged.back();
        const SyntaxStyle& current = styles[i];

        // Check if we're going to add a high-priority type (string/comment)
        bool isHighPriorityType = (current.color == SyntaxColor::String || current.color == SyntaxColor::Comment);
        
        // If current is completely contained within lastMerged, skip unless it's a high-priority type
        if (current.startCol >= lastMerged.startCol && current.endCol <= lastMerged.endCol) {
            // If current is a high-priority type, replace the overlapping portion of lastMerged
            if (isHighPriorityType && 
                lastMerged.color != SyntaxColor::String && 
                lastMerged.color != SyntaxColor::Comment) {
                
                // Special case: completely replace if same span
                if (current.startCol == lastMerged.startCol && current.endCol == lastMerged.endCol) {
                    merged.back() = current;
                }
                // Otherwise, potentially split lastMerged
                else {
                    // This is a more complex case where we would need to split lastMerged
                    // This implementation is simplified for now - just add the high-priority segment
                    merged.push_back(current);
                }
            }
            // Otherwise skip if fully contained (default behavior)
            continue; 
        }
        // If current starts before lastMerged ends (overlap) but is not fully contained
        else if (current.startCol < lastMerged.endCol) {
            // If current is high priority and last merged is not, prioritize current
            if (isHighPriorityType && 
                lastMerged.color != SyntaxColor::String && 
                lastMerged.color != SyntaxColor::Comment) {
                // For simplicity in this implementation, just add it - the rendering should prioritize
                // string/comment if there's overlap
                merged.push_back(current);
            }
            // If current extends past lastMerged, add it regardless of priority
            else if (current.endCol > lastMerged.endCol) {
                merged.push_back(current);
            }
        } else { // No overlap with lastMerged
            merged.push_back(current);
        }
    }
    return merged;
}

// Helper to find the end of a string literal, handling escapes
// Returns position AFTER the closing quote, or line.length() if unterminated
static size_t findStringEnd(const std::string& line, size_t startPos) {
    size_t current = startPos;
    while (current < line.length()) {
        if (line[current] == '\\') { // Escape character
            current++; // Skip the escape
            if (current < line.length()) {
                current++; // Skip the escaped character
            }
        } else if (line[current] == '"') { // Closing quote
            return current + 1; // Position after closing quote
        } else {
            current++;
        }
    }
    return line.length(); // Unterminated on this line
}

// Helper to find the end of a char literal, handling escapes
// Returns position AFTER the closing quote, or line.length() if unterminated
// Simple version, assumes basic escapes like '\\n', '\\t', '\\\\', '\'\''
static size_t findCharEnd(const std::string& line, size_t startPos) {
    size_t current = startPos; // Position *after* opening '
    if (startPos == 0 || startPos > line.length()) return line.length(); // Invalid start

    // Expecting char content then closing quote
    if (current < line.length()) { // Potentially first char of content or escape
        if (line[current] == '\\') { // Escape character
            current++; // Skip the escape char itself
            if (current < line.length()) {
                current++; // Skip the escaped character
            }
        } else {
             current++; // Non-escaped character
        }
    }
    // After potential content/escape, look for closing quote
    if (current < line.length() && line[current] == '\'') { // Closing quote
        return current + 1; // Position after closing quote
    }
    // If no closing quote, it's unterminated (or malformed, e.g. empty '')
    // For '' (empty char literal), current would be startPos here if findCharEnd was called with startPos pointing to the 2nd quote.
    // If called with startPos *after* 1st quote, and line is just '', current points to 2nd quote.
    // If line is just "'", startPos is 1 (after quote), current becomes 1 (at EOL), returns line.length(). Correct.
    return line.length(); 
}

void CppHighlighter::findNextStatefulToken(const std::string& segment, size_t& outNextTokenPos, NextTokenType& outTokenType) const {
    outNextTokenPos = std::string::npos;
    outTokenType = NextTokenType::UNKNOWN;

    // First find the raw positions of all potential tokens
    size_t lineCommentPos = segment.find("//");
    size_t blockCommentStartPos = segment.find("/*");
    size_t stringPos = segment.find("\""); // Standard string
    size_t charPos = segment.find("'");

    // Regex for raw string prefix: R"(delimiter?(
    // Delimiter can be 0-16 chars, not containing '(', ')', '\\', or whitespace.
    // std::regex rawStringRegex("R\\\"\\("); // SIMPLIFIED REGEX for R"(
    std::regex rawStringRegex(R"#(R"([^\s()\\]{0,16})\()#"); // Full regex using C++ Raw String Literal
    std::smatch rawStringMatch;
    size_t rawStringStartPos = std::string::npos;
    if (std::regex_search(segment, rawStringMatch, rawStringRegex)) {
        rawStringStartPos = rawStringMatch.position(0);
    }

    // Next, determine the first occurring token with proper nesting behavior
    
    // First, find the actual first token position without considering nesting
    size_t firstTokenPos = std::string::npos;
    NextTokenType firstTokenType = NextTokenType::UNKNOWN;

    auto updateFirstToken = [&](size_t pos, NextTokenType type) {
        if (pos != std::string::npos && (firstTokenPos == std::string::npos || pos < firstTokenPos)) {
            firstTokenPos = pos;
            firstTokenType = type;
        }
    };

    updateFirstToken(stringPos, NextTokenType::STRING);
    updateFirstToken(charPos, NextTokenType::CHAR);
    updateFirstToken(rawStringStartPos, NextTokenType::RAW_STRING);
    updateFirstToken(lineCommentPos, NextTokenType::LINE_COMMENT);
    updateFirstToken(blockCommentStartPos, NextTokenType::BLOCK_COMMENT);

    // Now apply proper nesting rules:
    
    // If a string is the first token, then comments inside it should not be recognized as tokens
    // This ensures comments in strings don't get highlighted as comments
    if (firstTokenType == NextTokenType::STRING) {
        // Locate the end of this string
        size_t stringEndPos = findStringEnd(segment, stringPos + 1);
        
        // Check if a comment starts after the string
        size_t nextLineCommentPos = (lineCommentPos != std::string::npos && lineCommentPos > stringEndPos) ? 
                                    lineCommentPos : std::string::npos;
        size_t nextBlockCommentPos = (blockCommentStartPos != std::string::npos && blockCommentStartPos > stringEndPos) ? 
                                    blockCommentStartPos : std::string::npos;
        
        // If there's a comment after the string, consider it as the next token
        if (nextLineCommentPos != std::string::npos || nextBlockCommentPos != std::string::npos) {
            if ((nextLineCommentPos != std::string::npos && 
                (nextBlockCommentPos == std::string::npos || nextLineCommentPos < nextBlockCommentPos))) {
                outNextTokenPos = stringPos;  // Return the string position first
                outTokenType = NextTokenType::STRING;
            } else {
                outNextTokenPos = stringPos;  // Return the string position first
                outTokenType = NextTokenType::STRING;
            }
        } else {
            // No comments after this string, return the string position
            outNextTokenPos = stringPos;
            outTokenType = NextTokenType::STRING;
        }
    }
    // Similarly for char literals
    else if (firstTokenType == NextTokenType::CHAR) {
        outNextTokenPos = charPos;
        outTokenType = NextTokenType::CHAR;
    }
    // And raw strings
    else if (firstTokenType == NextTokenType::RAW_STRING) {
        outNextTokenPos = rawStringStartPos;
        outTokenType = NextTokenType::RAW_STRING;
    }
    // If comment appears first, then this token takes precedence
    else if (firstTokenType == NextTokenType::LINE_COMMENT) {
        outNextTokenPos = lineCommentPos;
        outTokenType = NextTokenType::LINE_COMMENT;
    }
    else if (firstTokenType == NextTokenType::BLOCK_COMMENT) {
        outNextTokenPos = blockCommentStartPos;
        outTokenType = NextTokenType::BLOCK_COMMENT;
    }
    // Default case: No tokens found
    else {
        outNextTokenPos = std::string::npos;
        outTokenType = NextTokenType::UNKNOWN;
    }
}

void CppHighlighter::appendBaseStyles(std::vector<SyntaxStyle>& existingStyles, const std::string& subLine, size_t offset) const {
    logDebug("CppHL::appendBaseStyles - Segment: '" + subLine + "', Offset: " + std::to_string(offset));
    
    if (subLine.empty()) {
        logDebug("CppHL::appendBaseStyles - Segment empty, returning.");
        return;
    }
    
    // Get styles from PatternBasedHighlighter (regexes for keywords, types, etc.)
    auto segmentStylesPtr = PatternBasedHighlighter::highlightLine(subLine, 0); 
    std::vector<SyntaxStyle> segmentStyles;
    if (segmentStylesPtr) {
        segmentStyles = std::move(*segmentStylesPtr);
    }

    std::string debugOutput = "CppHL::appendBaseStyles - Got " + std::to_string(segmentStyles.size()) + 
                             " styles from PatternBasedHighlighter for segment '" + subLine + "'";
    logDebug(debugOutput);
    
    for (const auto& s_style : segmentStyles) {
        std::string styleInfo = "  Raw Style: (" + std::to_string(s_style.startCol) + "," + 
                              std::to_string(s_style.endCol) + ") Color: " + 
                              std::to_string(static_cast<int>(s_style.color));
        logDebug(styleInfo);
    }

    for (const auto& s_style : segmentStyles) {
        SyntaxStyle newStyle(s_style.startCol + offset, s_style.endCol + offset, s_style.color);
        
        // Avoid adding a base style if a stateful style (Comment, String) already covers this exact range.
        // This is a simple check; a more robust solution would involve checking for any overlap
        // and giving precedence to stateful styles.
        bool overlapsWithStateful = false;
        for(const auto& existing_s : existingStyles) {
            if (existing_s.startCol == newStyle.startCol && existing_s.endCol == newStyle.endCol &&
                (existing_s.color == SyntaxColor::Comment || existing_s.color == SyntaxColor::String)) {
                overlapsWithStateful = true;
                break;
            }
            // More complex overlap: if newStyle is entirely within an existing String/Comment
            if (newStyle.startCol >= existing_s.startCol && newStyle.endCol <= existing_s.endCol &&
                 (existing_s.color == SyntaxColor::Comment || existing_s.color == SyntaxColor::String)) {
                overlapsWithStateful = true;
                break;
            }
        }

        if (!overlapsWithStateful) {
            existingStyles.push_back(newStyle);
        }
    }
}

std::unique_ptr<std::vector<SyntaxStyle>> CppHighlighter::highlightLine(const std::string& line, size_t lineIndex) const {
    // Debug output to understand the state
    logDebug("CppHighlighter::highlightLine - Line: '" + line + "', Index: " + std::to_string(lineIndex) + ", isInBlockComment: " + (isInBlockComment_ ? "true" : "false"));

    // Check for line ending with backslash - needs to happen before other processing
    std::string trimmedLine = trimTrailingWhitespace(line);
    bool lineEndsWithBackslash = !trimmedLine.empty() && trimmedLine.back() == '\\';
    
    // Special handling for macro continuations
    // If the previous line ended with a backslash and this line is the next sequential line,
    // we treat this line as part of the macro continuation
    if (isInMacroContinuation_ && lineIndex > 0 && lastProcessedLineIndex_ == lineIndex - 1) {
        // Set the "last processed line" to the current line
        lastProcessedLineIndex_ = lineIndex;
        
        // Check if this line also continues the macro
        isInMacroContinuation_ = lineEndsWithBackslash;
        
        // Instead of returning an empty vector, process the line normally but track that we're in a continuation
        // We want syntax highlighting to work on continuation lines
    }
    
    // Reset the statement block states for non-continuation lines
    if (lineIndex == 0 || lastProcessedLineIndex_ != lineIndex - 1) {
        isInBlockComment_ = false;
        isInString_ = false;
        isInChar_ = false;
        isInRawString_ = false;
        rawStringDelimiter_.clear();
        isInMacroContinuation_ = false;
    }
    
    // Track that we've processed this line
    lastProcessedLineIndex_ = lineIndex;
    
    std::vector<SyntaxStyle> styles;
    size_t currentPos = 0;

    // If the line ended mid-raw-string, the next line continues that raw string.
    // (Raw string logic comes before other stateful tokens like comments)
    if (isInRawString_ && lineIndex > 0 && lastProcessedLineIndex_ == lineIndex - 1) {
        isInRawString_ = false; // Raw string processing ends with this line
    }

    // Explicitly clear 'styles' before checking for macro continuation, as a desperate measure.
    styles.clear(); 

    // Check if this is the first line of a preprocessor directive
    // bool isPreprocessorDirective = false;  // Remove this line if it's not used or defined elsewhere
    if (!isInMacroContinuation_ && line.length() > 0) {
        // Find the first non-whitespace character
        size_t nonWhitespacePos = line.find_first_not_of(" \t");
        if (nonWhitespacePos != std::string::npos && line[nonWhitespacePos] == '#') {
            // isPreprocessorDirective = true;
            // Add style for the preprocessor directive
            size_t directiveEnd = line.find_first_of(" \t", nonWhitespacePos + 1);
            if (directiveEnd == std::string::npos) directiveEnd = line.length();
            styles.push_back(SyntaxStyle(nonWhitespacePos, directiveEnd, SyntaxColor::Preprocessor));
            
            // Check if line ends with backslash to start continuation
            if (lineEndsWithBackslash) {
                isInMacroContinuation_ = true;
            }
        }
    }

    while (currentPos < line.length()) {
        size_t segmentStartPos = currentPos; // Start of the segment we might pass to appendBaseStyles

        if (isInBlockComment_) {
            size_t commentEndPos = line.find("*/", currentPos);
            if (commentEndPos != std::string::npos) {
                styles.push_back(SyntaxStyle(currentPos, commentEndPos + 2, SyntaxColor::Comment));
                currentPos = commentEndPos + 2;
                isInBlockComment_ = false;
            } else {
                styles.push_back(SyntaxStyle(currentPos, line.length(), SyntaxColor::Comment));
                currentPos = line.length();
                // isInBlockComment_ remains true
            }
            continue; // Styled this segment as comment, restart loop
        } else if (isInRawString_) {
            std::string terminator = ")" + rawStringDelimiter_ + "\"";
            size_t rawStringEndPos = line.find(terminator, currentPos);
            if (rawStringEndPos != std::string::npos) {
                styles.push_back(SyntaxStyle(currentPos, rawStringEndPos + terminator.length(), SyntaxColor::String));
                currentPos = rawStringEndPos + terminator.length();
                isInRawString_ = false;
                rawStringDelimiter_.clear();
            } else {
                styles.push_back(SyntaxStyle(currentPos, line.length(), SyntaxColor::String));
                currentPos = line.length();
                // isInRawString_ and rawStringDelimiter_ remain for next line
            }
            continue; // Styled this segment as raw string, restart loop
        } else if (isInString_) {
            size_t stringEndPos = findStringEnd(line, currentPos); // findStringEnd expects pos *after* opening quote
            // For a continued string, currentPos is 0. The original opening quote was on a previous line.
            // We need to find the end from currentPos.
            size_t actualStringEndPos = stringEndPos; // findStringEnd returns pos AFTER closing quote or line.length()
            
            styles.push_back(SyntaxStyle(currentPos, actualStringEndPos, SyntaxColor::String));
            currentPos = actualStringEndPos;
            if (actualStringEndPos < line.length() || (actualStringEndPos == line.length() && line.back() == '"' && (line.length() < 2 || line[line.length()-2] != '\\'))) { // Properly terminated
                 isInString_ = false;
            } // else: isInString_ remains true for next line
            continue;
        } else if (isInChar_) {
            // Similar logic for continued char, though less common.
            // Assuming a char literal can't legitimately span lines for highlighting simplicity.
            // If isInChar_ is true, it implies an unterminated char from previous line. Style as error/default.
            // For this iteration, we'll assume if isInChar_ is true, it's an error from previous line.
            // The first char of this line is part of that unterminated char.
            // This state should ideally be cleared if line starts fresh.
            // The current resetStateForNewLine should handle this.
            // If still in isInChar_ it means it was unterminated on the *previous* line.
            // We'll treat the start of this line as default and reset isInChar.
            // This simplification might need review for very complex char literal error handling.
            appendBaseStyles(styles, line.substr(currentPos, 1), currentPos); // Style first char as default
            currentPos++;
            isInChar_ = false; // Assume error state ends here for this line
            continue;
        }

        // Not in a multi-line token. Look for the next stateful token.
        size_t nextTokenPos = std::string::npos;
        NextTokenType tokenType = NextTokenType::UNKNOWN;
        findNextStatefulToken(line.substr(currentPos), nextTokenPos, tokenType);

        if (nextTokenPos != std::string::npos) { // A stateful token starts on this line
            nextTokenPos += currentPos; // Adjust to be relative to full line

            // Style the segment before this token using PatternBasedHighlighter
            if (nextTokenPos > segmentStartPos) {
                appendBaseStyles(styles, line.substr(segmentStartPos, nextTokenPos - segmentStartPos), segmentStartPos);
            }
            currentPos = nextTokenPos; // Advance to the start of the stateful token

            // Now handle the stateful token itself
            if (tokenType == NextTokenType::BLOCK_COMMENT) {
                size_t commentEndPos = line.find("*/", currentPos);
                if (commentEndPos != std::string::npos) {
                    styles.push_back(SyntaxStyle(currentPos, commentEndPos + 2, SyntaxColor::Comment));
                    currentPos = commentEndPos + 2;
                    // isInBlockComment_ remains false
                } else {
                    styles.push_back(SyntaxStyle(currentPos, line.length(), SyntaxColor::Comment));
                    currentPos = line.length();
                    isInBlockComment_ = true;
                }
            } else if (tokenType == NextTokenType::LINE_COMMENT) {
                styles.push_back(SyntaxStyle(currentPos, line.length(), SyntaxColor::Comment));
                currentPos = line.length();
            } else if (tokenType == NextTokenType::RAW_STRING) {
                // New raw string found
                std::regex rawStringRegex(R"#(R"([^\s()\\]{0,16})\()#"); // Full regex using C++ Raw String Literal
                std::smatch match;
                std::string segmentForRegex = line.substr(currentPos);

                if (std::regex_search(segmentForRegex, match, rawStringRegex) && match.position(0) == 0) {
                    rawStringDelimiter_ = match.str(1);
                    isInRawString_ = true;
                    size_t prefixLength = match.length(0);
                    styles.push_back(SyntaxStyle(currentPos, currentPos + prefixLength, SyntaxColor::String));
                    currentPos += prefixLength;

                    // Now look for the terminator on the same line
                    std::string terminator = ")" + rawStringDelimiter_ + "\"";
                    size_t rawStringEndPosInRemainder = line.substr(currentPos).find(terminator);

                    if (rawStringEndPosInRemainder != std::string::npos) {
                        // Terminator found on the same line
                        styles.push_back(SyntaxStyle(currentPos, currentPos + rawStringEndPosInRemainder + terminator.length(), SyntaxColor::String));
                        currentPos += rawStringEndPosInRemainder + terminator.length();
                        isInRawString_ = false;
                        rawStringDelimiter_.clear();
                    } else {
                        // Unterminated on this line
                        styles.push_back(SyntaxStyle(currentPos, line.length(), SyntaxColor::String));
                        currentPos = line.length();
                        // isInRawString_ remains true, rawStringDelimiter_ is set
                    }
                } else {
                    // This case should ideally not be hit if findNextStatefulToken worked correctly
                    // and we are indeed at a raw string start. Add a fallback or error.
                    // For now, just style the R" part if it looks like it, to avoid losing it.
                    if (line.substr(currentPos, 2) == "R\"") { // Basic R"
                         styles.push_back(SyntaxStyle(currentPos, currentPos + 2, SyntaxColor::Keyword)); // Or String?
                         currentPos += 2;
                    } else { // Should not happen, advance by 1 to avoid infinite loop
                         styles.push_back(SyntaxStyle(currentPos, currentPos + 1, SyntaxColor::Default));
                         currentPos += 1;
                    }
                }
                continue;
            } else if (tokenType == NextTokenType::STRING) {
                size_t actualOpeningQuotePos = currentPos; // currentPos is at the opening quote
                size_t stringEndPos = findStringEnd(line, actualOpeningQuotePos + 1); // Pass pos *after* opening quote

                styles.push_back(SyntaxStyle(actualOpeningQuotePos, stringEndPos, SyntaxColor::String));
                
                currentPos = stringEndPos;
                if (stringEndPos == line.length() && (line.empty() || line.back() != '"' || (line.length() >= 2 && line[line.length()-2] == '\\'))) { // Unterminated
                    isInString_ = true;
                } else {
                    isInString_ = false; // Terminated on this line
                }
            } else if (tokenType == NextTokenType::CHAR) {
                size_t actualOpeningQuotePos = currentPos;
                size_t charEndPos = findCharEnd(line, actualOpeningQuotePos + 1); // Pass pos *after* opening quote

                styles.push_back(SyntaxStyle(actualOpeningQuotePos, charEndPos, SyntaxColor::String)); // Treat as string
                currentPos = charEndPos;

                if (charEndPos == line.length() && (line.empty() || line.back() != '\'' || (line.length() >=2 && line[line.length()-2] == '\\'))) { // Unterminated
                    isInChar_ = true;
                } else if (charEndPos == actualOpeningQuotePos + 1) { // Empty char '' is invalid, but findCharEnd might return this
                    // This case could be styled as error or just let appendBaseStyles handle it if we advance
                    // For now, styled as string of length 1 (just the quote) by above logic, currentPos advanced.
                    // This implies it's handled.
                     isInChar_ = false;
                }
                 else {
                    isInChar_ = false; // Terminated
                }
            }
        } else { // No more stateful tokens on this line
            if (segmentStartPos < line.length()) {
                appendBaseStyles(styles, line.substr(segmentStartPos), segmentStartPos);
            }
            currentPos = line.length(); // Done with the line
        }
    } // End while (currentPos < line.length())

    // After processing the line with stateful tokens and pattern-based styles,
    // determine if THIS line itself ends with a backslash (and is not a comment/string ending)
    // to set up macro continuation for the NEXT line.
    if (!isInBlockComment_ && !isInRawString_ && !isInString_ && !isInChar_) {
        isInMacroContinuation_ = lineEndsWithBackslash;
    }
    
    // Ensure styles are sorted for merging and correct rendering order
    auto mergedStyles = mergeStyles(styles);

    return std::make_unique<std::vector<SyntaxStyle>>(mergedStyles);
}

std::vector<std::vector<SyntaxStyle>> CppHighlighter::highlightBuffer(const ITextBuffer& buffer) const {
    std::vector<std::vector<SyntaxStyle>> result;
    if (buffer.isEmpty()) {
        return result;
    }
    result.reserve(buffer.lineCount());
    
    // Cannot copy the highlighter due to mutex, so create a new instance
    // Initialize the mutable state variables since this is a const method
    mutable_reset(); // Reset all the mutable state for buffer processing
    
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::string lineContent = buffer.getLine(i);
        auto lineStylesPtr = this->highlightLine(lineContent, i);
        
        if (lineStylesPtr) {
            logDebug("For line " + std::to_string(i) + " ('" + lineContent.substr(0,40) + (lineContent.length() > 40 ? "..." : "") + "'), received " + std::to_string(lineStylesPtr->size()) + " styles.");
            if ((i == 1 || i == 2)) { // Lines of interest for MultiLinePreprocessorDirectives
                if (lineStylesPtr->empty()) {
                    logDebug("Line " + std::to_string(i) + " received styles are indeed EMPTY.");
                } else {
                    logDebug("Line " + std::to_string(i) + " received styles are NOT EMPTY. First style: (" + std::to_string((*lineStylesPtr)[0].startCol) + "," + std::to_string((*lineStylesPtr)[0].endCol) + ") Color: " + std::to_string(static_cast<int>((*lineStylesPtr)[0].color)));
                }
            }
            result.push_back(std::move(*lineStylesPtr));
        } else {
            logDebug("For line " + std::to_string(i) + " ('" + lineContent.substr(0,40) + (lineContent.length() > 40 ? "..." : "") + "'), received nullptr.");
            result.push_back({}); // Add empty styles if null ptr
        }
    }
    return result;
}

// Add a helper method to reset the mutable state
void CppHighlighter::mutable_reset() const {
    isInBlockComment_ = false;
    isInString_ = false; 
    isInChar_ = false;   
    isInRawString_ = false;
    rawStringDelimiter_.clear();
    lastProcessedLineIndex_ = static_cast<size_t>(-1); 
    isInMacroContinuation_ = false; // Reset for buffer processing
} 