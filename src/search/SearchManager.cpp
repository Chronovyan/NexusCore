#include "SearchManager.h"
#include "../core/Document.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cstring>

namespace ai_editor {

SearchManager::SearchManager(Document* document) 
    : document_(document) {
    // Initialize with default options
    options_ = Options();
}

SearchResult SearchManager::findNext(const std::string& searchTerm, int startLine, int startColumn) {
    if (!document_ || searchTerm.empty()) {
        return SearchResult();
    }
    
    addToSearchHistory(searchTerm);
    
    // Convert to 0-based indexing
    int currentLine = std::max(0, startLine - 1);
    int currentCol = std::max(0, startColumn - 1);
    
    const int lineCount = static_cast<int>(document_->getLineCount());
    
    // Determine search direction
    const int lineStep = options_.searchUp ? -1 : 1;
    
    // Start searching from the current position
    for (int i = 0; i < lineCount; ++i) {
        int line = (currentLine + i * lineStep + lineCount) % lineCount;
        const auto& lineText = document_->getLine(static_cast<size_t>(line));
        
        size_t startPos = (line == currentLine) ? static_cast<size_t>(currentCol) : 0;
        
        // If we're searching up, we need to search from the end of the previous line
        if (options_.searchUp && line != currentLine) {
            startPos = lineText.length();
        }
        
        size_t foundPos = std::string::npos;
        size_t matchLength = 0;
        
        if (options_.useRegex) {
            // Handle regex search
            try {
                std::regex_constants::syntax_option_type flags = 
                    std::regex_constants::ECMAScript | std::regex_constants::optimize;
                if (!options_.matchCase) {
                    flags |= std::regex_constants::icase;
                }
                
                // Compile the regex
                std::regex regex(searchTerm, flags);
                std::smatch match;
                
                // Create a string_view of the line to search
                std::string_view lineView(lineText);
                
                // If searching forward
                if (!options_.searchUp) {
                    std::string subStr = lineView.substr(startPos).data();
                    if (std::regex_search(subStr, match, regex, 
                                         std::regex_constants::match_continuous)) {
                        foundPos = startPos + match.position();
                        matchLength = match.length();
                    }
                } 
                // If searching backward
                else {
                    std::string subStr = lineView.substr(0, startPos).data();
                    std::sregex_iterator it(subStr.begin(), subStr.end(), regex);
                    std::sregex_iterator end;
                    
                    size_t lastPos = 0;
                    while (it != end) {
                        lastPos = it->position();
                        matchLength = it->length();
                        ++it;
                    }
                    
                    if (lastPos + matchLength <= startPos) {
                        foundPos = lastPos;
                    }
                }
            } catch (const std::regex_error&) {
                // Invalid regex pattern
                return SearchResult();
            }
        } else {
            // Simple text search
            if (!options_.searchUp) {
                // Search forward
                if (options_.matchCase) {
                    foundPos = lineText.find(searchTerm, startPos);
                } else {
                    std::string lineLower = lineText;
                    std::string termLower = searchTerm;
                    std::transform(lineLower.begin(), lineLower.end(), lineLower.begin(), ::tolower);
                    std::transform(termLower.begin(), termLower.end(), termLower.begin(), ::tolower);
                    foundPos = lineLower.find(termLower, startPos);
                }
                
                if (foundPos != std::string::npos) {
                    matchLength = searchTerm.length();
                    
                    // Check for whole word if needed
                    if (options_.matchWholeWord) {
                        if (!isWordBoundary(lineText[foundPos + matchLength]) || 
                            (foundPos > 0 && !isWordBoundary(lineText[foundPos - 1]))) {
                            // Not a whole word match, continue searching
                            foundPos = std::string::npos;
                        }
                    }
                }
            } else {
                // Search backward
                size_t searchEnd = (line == currentLine) ? startPos : lineText.length();
                
                if (options_.matchCase) {
                    size_t pos = lineText.rfind(searchTerm, searchEnd);
                    if (pos != std::string::npos && (pos + searchTerm.length() <= searchEnd)) {
                        foundPos = pos;
                        matchLength = searchTerm.length();
                    }
                } else {
                    std::string lineLower = lineText;
                    std::string termLower = searchTerm;
                    std::transform(lineLower.begin(), lineLower.end(), lineLower.begin(), ::tolower);
                    std::transform(termLower.begin(), termLower.end(), termLower.begin(), ::tolower);
                    
                    size_t pos = lineLower.rfind(termLower, searchEnd);
                    if (pos != std::string::npos && (pos + termLower.length() <= searchEnd)) {
                        foundPos = pos;
                        matchLength = termLower.length();
                    }
                }
                
                // Check for whole word if needed
                if (foundPos != std::string::npos && options_.matchWholeWord) {
                    if (!isWordBoundary(lineText[foundPos + matchLength]) || 
                        (foundPos > 0 && !isWordBoundary(lineText[foundPos - 1]))) {
                        // Not a whole word match, continue searching
                        foundPos = std::string::npos;
                    }
                }
            }
        }
        
        if (foundPos != std::string::npos) {
            SearchResult result;
            result.line = line + 1; // Convert to 1-based
            result.column = static_cast<int>(foundPos) + 1; // Convert to 1-based
            result.length = static_cast<int>(matchLength);
            result.lineText = lineText;
            result.matchText = lineText.substr(foundPos, matchLength);
            result.byteOffset = 0; // This would need to be calculated based on the document
            result.matchByteLength = matchLength; // Simplified for now
            
            return result;
        }
        
        // If we've wrapped around and didn't find anything, stop
        if (i > 0 && ((!options_.searchUp && line == currentLine) || 
                      (options_.searchUp && line == currentLine))) {
            break;
        }
        
        // If we're not wrapping and we've reached the end, stop
        if (!options_.wrapAround && 
            ((!options_.searchUp && line == lineCount - 1) || 
             (options_.searchUp && line == 0))) {
            break;
        }
    }
    
    return SearchResult();
}

SearchResult SearchManager::findPrevious(const std::string& searchTerm, int startLine, int startColumn) {
    // Save the current search direction
    bool wasSearchingUp = options_.searchUp;
    options_.searchUp = true;
    
    // Perform the search
    auto result = findNext(searchTerm, startLine, startColumn);
    
    // Restore the search direction
    options_.searchUp = wasSearchingUp;
    
    return result;
}

std::vector<SearchResult> SearchManager::findAll(const std::string& searchTerm) {
    if (!document_ || searchTerm.empty()) {
        return {};
    }
    
    addToSearchHistory(searchTerm);
    
    // Delegate to the internal search implementation
    return searchInDocument(searchTerm, document_);
}

SearchResult SearchManager::replaceNext(const std::string& searchTerm, const std::string& replaceText, 
                                      int startLine, int startColumn) {
    if (!document_ || searchTerm.empty()) {
        return SearchResult();
    }
    
    addToSearchHistory(searchTerm);
    addToReplaceHistory(replaceText);
    
    // Find the next occurrence
    auto result = findNext(searchTerm, startLine, startColumn);
    
    if (result.line > 0) {
        // Replace the text
        std::string newText = performReplacement(result.matchText, searchTerm, replaceText);
        
        // Update the document
        document_->replaceText(
            result.line - 1, result.column - 1, // Convert to 0-based
            result.line - 1, result.column - 1 + result.length,
            newText
        );
        
        // Update the result with the new text
        result.matchText = newText;
        result.length = static_cast<int>(newText.length());
    }
    
    return result;
}

int SearchManager::replaceAll(const std::string& searchTerm, const std::string& replaceText) {
    if (!document_ || searchTerm.empty()) {
        return 0;
    }
    
    addToSearchHistory(searchTerm);
    addToReplaceHistory(replaceText);
    
    // Find all occurrences
    auto results = searchInDocument(searchTerm, document_);
    
    if (results.empty()) {
        return 0;
    }
    
    // Replace from bottom to top to avoid invalidating positions
    int replaceCount = 0;
    
    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        const auto& result = *it;
        
        // Skip if this result is part of a previous replacement
        if (result.line < 0) {
            continue;
        }
        
        // Replace the text
        std::string newText = performReplacement(result.matchText, searchTerm, replaceText);
        
        // Update the document
        document_->replaceText(
            result.line - 1, result.column - 1, // Convert to 0-based
            result.line - 1, result.column - 1 + result.length,
            newText
        );
        
        replaceCount++;
    }
    
