#pragma once

#include <string>
#include <vector>

/**
 * @interface IWorkspaceManager
 * @brief Interface for workspace file operations
 * 
 * Defines the contract for components that handle file operations within
 * a workspace directory.
 */
class IWorkspaceManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IWorkspaceManager() = default;
    
    /**
     * @brief Write content to a file
     * 
     * @param filename The name of the file to write
     * @param content The content to write to the file
     * @return bool True if the file was written successfully
     */
    virtual bool writeFile(const std::string& filename, const std::string& content) = 0;
    
    /**
     * @brief Check if a file exists in the workspace
     * 
     * @param filename The name of the file to check
     * @return bool True if the file exists
     */
    virtual bool fileExists(const std::string& filename) const = 0;
    
    /**
     * @brief Get a list of all files in the workspace
     * 
     * @return std::vector<std::string> A list of filenames
     */
    virtual std::vector<std::string> listFiles() const = 0;
    
    /**
     * @brief Get the content of a file
     * 
     * @param filename The name of the file to read
     * @return std::string The content of the file, or empty string if the file doesn't exist
     */
    virtual std::string readFile(const std::string& filename) const = 0;
    
    /**
     * @brief Get the full path to the workspace directory
     * 
     * @return std::string The absolute path to the workspace directory
     */
    virtual std::string getWorkspacePath() const = 0;
    
    /**
     * @brief Create a directory within the workspace
     * 
     * @param dirname The name of the directory to create
     * @return bool True if the directory was created successfully
     */
    virtual bool createDirectory(const std::string& dirname) = 0;
    
    /**
     * @brief Delete a file from the workspace
     * 
     * @param filename The name of the file to delete
     * @return bool True if the file was deleted successfully
     */
    virtual bool deleteFile(const std::string& filename) = 0;
    
    /**
     * @brief Rename a file in the workspace
     * 
     * @param oldFilename The current name of the file
     * @param newFilename The new name for the file
     * @return bool True if the file was renamed successfully
     */
    virtual bool renameFile(const std::string& oldFilename, const std::string& newFilename) = 0;
}; 