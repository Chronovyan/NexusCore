#pragma once

#include "../interfaces/plugins/ISyntaxHighlightingRegistry.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include <set>

namespace ai_editor {

/**
 * @class SyntaxHighlightingRegistry
 * @brief Implementation of the ISyntaxHighlightingRegistry interface.
 * 
 * This class manages syntax highlighters for different languages and file extensions,
 * allowing the editor to apply appropriate syntax highlighting based on the file type.
 */
class SyntaxHighlightingRegistry : public ISyntaxHighlightingRegistry {
public:
    /**
     * @brief Constructor.
     */
    SyntaxHighlightingRegistry();
    
    /**
     * @brief Destructor.
     */
    ~SyntaxHighlightingRegistry() override = default;
    
    /**
     * @brief Register a syntax highlighter for one or more file extensions.
     * @param highlighter The syntax highlighter to register.
     * @param fileExtensions The file extensions this highlighter should be associated with.
     * @return True if registration was successful, false otherwise.
     */
    bool registerHighlighter(
        std::shared_ptr<SyntaxHighlighter> highlighter, 
        const std::vector<std::string>& fileExtensions) override;
    
    /**
     * @brief Unregister a syntax highlighter by its ID.
     * @param highlighterId The ID of the highlighter to unregister.
     * @return True if unregistration was successful, false otherwise.
     */
    bool unregisterHighlighter(const std::string& highlighterId) override;
    
    /**
     * @brief Get a highlighter by its ID.
     * @param highlighterId The ID of the highlighter to retrieve.
     * @return A shared pointer to the highlighter, or nullptr if not found.
     */
    std::shared_ptr<SyntaxHighlighter> getHighlighter(
        const std::string& highlighterId) override;
    
    /**
     * @brief Get a highlighter for a specific file extension.
     * @param fileExtension The file extension (without the leading dot).
     * @return A shared pointer to the appropriate highlighter, or nullptr if not found.
     */
    std::shared_ptr<SyntaxHighlighter> getHighlighterForExtension(
        const std::string& fileExtension) override;
    
    /**
     * @brief Check if a highlighter exists for a specific file extension.
     * @param fileExtension The file extension (without the leading dot).
     * @return True if a highlighter exists for the extension, false otherwise.
     */
    bool hasHighlighterForExtension(const std::string& fileExtension) override;
    
    /**
     * @brief Get all registered highlighter IDs.
     * @return A vector of highlighter IDs.
     */
    std::vector<std::string> getAllHighlighterIds() override;
    
    /**
     * @brief Get all supported file extensions.
     * @return A vector of supported file extensions.
     */
    std::vector<std::string> getSupportedFileExtensions() override;
    
private:
    /**
     * @brief Normalize a file extension by removing leading dot and converting to lowercase.
     * @param extension The file extension to normalize.
     * @return The normalized file extension.
     */
    std::string normalizeExtension(const std::string& extension);
    
    /** @brief Map of highlighter IDs to highlighter instances */
    std::unordered_map<std::string, std::shared_ptr<SyntaxHighlighter>> highlighters_;
    
    /** @brief Map of file extensions to highlighter IDs */
    std::map<std::string, std::string> extensionMap_;
};

} // namespace ai_editor 