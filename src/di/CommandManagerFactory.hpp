#pragma once

#include <memory>
#include "Injector.hpp"
#include "../CommandManager.h"
#include "../interfaces/ICommandManager.hpp"
#include "../AppDebugLog.h"

/**
 * @class CommandManagerFactory
 * @brief Factory for creating and configuring CommandManager instances
 * 
 * This factory is responsible for creating CommandManager instances with the appropriate
 * configuration and returning them as ICommandManager interface implementations.
 */
class CommandManagerFactory {
public:
    /**
     * @brief Create a new CommandManager instance
     * 
     * @param injector The dependency injector
     * @return A shared pointer to a CommandManager instance, as an ICommandManager
     */
    static std::shared_ptr<ICommandManager> create(di::Injector& injector) {
        // Currently CommandManager doesn't have dependencies, but if it did,
        // we would resolve them from the injector here
        
        // Create a new CommandManager instance
        auto commandManager = std::make_shared<CommandManager>();
        
        // Additional configuration could be done here
        
        LOG_DEBUG("Created new CommandManager instance");
        
        return commandManager;
    }
}; 