#pragma once

#include <memory>
#include "Injector.hpp"
#include "../interfaces/IErrorReporter.hpp"
#include "../EditorErrorReporter.h"
#include "../AppDebugLog.h"

/**
 * @class ErrorReporterFactory
 * @brief Factory for creating and configuring ErrorReporter instances
 * 
 * This factory is responsible for creating ErrorReporter instances with
 * the appropriate configuration.
 */
class ErrorReporterFactory {
public:
    /**
     * @brief Create a new ErrorReporter instance
     * 
     * @param injector The dependency injector
     * @return A shared pointer to an ErrorReporter instance
     */
    static std::shared_ptr<IErrorReporter> create(di::Injector& injector) {
        LOG_DEBUG("Creating new ErrorReporter instance");
        
        // Create a new EditorErrorReporter instance
        auto errorReporter = std::make_shared<EditorErrorReporter>();
        
        // Initialize with default settings
        errorReporter->initializeDefaultLogging();
        
        // Enable async logging by default for better performance
        errorReporter->enableAsyncLogging(true);
        
        // Configure async queue with reasonable defaults
        errorReporter->configureAsyncQueue(
            1000, // Max queue size
            error_reporting::QueueOverflowPolicy::DropOldest
        );
        
        LOG_DEBUG("ErrorReporter instance created and configured successfully");
        return errorReporter;
    }
}; 