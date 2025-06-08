#include <iostream>
#include <memory>
#include <string>

#include "DIFramework.hpp"
#include "ServiceCollection.hpp"
#include "ComponentFactories.hpp"
#include "../interfaces/IApplication.hpp"
#include "../interfaces/IEditor.hpp"
#include "../AppDebugLog.h"

/**
 * @file DIApplicationExample.cpp
 * @brief Example of how to use the DI framework in the main application
 * 
 * This file shows how to configure the DI framework for the main application,
 * including registering all the necessary services and resolving them.
 */

int main() {
    try {
        LOG_INFO("Starting application with DIFramework");
        
        // Create a service collection
        di::ServiceCollection services;
        
        // Register all component factories
        di::ComponentFactories::registerAll(services);
        
        // Build the service provider
        auto serviceProvider = services.buildServiceProvider();
        
        // Log the successful creation of the service provider
        LOG_INFO("Service provider created successfully");
        
        // Resolve the application
        auto app = serviceProvider->get<IApplication>();
        
        // Log that we're about to start the application
        LOG_INFO("Application resolved successfully, starting the application");
        
        // Run the application
        app->run();
        
        // Log the successful completion of the application
        LOG_INFO("Application completed successfully");
        
        return 0;
    }
    catch (const std::exception& ex) {
        LOG_ERROR("Error: " + std::string(ex.what()));
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
} 