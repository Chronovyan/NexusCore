#pragma once

#include <memory>
#include <string>
#include <atomic>
#include "interfaces/IApplication.hpp"
#include "interfaces/IEditor.hpp"
#include "di/Injector.hpp"
#include "di/ModuleManager.hpp"

/**
 * @class Application
 * @brief Main application class
 * 
 * This class implements the IApplication interface and serves as the
 * main entry point for the application. It initializes the DI container,
 * creates and configures all the necessary components, and runs the
 * application.
 */
class Application : public IApplication {
public:
    /**
     * @brief Constructor
     */
    Application();
    
    /**
     * @brief Destructor
     */
    ~Application() override;
    
    /**
     * @brief Initialize the application
     * 
     * @param argc Number of command line arguments
     * @param argv Array of command line arguments
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize(int argc, char** argv) override;
    
    /**
     * @brief Run the application
     * 
     * @return Exit code (0 for success, non-zero for error)
     */
    int run() override;
    
    /**
     * @brief Shut down the application
     */
    void shutdown() override;
    
    /**
     * @brief Check if the application is running
     * 
     * @return True if the application is running, false otherwise
     */
    bool isRunning() const override { return running_; }
    
    /**
     * @brief Get the application name
     * 
     * @return The application name
     */
    std::string getName() const override { return "AI-First TextEditor"; }
    
    /**
     * @brief Get the application version
     * 
     * @return The application version
     */
    std::string getVersion() const override { return "1.0.0"; }
    
    /**
     * @brief Get the DI container
     * 
     * @return Reference to the DI container
     */
    di::Injector& getInjector() { return injector_; }
    
    /**
     * @brief Get the editor instance
     * 
     * @return Shared pointer to the editor
     */
    std::shared_ptr<IEditor> getEditor() { return editor_; }
    
private:
    /**
     * @brief Configure the DI container
     */
    void configureContainer();
    
    /**
     * @brief Create the main editor instance
     */
    void createEditor();
    
    /**
     * @brief Process command line arguments
     * 
     * @param argc Number of command line arguments
     * @param argv Array of command line arguments
     * @return True if processing succeeded, false otherwise
     */
    bool processCommandLineArgs(int argc, char** argv);
    
    // Core application services
    di::Injector injector_;
    di::ModuleManager moduleManager_;
    std::shared_ptr<IEditor> editor_;
    
    // Application state
    std::atomic<bool> running_;
    
    // Command line arguments
    std::string initialFilename_;
}; 