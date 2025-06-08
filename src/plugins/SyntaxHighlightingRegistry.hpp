#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include "../interfaces/plugins/ISyntaxHighlightingRegistry.hpp"
#include "../AppDebugLog.h"

namespace ai_editor {

/**
 * @brief Implementation of the ISyntaxHighlightingRegistry interface.
 * 
 * This class manages syntax highlighters for different languages and file extensions.
 */
class SyntaxHighlightingRegistry : public ISyntaxHighlightingRegistry {
public:
    /**
     * @brief Constructor.
     */
    SyntaxHighlightingRegistry() {
        LOG_INFO("SyntaxHighlightingRegistry initialized");
    }

    /**
     * @brief Destructor.
     */
    ~SyntaxHighlightingRegistry() {
        LOG_INFO("SyntaxHighlightingRegistry destroyed");
    }

    /**
     * @brief Register a syntax highlighter for one or more file extensions.
     * @param highlighter The syntax highlighter to register.
     * @param fileExtensions The file extensions this highlighter should be associated with.
     * @return True if registration was successful, false otherwise.
     */
    bool registerHighlighter(
        std::shared_ptr<SyntaxHighlighter> highlighter,
        const std::vector<std::string>& fileExtensions) override {
        if (!highlighter) {
            LOG_ERROR("Attempted to register null highlighter");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string highlighterId = highlighter->getLanguageName();
        
        // Check if a highlighter with this ID already exists
        if (highlighters_.find(highlighterId) != highlighters_.end()) {
            LOG_WARNING("Highlighter with ID '" + highlighterId + "' already exists");
            return false;
        }
        
        // Store the highlighter
        highlighters_[highlighterId] = highlighter;
        
        // Associate the file extensions with this highlighter
        for (const auto& ext : fileExtensions) {
            std::string lowerExt = toLower(ext);
            extensionMap_[lowerExt] = highlighterId;
            LOG_DEBUG("Associated extension '" + lowerExt + "' with highlighter '" + highlighterId + "'");
        }
        
        // Add the extensions from the highlighter itself
        for (const auto& ext : highlighter->getSupportedExtensions()) {
            std::string lowerExt = toLower(ext);
            extensionMap_[lowerExt] = highlighterId;
            LOG_DEBUG("Associated extension '" + lowerExt + "' with highlighter '" + highlighterId + "'");
        }
        
        LOG_INFO("Registered syntax highlighter: " + highlighterId);
        return true;
    }

    /**
     * @brief Unregister a syntax highlighter by its ID.
     * @param highlighterId The ID of the highlighter to unregister.
     * @return True if unregistration was successful, false otherwise.
     */
    bool unregisterHighlighter(const std::string& highlighterId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = highlighters_.find(highlighterId);
        if (it == highlighters_.end()) {
            LOG_WARNING("Highlighter with ID '" + highlighterId + "' not found for unregistration");
            return false;
        }
        
        // Remove the highlighter
        auto highlighter = it->second;
        highlighters_.erase(it);
        
        // Remove the file extension associations
        for (auto it = extensionMap_.begin(); it != extensionMap_.end();) {
            if (it->second == highlighterId) {
                it = extensionMap_.erase(it);
            } else {
                ++it;
            }
        }
        
        LOG_INFO("Unregistered syntax highlighter: " + highlighterId);
        return true;
    }

    /**
     * @brief Get a highlighter by its ID.
     * @param highlighterId The ID of the highlighter to retrieve.
     * @return A shared pointer to the highlighter, or nullptr if not found.
     */
    std::shared_ptr<SyntaxHighlighter> getHighlighter(
        const std::string& highlighterId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = highlighters_.find(highlighterId);
        if (it == highlighters_.end()) {
            return nullptr;
        }
        
        return it->second;
    }

    /**
     * @brief Get a highlighter for a specific file extension.
     * @param fileExtension The file extension (without the leading dot).
     * @return A shared pointer to the appropriate highlighter, or nullptr if not found.
     */
    std::shared_ptr<SyntaxHighlighter> getHighlighterForExtension(
        const std::string& fileExtension) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string lowerExt = toLower(fileExtension);
        auto it = extensionMap_.find(lowerExt);
        if (it == extensionMap_.end()) {
            return nullptr;
        }
        
        std::string highlighterId = it->second;
        auto highlighterIt = highlighters_.find(highlighterId);
        if (highlighterIt == highlighters_.end()) {
            return nullptr;
        }
        
        return highlighterIt->second;
    }

    /**
     * @brief Check if a highlighter exists for a specific file extension.
     * @param fileExtension The file extension (without the leading dot).
     * @return True if a highlighter exists for the extension, false otherwise.
     */
    bool hasHighlighterForExtension(const std::string& fileExtension) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string lowerExt = toLower(fileExtension);
        return extensionMap_.find(lowerExt) != extensionMap_.end();
    }

    /**
     * @brief Get all registered highlighter IDs.
     * @return A vector of highlighter IDs.
     */
    std::vector<std::string> getAllHighlighterIds() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> ids;
        ids.reserve(highlighters_.size());
        
        for (const auto& pair : highlighters_) {
            ids.push_back(pair.first);
        }
        
        return ids;
    }

    /**
     * @brief Get all supported file extensions.
     * @return A vector of supported file extensions.
     */
    std::vector<std::string> getSupportedFileExtensions() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> extensions;
        extensions.reserve(extensionMap_.size());
        
        for (const auto& pair : extensionMap_) {
            extensions.push_back(pair.first);
        }
        
        return extensions;
    }

private:
    /**
     * @brief Convert a string to lowercase.
     * @param str The string to convert.
     * @return The lowercase string.
     */
    std::string toLower(const std::string& str) const {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::unordered_map<std::string, std::shared_ptr<SyntaxHighlighter>> highlighters_;
    std::unordered_map<std::string, std::string> extensionMap_;  // file extension -> highlighter ID
    mutable std::mutex mutex_;  // Protects all data structures
};

} // namespace ai_editor 