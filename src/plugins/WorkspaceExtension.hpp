#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include "../interfaces/plugins/IWorkspaceExtension.hpp"
#include "../AppDebugLog.h"

/**
 * @brief Implementation of the IWorkspaceExtension interface.
 * 
 * This class manages file type handlers and workspace scanners.
 */
class WorkspaceExtension : public IWorkspaceExtension {
public:
    /**
     * @brief Constructor.
     */
    WorkspaceExtension() {
        LOG_INFO("WorkspaceExtension initialized");
    }

    /**
     * @brief Destructor.
     */
    ~WorkspaceExtension() {
        LOG_INFO("WorkspaceExtension destroyed");
    }

    /**
     * @brief Register a custom file type handler.
     * 
     * @param handler A shared pointer to the IFileTypeHandler implementation.
     * @return true if registration succeeded, false otherwise.
     */
    bool registerFileTypeHandler(std::shared_ptr<IFileTypeHandler> handler) override {
        if (!handler) {
            LOG_ERROR("Attempted to register null file type handler");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string handlerId = handler->getId();
        
        // Check if a handler with this ID already exists
        if (fileTypeHandlers_.find(handlerId) != fileTypeHandlers_.end()) {
            LOG_WARNING("File type handler with ID '" + handlerId + "' already exists");
            return false;
        }
        
        // Store the handler
        fileTypeHandlers_[handlerId] = handler;
        
        // Associate the file extensions with this handler
        for (const auto& ext : handler->getSupportedExtensions()) {
            std::string lowerExt = toLower(ext);
            extensionHandlerMap_[lowerExt] = handlerId;
            LOG_DEBUG("Associated extension '" + lowerExt + "' with file type handler '" + handlerId + "'");
        }
        
        LOG_INFO("Registered file type handler: " + handlerId + " (" + handler->getDisplayName() + ")");
        return true;
    }

    /**
     * @brief Unregister a file type handler.
     *
     * @param handlerId The unique identifier of the handler to unregister.
     * @return true if the handler was found and unregistered, false otherwise.
     */
    bool unregisterFileTypeHandler(const std::string& handlerId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = fileTypeHandlers_.find(handlerId);
        if (it == fileTypeHandlers_.end()) {
            LOG_WARNING("File type handler with ID '" + handlerId + "' not found for unregistration");
            return false;
        }
        
        // Get the handler
        auto handler = it->second;
        
        // Remove the handler
        fileTypeHandlers_.erase(it);
        
        // Remove the file extension associations
        for (auto it = extensionHandlerMap_.begin(); it != extensionHandlerMap_.end();) {
            if (it->second == handlerId) {
                it = extensionHandlerMap_.erase(it);
            } else {
                ++it;
            }
        }
        
        LOG_INFO("Unregistered file type handler: " + handlerId);
        return true;
    }

    /**
     * @brief Get a file type handler for a specific file extension.
     *
     * @param fileExtension The file extension (without the dot) to get a handler for.
     * @return A shared pointer to the IFileTypeHandler, or nullptr if not found.
     */
    std::shared_ptr<IFileTypeHandler> getFileTypeHandler(const std::string& fileExtension) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string lowerExt = toLower(fileExtension);
        auto it = extensionHandlerMap_.find(lowerExt);
        if (it == extensionHandlerMap_.end()) {
            return nullptr;
        }
        
        std::string handlerId = it->second;
        auto handlerIt = fileTypeHandlers_.find(handlerId);
        if (handlerIt == fileTypeHandlers_.end()) {
            return nullptr;
        }
        
        return handlerIt->second;
    }

    /**
     * @brief Register a workspace scanner.
     *
     * @param scanner A shared pointer to the IWorkspaceScanner implementation.
     * @return true if registration succeeded, false otherwise.
     */
    bool registerWorkspaceScanner(std::shared_ptr<IWorkspaceScanner> scanner) override {
        if (!scanner) {
            LOG_ERROR("Attempted to register null workspace scanner");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string scannerId = scanner->getId();
        
        // Check if a scanner with this ID already exists
        if (workspaceScanners_.find(scannerId) != workspaceScanners_.end()) {
            LOG_WARNING("Workspace scanner with ID '" + scannerId + "' already exists");
            return false;
        }
        
        // Store the scanner
        workspaceScanners_[scannerId] = scanner;
        
        LOG_INFO("Registered workspace scanner: " + scannerId + " (" + scanner->getDisplayName() + ")");
        return true;
    }

    /**
     * @brief Unregister a workspace scanner.
     *
     * @param scannerId The unique identifier of the scanner to unregister.
     * @return true if the scanner was found and unregistered, false otherwise.
     */
    bool unregisterWorkspaceScanner(const std::string& scannerId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = workspaceScanners_.find(scannerId);
        if (it == workspaceScanners_.end()) {
            LOG_WARNING("Workspace scanner with ID '" + scannerId + "' not found for unregistration");
            return false;
        }
        
        // Remove the scanner
        workspaceScanners_.erase(it);
        
        LOG_INFO("Unregistered workspace scanner: " + scannerId);
        return true;
    }

    /**
     * @brief Get a workspace scanner by its ID.
     *
     * @param scannerId The unique identifier of the scanner to retrieve.
     * @return A shared pointer to the IWorkspaceScanner, or nullptr if not found.
     */
    std::shared_ptr<IWorkspaceScanner> getWorkspaceScanner(const std::string& scannerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = workspaceScanners_.find(scannerId);
        if (it == workspaceScanners_.end()) {
            return nullptr;
        }
        
        return it->second;
    }

    /**
     * @brief Get all registered file type handlers.
     *
     * @return A map of handler IDs to handlers.
     */
    std::map<std::string, std::shared_ptr<IFileTypeHandler>> getAllFileTypeHandlers() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::map<std::string, std::shared_ptr<IFileTypeHandler>> result;
        for (const auto& pair : fileTypeHandlers_) {
            result[pair.first] = pair.second;
        }
        
        return result;
    }

    /**
     * @brief Get all registered workspace scanners.
     *
     * @return A map of scanner IDs to scanners.
     */
    std::map<std::string, std::shared_ptr<IWorkspaceScanner>> getAllWorkspaceScanners() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::map<std::string, std::shared_ptr<IWorkspaceScanner>> result;
        for (const auto& pair : workspaceScanners_) {
            result[pair.first] = pair.second;
        }
        
        return result;
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

    std::unordered_map<std::string, std::shared_ptr<IFileTypeHandler>> fileTypeHandlers_;
    std::unordered_map<std::string, std::string> extensionHandlerMap_;  // file extension -> handler ID
    
    std::unordered_map<std::string, std::shared_ptr<IWorkspaceScanner>> workspaceScanners_;
    
    mutable std::mutex mutex_;  // Protects all data structures
}; 