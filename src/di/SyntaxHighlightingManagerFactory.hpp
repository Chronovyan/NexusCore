#pragma once

#include <memory>
#include "Injector.hpp"
#include "../interfaces/ISyntaxHighlightingManager.hpp"
#include "../SyntaxHighlightingManager.h"
#include "../interfaces/IErrorReporter.hpp"
#include "../AppDebugLog.h"

/**
 * @class SyntaxHighlightingManagerFactory
 * @brief Factory for creating and configuring SyntaxHighlightingManager instances
 * 
 * This factory is responsible for creating SyntaxHighlightingManager instances with
 * the appropriate dependencies and configuration.
 */
class SyntaxHighlightingManagerFactory {
public:
    /**
     * @brief Create a new SyntaxHighlightingManager instance
     * 
     * @param injector The dependency injector
     * @return A shared pointer to a SyntaxHighlightingManager instance
     */
    static std::shared_ptr<ISyntaxHighlightingManager> create(di::Injector& injector) {
        LOG_DEBUG("Creating new SyntaxHighlightingManager instance");
        
        // Get the error reporter from the injector
        auto errorReporter = injector.resolve<IErrorReporter>();
        
        // Create a new SyntaxHighlightingManager instance
        auto manager = std::make_shared<SyntaxHighlightingManager>();
        
        // Configure the manager
        manager->setEnabled(true);
        manager->setHighlightingTimeout(50); // 50ms timeout for responsive UI
        manager->setContextLines(50); // 50 lines of context
        
        LOG_DEBUG("SyntaxHighlightingManager instance created and configured successfully");
        return manager;
    }
}; 