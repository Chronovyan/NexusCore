#include "Application.h"
#include "AppDebugLog.h"
#include "di/ApplicationModule.hpp"
#include <iostream>

Application::Application() : running_(false) {
    LOG_DEBUG("Application created");
}

Application::~Application() {
    if (running_) {
        shutdown();
    }
    
    LOG_DEBUG("Application destroyed");
}

bool Application::initialize(int argc, char** argv) {
    LOG_DEBUG("Initializing application");
    
    // Process command line arguments
    if (!processCommandLineArgs(argc, argv)) {
        LOG_ERROR("Failed to process command line arguments");
        return false;
    }
    
    // Configure the DI container
    configureContainer();
    
    // Create the editor
    createEditor();
    
    // If there's an initial filename, open it
    if (!initialFilename_.empty()) {
        if (!editor_->openFile(initialFilename_)) {
            LOG_ERROR("Failed to open initial file: %s", initialFilename_.c_str());
            // Continue anyway, don't treat this as a fatal error
        }
    }
    
    LOG_DEBUG("Application initialized successfully");
    return true;
}

int Application::run() {
    LOG_DEBUG("Running application");
    
    // Set the running flag
    running_ = true;
    
    // Main application loop would go here
    // For now, just print a message
    std::cout << "Application is running..." << std::endl;
    std::cout << "Editor is ready." << std::endl;
    
    // In a real application, we would run a UI event loop here
    
    // For the purposes of this example, we'll just shut down immediately
    shutdown();
    
    return 0;
}

void Application::shutdown() {
    LOG_DEBUG("Shutting down application");
    
    // Set the running flag to false
    running_ = false;
    
    // Clean up resources
    editor_.reset();
    
    LOG_DEBUG("Application shut down successfully");
}

void Application::configureContainer() {
    LOG_DEBUG("Configuring DI container");
    
    // Register the ApplicationModule
    moduleManager_.registerModule(ApplicationModule::configure);
    
    // Configure all registered modules
    moduleManager_.configureAll(injector_);
    
    LOG_DEBUG("DI container configured successfully");
}

void Application::createEditor() {
    LOG_DEBUG("Creating editor");
    
    // Create the editor from the DI container
    editor_ = injector_.resolve<IEditor>();
    
    LOG_DEBUG("Editor created successfully");
}

bool Application::processCommandLineArgs(int argc, char** argv) {
    LOG_DEBUG("Processing command line arguments");
    
    // If there's at least one argument, treat it as the filename to open
    if (argc > 1) {
        initialFilename_ = argv[1];
        LOG_DEBUG("Initial filename: %s", initialFilename_.c_str());
    }
    
    LOG_DEBUG("Command line arguments processed successfully");
    return true;
} 