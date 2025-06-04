#pragma once

#include "interfaces/ILanguageDetector.hpp"
#include "EditorErrorReporter.h"
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <mutex>
#include <filesystem>

namespace ai_editor {

/**
 * @class LanguageDetector
 * @brief Implements language detection for files
 * 
 * This class is responsible for detecting the programming language
 * of a file based on its extension, content, or other heuristics.
 */
class LanguageDetector : public ILanguageDetector {
public:
    /**
     * @brief Constructor
     */
    LanguageDetector();
    
    /**
     * @brief Destructor
     */
    ~LanguageDetector() = default;
    
    // ILanguageDetector implementation
    std::optional<LanguageInfo> detectLanguageFromPath(const std::string& filePath) const override;
    std::optional<LanguageInfo> detectLanguageFromContent(
        const std::string& fileContent,
        const std::optional<std::string>& filePath) const override;
    std::optional<LanguageInfo> getLanguageInfo(const std::string& languageId) const override;
    std::vector<LanguageInfo> getAllLanguages() const override;
    bool registerLanguage(const LanguageInfo& languageInfo) override;
    bool shouldIgnoreFile(const std::string& filePath) const override;
    std::vector<std::string> getFileExtensions(const std::string& languageId) const override;
    std::optional<std::string> getLanguageIdForExtension(const std::string& extension) const override;
    
    /**
     * @brief Initialize with default language definitions
     */
    void initializeDefaultLanguages();
    
    /**
     * @brief Add file patterns to ignore during indexing
     * 
     * @param patterns Regular expression patterns for files to ignore
     */
    void addIgnorePatterns(const std::vector<std::string>& patterns);

private:
    /**
     * @brief Detect language from shebang line
     * 
     * @param firstLine The first line of the file
     * @return std::optional<std::string> The language ID, if detected
     */
    std::optional<std::string> detectLanguageFromShebang(const std::string& firstLine) const;
    
    /**
     * @brief Detect language from file content heuristics
     * 
     * @param fileContent The content of the file
     * @return std::optional<std::string> The language ID, if detected
     */
    std::optional<std::string> detectLanguageFromHeuristics(const std::string& fileContent) const;
    
    // Maps to store language information
    std::unordered_map<std::string, LanguageInfo> languages_;
    std::unordered_map<std::string, std::string> extensionToLanguage_;
    std::unordered_map<std::string, std::string> filenameToLanguage_;
    
    // Shebang patterns for script languages
    std::unordered_map<std::string, std::string> shebangPatterns_;
    
    // Patterns for files to ignore
    std::vector<std::regex> ignorePatterns_;
    
    // Default extensions to ignore
    std::unordered_set<std::string> ignoreExtensions_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
};

} // namespace ai_editor 