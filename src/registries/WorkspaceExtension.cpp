#include "WorkspaceExtension.h"
#include "../AppDebugLog.h"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace ai_editor {

WorkspaceExtension::WorkspaceExtension() {
    LOG_DEBUG("WorkspaceExtension initialized");
}

bool WorkspaceExtension::registerFileTypeHandler(std::shared_ptr<IFileTypeHandler> handler) {
    if (!handler) {
        LOG_ERROR("Attempted to register null file type handler");
        return false;
    }

    const std::string& handlerId = handler->getId();
    if (handlerId.empty()) {
        LOG_ERROR("File type handler has empty ID");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a handler with the same ID already exists
    if (fileTypeHandlers_.find(handlerId) != fileTypeHandlers_.end()) {
        LOG_ERROR("File type handler with ID '" + handlerId + "' already registered");
        return false;
    }
    
    // Register the handler
    fileTypeHandlers_[handlerId] = handler;
    
    // Register all supported file extensions for this handler
    const auto& supportedExtensions = handler->getSupportedFileExtensions();
    for (const auto& ext : supportedExtensions) {
        std::string normalizedExt = normalizeExtension(ext);
        
        // Log if we're overriding an existing extension mapping
        auto existingMapping = extensionToHandlerMap_.find(normalizedExt);
        if (existingMapping != extensionToHandlerMap_.end()) {
            LOG_WARNING("Extension '" + normalizedExt + "' already mapped to handler '" + 
                existingMapping->second + "', overriding with '" + handlerId + "'");
        }
        
        extensionToHandlerMap_[normalizedExt] = handlerId;
    }
    
    LOG_INFO("Registered file type handler '" + handlerId + "' with " + 
        std::to_string(supportedExtensions.size()) + " supported extensions");
    return true;
}

bool WorkspaceExtension::unregisterFileTypeHandler(const std::string& handlerId) {
    if (handlerId.empty()) {
        LOG_ERROR("Attempted to unregister file type handler with empty ID");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the handler exists
    auto handlerIt = fileTypeHandlers_.find(handlerId);
    if (handlerIt == fileTypeHandlers_.end()) {
        LOG_WARNING("File type handler '" + handlerId + "' not found for unregistration");
        return false;
    }
    
    // Remove all extension mappings for this handler
    auto handler = handlerIt->second;
    if (handler) {
        const auto& supportedExtensions = handler->getSupportedFileExtensions();
        for (const auto& ext : supportedExtensions) {
            std::string normalizedExt = normalizeExtension(ext);
            
            // Only remove the mapping if it points to this handler
            auto extIt = extensionToHandlerMap_.find(normalizedExt);
            if (extIt != extensionToHandlerMap_.end() && extIt->second == handlerId) {
                extensionToHandlerMap_.erase(extIt);
            }
        }
    }
    
    // Remove the handler itself
    fileTypeHandlers_.erase(handlerIt);
    
    LOG_INFO("Unregistered file type handler '" + handlerId + "'");
    return true;
}

std::shared_ptr<IFileTypeHandler> WorkspaceExtension::getFileTypeHandler(const std::string& fileExtension) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedExt = normalizeExtension(fileExtension);
    
    // Find the handler ID for this extension
    auto extIt = extensionToHandlerMap_.find(normalizedExt);
    if (extIt == extensionToHandlerMap_.end()) {
        return nullptr;
    }
    
    // Find the handler with this ID
    auto handlerIt = fileTypeHandlers_.find(extIt->second);
    if (handlerIt == fileTypeHandlers_.end()) {
        LOG_ERROR("Inconsistent state: extension '" + normalizedExt + 
            "' maps to non-existent handler '" + extIt->second + "'");
        return nullptr;
    }
    
    return handlerIt->second;
}

bool WorkspaceExtension::registerWorkspaceScanner(std::shared_ptr<IWorkspaceScanner> scanner) {
    if (!scanner) {
        LOG_ERROR("Attempted to register null workspace scanner");
        return false;
    }

    const std::string& scannerId = scanner->getId();
    if (scannerId.empty()) {
        LOG_ERROR("Workspace scanner has empty ID");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a scanner with the same ID already exists
    if (workspaceScanners_.find(scannerId) != workspaceScanners_.end()) {
        LOG_ERROR("Workspace scanner with ID '" + scannerId + "' already registered");
        return false;
    }
    
    // Register the scanner
    workspaceScanners_[scannerId] = scanner;
    
    LOG_INFO("Registered workspace scanner '" + scannerId + "'");
    return true;
}

bool WorkspaceExtension::unregisterWorkspaceScanner(const std::string& scannerId) {
    if (scannerId.empty()) {
        LOG_ERROR("Attempted to unregister workspace scanner with empty ID");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the scanner exists
    auto scannerIt = workspaceScanners_.find(scannerId);
    if (scannerIt == workspaceScanners_.end()) {
        LOG_WARNING("Workspace scanner '" + scannerId + "' not found for unregistration");
        return false;
    }
    
    // Remove the scanner
    workspaceScanners_.erase(scannerIt);
    
    LOG_INFO("Unregistered workspace scanner '" + scannerId + "'");
    return true;
}

std::shared_ptr<IWorkspaceScanner> WorkspaceExtension::getWorkspaceScanner(const std::string& scannerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto scannerIt = workspaceScanners_.find(scannerId);
    if (scannerIt == workspaceScanners_.end()) {
        return nullptr;
    }
    
    return scannerIt->second;
}

std::map<std::string, std::shared_ptr<IFileTypeHandler>> WorkspaceExtension::getAllFileTypeHandlers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Convert from unordered_map to map to get sorted results
    std::map<std::string, std::shared_ptr<IFileTypeHandler>> result;
    for (const auto& pair : fileTypeHandlers_) {
        result[pair.first] = pair.second;
    }
    
    return result;
}

std::map<std::string, std::shared_ptr<IWorkspaceScanner>> WorkspaceExtension::getAllWorkspaceScanners() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Convert from unordered_map to map to get sorted results
    std::map<std::string, std::shared_ptr<IWorkspaceScanner>> result;
    for (const auto& pair : workspaceScanners_) {
        result[pair.first] = pair.second;
    }
    
    return result;
}

std::string WorkspaceExtension::normalizeExtension(const std::string& fileExtension) const {
    std::string normalized = fileExtension;
    
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