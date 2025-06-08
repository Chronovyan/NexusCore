#pragma once

#include "di/DIFramework.hpp"
#include "plugins/CommandRegistry.hpp"
#include "interfaces/plugins/ICommandRegistry.hpp"
#include "AppDebugLog.h"

namespace ai_editor {
namespace di {

/**
 * @brief Factory for creating and registering CommandRegistry instances.
 */
class CommandRegistryFactory {
public:
    /**
     * @brief Register the CommandRegistry with the DI framework.
     * 
     * @param framework The DI framework instance.
     */
    static void registerServices(DIFramework& framework) {
        LOG_DEBUG("Registering CommandRegistry factory");
        
        framework.registerSingletonFactory<ICommandRegistry>(
            []() -> std::shared_ptr<ICommandRegistry> {
                LOG_DEBUG("Creating CommandRegistry instance");
                return std::make_shared<CommandRegistry>();
            }
        );
    }
};

} // namespace di
} // namespace ai_editor 