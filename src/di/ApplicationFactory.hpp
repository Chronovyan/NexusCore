#pragma once

#include <memory>
#include "Injector.hpp"
#include "../interfaces/IApplication.hpp"
#include "../Application.h"
#include "../AppDebugLog.h"

/**
 * @class ApplicationFactory
 * @brief Factory for creating and configuring Application instances
 * 
 * This factory is responsible for creating Application instances with the appropriate
 * dependencies and configuration.
 */
class ApplicationFactory {
public:
    /**
     * @brief Create a new Application instance with dependencies resolved from the injector
     * 
     * @param injector The dependency injector
     * @return A shared pointer to an Application instance, as an IApplication
     */
    static std::shared_ptr<IApplication> create(di::Injector& injector) {
        LOG_DEBUG("Creating new Application instance");
        
        // Create a new Application instance
        auto application = std::make_shared<Application>();
        
        // In the future, this could register the application in the injector
        // or configure it with additional dependencies
        
        LOG_DEBUG("Application instance created successfully");
        return application;
    }
}; 