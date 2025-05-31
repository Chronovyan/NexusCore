#include "Application.h"
#include "AppDebugLog.h"
#include "di/Injector.hpp"
#include "di/CoreModule.hpp"
#include "interfaces/IApplication.hpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        // Initialize logging
        initAppDebugLog();
        LOG_DEBUG("Application starting");
        
        // Create the DI container
        di::Injector injector;
        
        // Configure the container with core components
        CoreModule::configure(injector);
        
        // Resolve the application instance
        auto app = injector.resolve<IApplication>();
        
        // Initialize the application
        if (!app->initialize(argc, argv)) {
            LOG_ERROR("Failed to initialize application");
            return 1;
        }
        
        // Run the application
        int result = app->run();
        
        LOG_DEBUG("Application exited with code %d", result);
        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        LOG_ERROR("Unhandled exception: %s", e.what());
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        LOG_ERROR("Unknown exception occurred");
        return -1;
    }
} 