    return replaceCount;
}

int SearchManager::countAll(const std::string& searchTerm) {
    if (!document_ || searchTerm.empty()) {
        return 0;
    }
    
    auto results = searchInDocument(searchTerm, document_);
    return static_cast<int>(results.size());
}

void SearchManager::addToSearchHistory(const std::string& term) {
    if (term.empty()) {
        return;
    }
    
    // Remove if already exists
    auto it = std::find(searchHistory_.begin(), searchHistory_.end(), term);
    if (it != searchHistory_.end()) {
        searchHistory_.erase(it);
    }
    
    // Add to front
    searchHistory_.insert(searchHistory_.begin(), term);
    
    // Trim history if needed
    if (searchHistory_.size() > maxHistorySize_) {
        searchHistory_.resize(maxHistorySize_);
    }
}

void SearchManager::addToReplaceHistory(const std::string& term) {
    if (term.empty()) {
        return;
    }
    
    // Remove if already exists
    auto it = std::find(replaceHistory_.begin(), replaceHistory_.end(), term);
    if (it != replaceHistory_.end()) {
        replaceHistory_.erase(it);
    }
    
    // Add to front
    replaceHistory_.insert(replaceHistory_.begin(), term);
    
    // Trim history if needed
    if (replaceHistory_.size() > maxHistorySize_) {
        replaceHistory_.resize(maxHistorySize_);
    }
}

// Internal helper methods

bool SearchManager::isWordBoundary(char c) const {
    return !(std::isalnum(static_cast<unsigned char>(c)) || c == '_');
}

