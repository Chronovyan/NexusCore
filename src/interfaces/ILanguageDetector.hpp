#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace ai_editor {

/**
 * @struct LanguageInfo
 * @brief Information about a programming language
 */
struct LanguageInfo {
    std::string id;                      // Unique identifier for the language
    std::string name;                    // Human-readable name
    std::vector<std::string> extensions; // File extensions for this language
    std::vector<std::string> filenames;  // Special filenames for this language
    std::string lineCommentPrefix;       // Line comment prefix (e.g., "//")
    std::vector<std::string> blockCommentDelimiters; // Block comment delimiters (e.g., "/*", "*/")
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
};

/**
 * @interface ILanguageDetector
 * @brief Interface for detecting programming languages
 * 
 * This interface provides methods to detect the programming language
 * of a file based on its extension, content, or other heuristics.
 */
class ILanguageDetector {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ILanguageDetector() = default;
    
    /**
     * @brief Detect the language of a file based on its path
     * 
     * @param filePath The path to the file
     * @return std::optional<LanguageInfo> The detected language, if any
     */
    virtual std::optional<LanguageInfo> detectLanguageFromPath(const std::string& filePath) const = 0;
    
    /**
     * @brief Detect the language of a file based on its content
     * 
     * @param fileContent The content of the file
     * @param filePath Optional file path for additional context
     * @return std::optional<LanguageInfo> The detected language, if any
     */
    virtual std::optional<LanguageInfo> detectLanguageFromContent(
        const std::string& fileContent,
        const std::optional<std::string>& filePath = std::nullopt) const = 0;
    
    /**
     * @brief Get information about a specific language
     * 
     * @param languageId The language identifier
     * @return std::optional<LanguageInfo> Information about the language, if available
     */
    virtual std::optional<LanguageInfo> getLanguageInfo(const std::string& languageId) const = 0;
    
    /**
     * @brief Get all supported languages
     * 
     * @return std::vector<LanguageInfo> Information about all supported languages
     */
    virtual std::vector<LanguageInfo> getAllLanguages() const = 0;
    
    /**
     * @brief Register a custom language
     * 
     * @param languageInfo Information about the language to register
     * @return bool True if registration was successful
     */
    virtual bool registerLanguage(const LanguageInfo& languageInfo) = 0;
    
    /**
     * @brief Check if a file should be ignored for indexing
     * 
     * @param filePath The path to the file
     * @return bool True if the file should be ignored
     */
    virtual bool shouldIgnoreFile(const std::string& filePath) const = 0;
    
    /**
     * @brief Get file extensions for a language
     * 
     * @param languageId The language identifier
     * @return std::vector<std::string> File extensions for the language
     */
    virtual std::vector<std::string> getFileExtensions(const std::string& languageId) const = 0;
    
    /**
     * @brief Get the language ID for a file extension
     * 
     * @param extension The file extension (without the dot)
     * @return std::optional<std::string> The language ID, if any
     */
    virtual std::optional<std::string> getLanguageIdForExtension(const std::string& extension) const = 0;
};

} // namespace ai_editor 