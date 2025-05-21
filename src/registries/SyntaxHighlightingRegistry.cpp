#include "SyntaxHighlightingRegistry.h"
#include "../AppDebugLog.h"
#include <algorithm>
#include <iostream>
#include <cctype>

namespace ai_editor {

SyntaxHighlightingRegistry::SyntaxHighlightingRegistry() 
    : highlighters_(),
      extensionMap_(),
      highlighterExtensions_() {
    LOG_DEBUG("SyntaxHighlightingRegistry initialized");
}

bool SyntaxHighlightingRegistry::registerHighlighter(
    std::shared_ptr<SyntaxHighlighter> highlighter,
    const std::vector<std::string>& fileExtensions) {
    
    // Validate inputs
    if (!highlighter) {
        LOG_ERROR("Failed to register highlighter: Highlighter is null");
        return false;
    }
    
    // Get the highlighter ID (language name)
    std::string highlighterId = highlighter->getLanguageName();
    if (highlighterId.empty()) {
        LOG_ERROR("Failed to register highlighter: Empty highlighter ID");
        return false;
    }
    
    // Check if a highlighter with this ID already exists
    if (highlighters_.find(highlighterId) != highlighters_.end()) {
        LOG_ERROR("Failed to register highlighter: Highlighter with ID '" + highlighterId + "' already exists");
        return false;
    }
    
    // Store the highlighter
    highlighters_[highlighterId] = highlighter;
    LOG_DEBUG("Registered highlighter with ID: " + highlighterId);
    
    // Register all file extensions for this highlighter
    for (const auto& ext : fileExtensions) {
        std::string normalizedExt = normalizeExtension(ext);
        if (!normalizedExt.empty()) {
            extensionMap_[normalizedExt] = highlighterId;
            LOG_DEBUG("Mapped file extension '" + normalizedExt + "' to highlighter '" + highlighterId + "'");
        }
    }
    
    return true;
}

bool SyntaxHighlightingRegistry::unregisterHighlighter(const std::string& highlighterId) {
    // Check if the highlighter exists
    auto it = highlighters_.find(highlighterId);
    if (it == highlighters_.end()) {
        LOG_WARNING("Cannot unregister highlighter: Highlighter with ID '" + highlighterId + "' not found");
        return false;
    }
    
    // Remove all file extension mappings for this highlighter
    for (auto it = extensionMap_.begin(); it != extensionMap_.end();) {
        if (it->second == highlighterId) {
            LOG_DEBUG("Removing mapping for extension '" + it->first + "' from highlighter '" + highlighterId + "'");
            it = extensionMap_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove the highlighter
    highlighters_.erase(highlighterId);
    LOG_DEBUG("Unregistered highlighter with ID: " + highlighterId);
    
    return true;
}

std::shared_ptr<SyntaxHighlighter> SyntaxHighlightingRegistry::getHighlighterForExtension(
    const std::string& fileExtension) {
    
    std::string normalizedExt = normalizeExtension(fileExtension);
    
    // Look up in the extension map
    auto extIt = extensionMap_.find(normalizedExt);
    if (extIt == extensionMap_.end()) {
        LOG_DEBUG("No highlighter found for file extension: " + normalizedExt);
        return nullptr;
    }
    
    // Get the highlighter ID
    const std::string& highlighterId = extIt->second;
    
    // Look up the highlighter
    auto highlighterIt = highlighters_.find(highlighterId);
    if (highlighterIt == highlighters_.end()) {
        LOG_ERROR("Inconsistent state: Extension '" + normalizedExt + 
                  "' is mapped to nonexistent highlighter '" + highlighterId + "'");
        return nullptr;
    }
    
    return highlighterIt->second;
}

std::shared_ptr<SyntaxHighlighter> SyntaxHighlightingRegistry::getHighlighter(
    const std::string& highlighterId) {
    
    auto it = highlighters_.find(highlighterId);
    if (it == highlighters_.end()) {
        LOG_DEBUG("Highlighter not found with ID: " + highlighterId);
        return nullptr;
    }
    
    return it->second;
}

std::vector<std::string> SyntaxHighlightingRegistry::getAllHighlighterIds() {
    std::vector<std::string> result;
    result.reserve(highlighters_.size());
    
    for (const auto& pair : highlighters_) {
        result.push_back(pair.first);
    }
    
    // Sort for consistent results
    std::sort(result.begin(), result.end());
    
    return result;
}

std::vector<std::string> SyntaxHighlightingRegistry::getSupportedFileExtensions() {
    std::vector<std::string> result;
    result.reserve(extensionMap_.size());
    
    for (const auto& pair : extensionMap_) {
        result.push_back(pair.first);
    }
    
    // Sort for consistent results
    std::sort(result.begin(), result.end());
    
    return result;
}

bool SyntaxHighlightingRegistry::hasHighlighterForExtension(const std::string& fileExtension) {
    std::string normalizedExt = normalizeExtension(fileExtension);
    return extensionMap_.find(normalizedExt) != extensionMap_.end();
}

std::string SyntaxHighlightingRegistry::normalizeExtension(const std::string& extension) {
    std::string normalized = extension;
    
    // Remove leading dot if present
    if (!normalized.empty() && normalized[0] == '.') {
        normalized = normalized.substr(1);
    }
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    return normalized;
}

} // namespace ai_editor 