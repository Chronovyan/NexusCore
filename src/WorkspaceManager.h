#ifndef WORKSPACE_MANAGER_H
#define WORKSPACE_MANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace ai_editor {

/**
 * @class WorkspaceManager
 * @brief Manages file operations in the project workspace
 * 
 * This class handles reading and writing files for AI-generated content.
 * It provides methods for creating files with content, checking if files
 * exist, and listing files in the workspace.
 */
class WorkspaceManager {
public:
    /**
     * @brief Constructor
     * 
     * @param workspacePath The base directory for the workspace
     */
    explicit WorkspaceManager(const std::string& workspacePath);
    
    /**
     * @brief Write content to a file
     * 
     * @param filename The name of the file to write
     * @param content The content to write to the file
     * @return bool True if the file was written successfully
     */
    bool writeFile(const std::string& filename, const std::string& content);
    
    /**
     * @brief Check if a file exists in the workspace
     * 
     * @param filename The name of the file to check
     * @return bool True if the file exists
     */
    bool fileExists(const std::string& filename) const;
    
    /**
     * @brief Get a list of all files in the workspace
     * 
     * @return std::vector<std::string> A list of filenames
     */
    std::vector<std::string> listFiles() const;
    
    /**
     * @brief Get the content of a file
     * 
     * @param filename The name of the file to read
     * @return std::string The content of the file, or empty string if the file doesn't exist
     */
    std::string readFile(const std::string& filename) const;
    
    /**
     * @brief Get the workspace path
     * 
     * @return std::string The base directory for the workspace
     */
    std::string getWorkspacePath() const { return workspacePath_; }

private:
    // The base directory for the workspace
    std::string workspacePath_;
    
    // Ensure all directories in a path exist
    bool ensureDirectoryExists(const std::string& path);
};

} // namespace ai_editor

#endif // WORKSPACE_MANAGER_H 