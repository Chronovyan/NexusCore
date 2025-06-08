#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include "Injector.hpp"
#include "../interfaces/IWorkspaceManager.hpp"
#include "../WorkspaceManager.h"
#include "../interfaces/IErrorReporter.hpp"
#include "../AppDebugLog.h"

// Adapter class to make WorkspaceManager implement IWorkspaceManager
class WorkspaceManagerAdapter : public IWorkspaceManager {
private:
    std::shared_ptr<ai_editor::WorkspaceManager> manager_;

public:
    explicit WorkspaceManagerAdapter(const std::string& workspacePath)
        : manager_(std::make_shared<ai_editor::WorkspaceManager>(workspacePath)) {}
    
    bool writeFile(const std::string& filename, const std::string& content) override {
        return manager_->writeFile(filename, content);
    }
    
    bool fileExists(const std::string& filename) const override {
        return manager_->fileExists(filename);
    }
    
    std::vector<std::string> listFiles() const override {
        return manager_->listFiles();
    }
    
    std::string readFile(const std::string& filename) const override {
        return manager_->readFile(filename);
    }
    
    std::string getWorkspacePath() const override {
        return manager_->getWorkspacePath();
    }
    
    bool createDirectory(const std::string& dirname) override {
        // Delegate to filesystem
        try {
            return std::filesystem::create_directories(manager_->getWorkspacePath() + "/" + dirname);
        } catch (...) {
            return false;
        }
    }
    
    bool deleteFile(const std::string& filename) override {
        try {
            return std::filesystem::remove(manager_->getWorkspacePath() + "/" + filename);
        } catch (...) {
            return false;
        }
    }
    
    bool renameFile(const std::string& oldFilename, const std::string& newFilename) override {
        try {
            std::filesystem::rename(
                manager_->getWorkspacePath() + "/" + oldFilename,
                manager_->getWorkspacePath() + "/" + newFilename
            );
            return true;
        } catch (...) {
            return false;
        }
    }
};

/**
 * @class WorkspaceManagerFactory
 * @brief Factory for creating and configuring WorkspaceManager instances
 * 
 * This factory is responsible for creating WorkspaceManager instances with
 * the appropriate configuration.
 */
class WorkspaceManagerFactory {
public:
    /**
     * @brief Create a new WorkspaceManager instance
     * 
     * @param injector The dependency injector
     * @return A shared pointer to a WorkspaceManager instance
     */
    static std::shared_ptr<IWorkspaceManager> create(di::Injector& injector) {
        LOG_DEBUG("Creating new WorkspaceManager instance");
        
        // Get the error reporter from the injector
        auto errorReporter = injector.resolve<IErrorReporter>();
        
        // Determine the workspace path
        std::string workspacePath = determineWorkspacePath();
        
        // Create the workspace directory if it doesn't exist
        std::filesystem::create_directories(workspacePath);
        
        // Create a new WorkspaceManager instance with adapter
        auto workspaceManager = std::make_shared<WorkspaceManagerAdapter>(workspacePath);
        
        LOG_DEBUG("WorkspaceManager instance created for workspace: %s", workspacePath.c_str());
        return workspaceManager;
    }

private:
    /**
     * @brief Determine the workspace path based on environment variables or default location
     * 
     * @return The workspace path to use
     */
    static std::string determineWorkspacePath() {
        // Try to get the workspace path from the environment
        const char* envWorkspace = std::getenv("AI_EDITOR_WORKSPACE");
        if (envWorkspace && strlen(envWorkspace) > 0) {
            return envWorkspace;
        }
        
        // Use a default workspace path
        return "./workspace";
    }
}; 