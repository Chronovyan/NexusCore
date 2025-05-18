#include "WorkspaceManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace ai_editor {

WorkspaceManager::WorkspaceManager(const std::string& workspacePath)
    : workspacePath_(workspacePath)
{
    // Ensure the workspace path ends with a separator
    if (!workspacePath_.empty() && workspacePath_.back() != '/' && workspacePath_.back() != '\\') {
        workspacePath_ += "/";
    }
    
    // Ensure the workspace directory exists
    ensureDirectoryExists(workspacePath_);
}

bool WorkspaceManager::writeFile(const std::string& filename, const std::string& content)
{
    try {
        // Get the full path to the file
        std::string fullPath = workspacePath_ + filename;
        
        // Ensure the directory exists
        std::filesystem::path filePath(fullPath);
        std::string directory = filePath.parent_path().string();
        
        if (!directory.empty() && !ensureDirectoryExists(directory)) {
            std::cerr << "Error: Failed to create directory for file: " << filename << std::endl;
            return false;
        }
        
        // Open the file for writing
        std::ofstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << fullPath << std::endl;
            return false;
        }
        
        // Write the content to the file
        file << content;
        
        // Check for errors
        if (file.bad()) {
            std::cerr << "Error: Failed to write content to file: " << fullPath << std::endl;
            return false;
        }
        
        // Close the file
        file.close();
        
        std::cout << "File created/updated successfully: " << filename << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while writing file " << filename << ": " << e.what() << std::endl;
        return false;
    }
}

bool WorkspaceManager::fileExists(const std::string& filename) const
{
    std::string fullPath = workspacePath_ + filename;
    return std::filesystem::exists(fullPath);
}

std::vector<std::string> WorkspaceManager::listFiles() const
{
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(workspacePath_)) {
            if (entry.is_regular_file()) {
                // Get the relative path to the workspace
                std::string relativePath = entry.path().string();
                if (relativePath.find(workspacePath_) == 0) {
                    relativePath = relativePath.substr(workspacePath_.length());
                }
                files.push_back(relativePath);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while listing files: " << e.what() << std::endl;
    }
    
    return files;
}

std::string WorkspaceManager::readFile(const std::string& filename) const
{
    std::string fullPath = workspacePath_ + filename;
    
    // Check if the file exists
    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "Error: File does not exist: " << fullPath << std::endl;
        return "";
    }
    
    try {
        // Open the file for reading
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file for reading: " << fullPath << std::endl;
            return "";
        }
        
        // Read the entire file content
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        return buffer.str();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while reading file " << filename << ": " << e.what() << std::endl;
        return "";
    }
}

bool WorkspaceManager::ensureDirectoryExists(const std::string& path)
{
    try {
        if (!std::filesystem::exists(path)) {
            if (!std::filesystem::create_directories(path)) {
                std::cerr << "Error: Failed to create directory: " << path << std::endl;
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while creating directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

} // namespace ai_editor 