std::string SearchManager::performReplacement(const std::string& text, const std::string& searchTerm, 
                                             const std::string& replaceText) const {
    if (options_.useRegex && options_.useRegexReplace) {
        try {
            std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
            if (!options_.matchCase) {
                flags |= std::regex_constants::icase;
            }
            
            std::regex regex(searchTerm, flags);
            return std::regex_replace(text, regex, replaceText);
        } catch (const std::regex_error&) {
            // Fall back to simple replacement
            return replaceText;
        }
    } else {
        // Simple text replacement
        if (options_.preserveCase && !text.empty() && !replaceText.empty()) {
            // Preserve the case of the first character
            std::string result = replaceText;
            
            if (std::isupper(text[0])) {
                if (!result.empty()) {
                    result[0] = static_cast<char>(std::toupper(result[0]));
                }
            } else if (std::islower(text[0])) {
                if (!result.empty()) {
                    result[0] = static_cast<char>(std::tolower(result[0]));
                }
            }
            
            return result;
        } else {
            return replaceText;
        }
    }
}

std::vector<SearchResult> 
SearchManager::searchInDocument(const std::string& searchTerm, Document* doc, 
                           int startLine, int startCol, int endLine, int endCol) const {
    std::vector<SearchResult> results;
    
    if (!doc || searchTerm.empty()) {
        return results;
    }
    
    const int lineCount = static_cast<int>(doc->getLineCount());
    
    // Adjust start and end lines
    startLine = std::max(0, startLine - 1); // Convert to 0-based
    startCol = std::max(0, startCol - 1);   // Convert to 0-based
    
    if (endLine <= 0) {
        endLine = lineCount;
    } else {
        endLine = std::min(lineCount, endLine);
    }
    
    if (endCol <= 0) {
        endCol = std::numeric_limits<int>::max();
    } else {
        endCol = std::max(0, endCol - 1);
    }
    
    // Pre-compile regex if needed
    std::regex regex;
    bool hasRegex = false;
    
    if (options_.useRegex) {
        try {
            std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
            if (!options_.matchCase) {
                flags |= std::regex_constants::icase;
            }
            
            regex = std::regex(searchTerm, flags);
            hasRegex = true;
        } catch (const std::regex_error&) {
            // Invalid regex, fall back to simple search
            hasRegex = false;
        }
    }
    
    // Search each line
    for (int line = startLine; line < endLine; ++line) {
        const auto& lineText = doc->getLine(static_cast<size_t>(line));
        size_t lineStart = (line == startLine) ? static_cast<size_t>(startCol) : 0;
        size_t lineEnd = (line == endLine - 1) ? 
            std::min(static_cast<size_t>(endCol), lineText.length()) : lineText.length();
        
        if (lineStart >= lineEnd) {
            continue;
        }
        
        std::string_view lineView(lineText.data() + lineStart, lineEnd - lineStart);
        
        if (hasRegex) {
            std::cregex_iterator it(lineView.begin(), lineView.end(), regex);
            std::cregex_iterator end;
            
            for (; it != end; ++it) {
                const std::cmatch& match = *it;
                
                SearchResult result;
                result.line = line + 1; // Convert to 1-based
                result.column = static_cast<int>(lineStart + match.position()) + 1; // Convert to 1-based
                result.length = static_cast<int>(match.length());
                result.lineText = lineText;
                result.matchText = match.str();
                result.byteOffset = 0; // Would need to be calculated based on encoding
                result.matchByteLength = match.length();
                
                results.push_back(result);
            }
        } else {
            // Simple text search
            size_t pos = lineStart;
            
            while (pos < lineEnd) {
                size_t foundPos = std::string::npos;
                
                if (options_.matchCase) {
                    foundPos = lineText.find(searchTerm, pos);
                } else {
                    std::string lineLower = lineText.substr(pos);
                    std::string termLower = searchTerm;
                    std::transform(lineLower.begin(), lineLower.end(), lineLower.begin(), ::tolower);
                    std::transform(termLower.begin(), termLower.end(), termLower.begin(), ::tolower);
                    
                    size_t found = lineLower.find(termLower);
                    if (found != std::string::npos) {
                        foundPos = pos + found;
                    }
                }
                
                if (foundPos == std::string::npos || foundPos >= lineEnd) {
                    break;
                }
                
                // Check for whole word if needed
                if (options_.matchWholeWord) {
                    if ((foundPos > 0 && !isWordBoundary(lineText[foundPos - 1])) ||
                        (foundPos + searchTerm.length() < lineText.length() && 
                         !isWordBoundary(lineText[foundPos + searchTerm.length()]))) {
                        pos = foundPos + 1;
                        continue;
                    }
                }
                
                SearchResult result;
                result.line = line + 1; // Convert to 1-based
                result.column = static_cast<int>(foundPos) + 1; // Convert to 1-based
                result.length = static_cast<int>(searchTerm.length());
                result.lineText = lineText;
                result.matchText = lineText.substr(foundPos, searchTerm.length());
                result.byteOffset = 0; // Would need to be calculated based on encoding
                result.matchByteLength = searchTerm.length();
                
                results.push_back(result);
                
                // Move past this match
                pos = foundPos + searchTerm.length();
            }
        }
    }
    
    return results;
}

} // namespace ai_editor
