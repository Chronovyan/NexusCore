#pragma once

#include "di/DIFramework.hpp"
#include "plugins/EventRegistry.hpp"
#include "interfaces/plugins/IEventRegistry.hpp"
#include "AppDebugLog.h"

namespace ai_editor {
namespace di {

/**
 * @brief Factory for creating and registering EventRegistry instances.
 */
class EventRegistryFactory {
public:
    /**
     * @brief Register the EventRegistry with the DI framework.
     * 
     * @param framework The DI framework instance.
     */
    static void registerServices(DIFramework& framework) {
        LOG_DEBUG("Registering EventRegistry factory");
        
        framework.registerSingletonFactory<IEventRegistry>(
            []() -> std::shared_ptr<IEventRegistry> {
                LOG_DEBUG("Creating EventRegistry instance");
                return std::make_shared<EventRegistry>();
            }
        );
    }
};

} // namespace di
} // namespace ai_editor 