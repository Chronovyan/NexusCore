#pragma once

#include "../interfaces/plugins/IWorkspaceExtension.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>

namespace ai_editor {

/**
 * @class WorkspaceExtension
 * @brief Implementation of the IWorkspaceExtension interface.
 * 
 * This class manages file type handlers and workspace scanners, allowing plugins
 * to extend the editor's capabilities for working with different file types and
 * scanning workspace content.
 */
class WorkspaceExtension : public IWorkspaceExtension {
public:
    /**
     * @brief Constructor.
     */
    WorkspaceExtension();
    
    /**
     * @brief Destructor.
     */
    ~WorkspaceExtension() override = default;
    
    /**
     * @brief Register a custom file type handler.
     * 
     * @param handler A shared pointer to the IFileTypeHandler implementation.
     * @return true if registration succeeded, false otherwise.
     */
    bool registerFileTypeHandler(std::shared_ptr<IFileTypeHandler> handler) override;
    
    /**
     * @brief Unregister a file type handler.
     * 
     * @param handlerId The unique identifier of the handler to unregister.
     * @return true if the handler was found and unregistered, false otherwise.
     */
    bool unregisterFileTypeHandler(const std::string& handlerId) override;
    
    /**
     * @brief Get a file type handler for a specific file extension.
     * 
     * @param fileExtension The file extension (without the dot) to get a handler for.
     * @return A shared pointer to the IFileTypeHandler, or nullptr if not found.
     */
    std::shared_ptr<IFileTypeHandler> getFileTypeHandler(const std::string& fileExtension) const override;
    
    /**
     * @brief Register a workspace scanner.
     * 
     * @param scanner A shared pointer to the IWorkspaceScanner implementation.
     * @return true if registration succeeded, false otherwise.
     */
    bool registerWorkspaceScanner(std::shared_ptr<IWorkspaceScanner> scanner) override;
    
    /**
     * @brief Unregister a workspace scanner.
     * 
     * @param scannerId The unique identifier of the scanner to unregister.
     * @return true if the scanner was found and unregistered, false otherwise.
     */
    bool unregisterWorkspaceScanner(const std::string& scannerId) override;
    
    /**
     * @brief Get a workspace scanner by its ID.
     * 
     * @param scannerId The unique identifier of the scanner to retrieve.
     * @return A shared pointer to the IWorkspaceScanner, or nullptr if not found.
     */
    std::shared_ptr<IWorkspaceScanner> getWorkspaceScanner(const std::string& scannerId) const override;
    
    /**
     * @brief Get all registered file type handlers.
     * 
     * @return A map of handler IDs to handlers.
     */
    std::map<std::string, std::shared_ptr<IFileTypeHandler>> getAllFileTypeHandlers() const override;
    
    /**
     * @brief Get all registered workspace scanners.
     * 
     * @return A map of scanner IDs to scanners.
     */
    std::map<std::string, std::shared_ptr<IWorkspaceScanner>> getAllWorkspaceScanners() const override;
    
private:
    /**
     * @brief Normalize a file extension for consistent lookups.
     * 
     * @param fileExtension The file extension to normalize.
     * @return The normalized file extension.
     */
    std::string normalizeExtension(const std::string& fileExtension) const;
    
    /** @brief Mutex for thread-safe access to handlers and scanners */
    mutable std::mutex mutex_;
    
    /** @brief Map of handler IDs to file type handlers */
    std::unordered_map<std::string, std::shared_ptr<IFileTypeHandler>> fileTypeHandlers_;
    
    /** @brief Map of file extensions to handler IDs */
    std::unordered_map<std::string, std::string> extensionToHandlerMap_;
    
    /** @brief Map of scanner IDs to workspace scanners */
    std::unordered_map<std::string, std::shared_ptr<IWorkspaceScanner>> workspaceScanners_;
};

} // namespace ai_editor 