#ifndef IWORKSPACE_EXTENSION_HPP
#define IWORKSPACE_EXTENSION_HPP

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>

/**
 * @brief Interface for handling specific file types.
 * 
 * Plugins can implement this interface to provide custom handling
 * for specific file types, such as special editing capabilities or
 * file format conversion.
 */
class IFileTypeHandler {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IFileTypeHandler() = default;
    
    /**
     * @brief Get the unique identifier for this file type handler.
     * 
     * @return The handler's unique identifier.
     */
    virtual std::string getId() const = 0;
    
    /**
     * @brief Get the display name for this file type handler.
     * 
     * @return The handler's display name.
     */
    virtual std::string getDisplayName() const = 0;
    
    /**
     * @brief Get the file extensions this handler can process.
     * 
     * @return A vector of file extensions (without the dot) this handler supports.
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    
    /**
     * @brief Check if this handler can process a specific file.
     * 
     * @param filePath The path to the file to check.
     * @return true if this handler can process the file, false otherwise.
     */
    virtual bool canHandleFile(const std::string& filePath) const = 0;
    
    /**
     * @brief Open a file with this handler.
     * 
     * @param filePath The path to the file to open.
     * @return true if the file was opened successfully, false otherwise.
     */
    virtual bool openFile(const std::string& filePath) = 0;
    
    /**
     * @brief Save a file with this handler.
     * 
     * @param filePath The path to save the file to.
     * @return true if the file was saved successfully, false otherwise.
     */
    virtual bool saveFile(const std::string& filePath) = 0;
};

/**
 * @brief Interface for scanning workspace content.
 * 
 * Plugins can implement this interface to provide custom scanning
 * functionality for the workspace, such as indexing files for
 * search or providing custom project structure visualization.
 */
class IWorkspaceScanner {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IWorkspaceScanner() = default;
    
    /**
     * @brief Get the unique identifier for this workspace scanner.
     * 
     * @return The scanner's unique identifier.
     */
    virtual std::string getId() const = 0;
    
    /**
     * @brief Get the display name for this workspace scanner.
     * 
     * @return The scanner's display name.
     */
    virtual std::string getDisplayName() const = 0;
    
    /**
     * @brief Scan the workspace or a subdirectory.
     * 
     * @param directoryPath The path to the directory to scan. Empty for the whole workspace.
     * @return true if the scan was successful, false otherwise.
     */
    virtual bool scan(const std::string& directoryPath = "") = 0;
    
    /**
     * @brief Get the file types this scanner is interested in.
     * 
     * @return A vector of file extensions (without the dot) this scanner is interested in.
     */
    virtual std::vector<std::string> getInterestingFileTypes() const = 0;
    
    /**
     * @brief Check if this scanner is currently scanning.
     * 
     * @return true if a scan is in progress, false otherwise.
     */
    virtual bool isScanning() const = 0;
    
    /**
     * @brief Cancel a scan in progress.
     * 
     * @return true if a scan was canceled, false if no scan was in progress.
     */
    virtual bool cancelScan() = 0;
};

/**
 * @brief Interface for extending workspace functionality.
 * 
 * Plugins can use this interface to register custom workspace scanners
 * and file type handlers.
 */
class IWorkspaceExtension {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IWorkspaceExtension() = default;
    
    /**
     * @brief Register a custom file type handler.
     * 
     * @param handler A shared pointer to the IFileTypeHandler implementation.
     * @return true if registration succeeded, false otherwise.
     */
    virtual bool registerFileTypeHandler(std::shared_ptr<IFileTypeHandler> handler) = 0;
    
    /**
     * @brief Unregister a file type handler.
     * 
     * @param handlerId The unique identifier of the handler to unregister.
     * @return true if the handler was found and unregistered, false otherwise.
     */
    virtual bool unregisterFileTypeHandler(const std::string& handlerId) = 0;
    
    /**
     * @brief Get a file type handler for a specific file extension.
     * 
     * @param fileExtension The file extension (without the dot) to get a handler for.
     * @return A shared pointer to the IFileTypeHandler, or nullptr if not found.
     */
    virtual std::shared_ptr<IFileTypeHandler> getFileTypeHandler(const std::string& fileExtension) const = 0;
    
    /**
     * @brief Register a workspace scanner.
     * 
     * @param scanner A shared pointer to the IWorkspaceScanner implementation.
     * @return true if registration succeeded, false otherwise.
     */
    virtual bool registerWorkspaceScanner(std::shared_ptr<IWorkspaceScanner> scanner) = 0;
    
    /**
     * @brief Unregister a workspace scanner.
     * 
     * @param scannerId The unique identifier of the scanner to unregister.
     * @return true if the scanner was found and unregistered, false otherwise.
     */
    virtual bool unregisterWorkspaceScanner(const std::string& scannerId) = 0;
    
    /**
     * @brief Get a workspace scanner by its ID.
     * 
     * @param scannerId The unique identifier of the scanner to retrieve.
     * @return A shared pointer to the IWorkspaceScanner, or nullptr if not found.
     */
    virtual std::shared_ptr<IWorkspaceScanner> getWorkspaceScanner(const std::string& scannerId) const = 0;
    
    /**
     * @brief Get all registered file type handlers.
     * 
     * @return A map of handler IDs to handlers.
     */
    virtual std::map<std::string, std::shared_ptr<IFileTypeHandler>> getAllFileTypeHandlers() const = 0;
    
    /**
     * @brief Get all registered workspace scanners.
     * 
     * @return A map of scanner IDs to scanners.
     */
    virtual std::map<std::string, std::shared_ptr<IWorkspaceScanner>> getAllWorkspaceScanners() const = 0;
};

#endif // IWORKSPACE_EXTENSION_HPP 