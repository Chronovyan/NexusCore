#pragma once

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include "../core/Document.h"
#include "../interfaces/ICodebaseIndex.hpp"

namespace ai_editor {

// Using SearchResult from ICodebaseIndex.hpp

/**
 * @class SearchManager
 * @brief Handles search and replace operations in a document
 */
class SearchManager {
public:
    /**
     * @brief Search options
     */
    struct Options {
        bool matchCase = false;      // Case-sensitive search
        bool matchWholeWord = false; // Match whole words only
        bool useRegex = false;       // Use regular expressions
        bool wrapAround = false;     // Wrap around when reaching the end
        bool searchUp = false;       // Search direction (up/down)
        
        // For replace operations
        bool preserveCase = false;   // Preserve case when replacing
        
        // For regex replace
        bool useRegexReplace = false; // Use regex replacement patterns
        
        // For search in selection
        bool inSelection = false;     // Search only in selection
        
        // For search in multiple files
        bool inAllOpenFiles = false;  // Search in all open files
        
        // File search options
        std::vector<std::string> filePatterns; // File patterns to search in
        bool searchSubdirectories = true;      // Search in subdirectories
        bool ignoreBinaryFiles = true;         // Skip binary files
        
        // File size limits
        size_t maxFileSize = 10 * 1024 * 1024; // 10MB default max file size
    };
    
    /**
     * @brief Constructor
     * @param document The document to search in (can be null)
     */
    explicit SearchManager(Document* document = nullptr);
    
    /**
     * @brief Set the document to search in
     */
    void setDocument(Document* document) { document_ = document; }
    
    /**
     * @brief Get the current search options
     */
    const Options& getOptions() const { return options_; }
    
    /**
     * @brief Set the search options
     */
    void setOptions(const Options& options) { options_ = options; }
    
    /**
     * @brief Find the next occurrence of the search term
     * @param searchTerm The term to search for
     * @param startLine Line to start searching from (1-based)
     * @param startColumn Column to start searching from (1-based)
     * @return Search result, or an empty result if not found
     */
    SearchResult findNext(const std::string& searchTerm, int startLine = 1, int startColumn = 1);
    
    /**
     * @brief Find the previous occurrence of the search term
     * @param searchTerm The term to search for
     * @param startLine Line to start searching from (1-based)
     * @param startColumn Column to start searching from (1-based)
     * @return Search result, or an empty result if not found
     */
    SearchResult findPrevious(const std::string& searchTerm, int startLine = 1, int startColumn = 1);
    
    /**
     * @brief Find all occurrences of the search term in the document
     * @param searchTerm The term to search for
     * @return Vector of search results
     */
    std::vector<SearchResult> findAll(const std::string& searchTerm);
    
    /**
     * @brief Replace the next occurrence of the search term with the replacement text
     * @param searchTerm The term to search for
     * @param replaceText The text to replace with
     * @param startLine Line to start searching from (1-based)
     * @param startColumn Column to start searching from (1-based)
     * @return The search result that was replaced, or an empty result if not found
     */
    SearchResult replaceNext(const std::string& searchTerm, const std::string& replaceText, 
                            int startLine = 1, int startColumn = 1);
    
    /**
     * @brief Replace all occurrences of the search term with the replacement text
     * @param searchTerm The term to search for
     * @param replaceText The text to replace with
     * @return Number of replacements made
     */
    int replaceAll(const std::string& searchTerm, const std::string& replaceText);
    
    /**
     * @brief Count all occurrences of the search term in the document
     * @param searchTerm The term to search for
     * @return Number of occurrences
     */
    int countAll(const std::string& searchTerm);
    
    /**
     * @brief Find in files
     * @param searchTerm The term to search for
     * @param directory The directory to search in
     * @param filePatterns File patterns to search for (e.g., "*.cpp,*.h")
     * @return Map of file paths to search results
     */
    std::unordered_map<std::string, std::vector<SearchResult>> 
    findInFiles(const std::string& searchTerm, const std::string& directory, 
               const std::vector<std::string>& filePatterns);
    
    /**
     * @brief Replace in files
     * @param searchTerm The term to search for
     * @param replaceText The text to replace with
     * @param directory The directory to search in
     * @param filePatterns File patterns to search for (e.g., "*.cpp,*.h")
     * @param dryRun If true, don't actually modify any files
     * @return Map of file paths to replacement counts and backup file paths
     */
    std::unordered_map<std::string, std::pair<int, std::string>>
    replaceInFiles(const std::string& searchTerm, const std::string& replaceText,
                  const std::string& directory, const std::vector<std::string>& filePatterns,
                  bool dryRun = true);
    
    /**
     * @brief Get the current search history
     */
    const std::vector<std::string>& getSearchHistory() const { return searchHistory_; }
    
    /**
     * @brief Get the current replace history
     */
    const std::vector<std::string>& getReplaceHistory() const { return replaceHistory_; }
    
    /**
     * @brief Add a term to the search history
     */
    void addToSearchHistory(const std::string& term);
    
    /**
     * @brief Add a term to the replace history
     */
    void addToReplaceHistory(const std::string& term);
    
    /**
     * @brief Clear the search history
     */
    void clearSearchHistory() { searchHistory_.clear(); }
    
    /**
     * @brief Clear the replace history
     */
    void clearReplaceHistory() { replaceHistory_.clear(); }
    
    /**
     * @brief Set the maximum number of history items to keep
     */
    void setMaxHistorySize(size_t size) { maxHistorySize_ = size; }
    
private:
    Document* document_ = nullptr;
    Options options_;
    std::vector<std::string> searchHistory_;
    std::vector<std::string> replaceHistory_;
    size_t maxHistorySize_ = 100;
    
    // Internal helper methods
    bool matchPattern(const std::string& text, const std::string& pattern, size_t& matchLength) const;
    std::string performReplacement(const std::string& text, const std::string& searchTerm, 
                                  const std::string& replaceText) const;
    std::string escapeRegex(const std::string& str) const;
    bool isWordBoundary(char c) const;
    
    // For regex searches
    mutable std::regex regexCache_;
    mutable std::string lastRegexPattern_;
    mutable bool lastRegexCaseSensitive_ = false;
    
    // For progress reporting
    using ProgressCallback = std::function<bool(int current, int total, const std::string& status)>;
    
    // Internal search implementation
    std::vector<SearchResult> searchInDocument(const std::string& searchTerm, Document* doc, 
                                             int startLine = 1, int startCol = 1, 
                                             int endLine = -1, int endCol = -1) const;
    
    // File search implementation
    void findInFile(const std::string& filePath, const std::string& searchTerm,
                   std::vector<SearchResult>& results, ProgressCallback progress = nullptr) const;
    
    // File search with line-based processing (for large files)
    void findInFileLineByLine(std::ifstream& file, const std::string& filePath,
                             const std::string& searchTerm, std::vector<SearchResult>& results,
                             ProgressCallback progress = nullptr) const;
};

} // namespace ai_editor
