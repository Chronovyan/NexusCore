#pragma once

#include "di/DIFramework.hpp"
#include "plugins/UIExtensionRegistry.hpp"
#include "interfaces/plugins/IUIExtensionRegistry.hpp"
#include "AppDebugLog.h"

namespace ai_editor {
namespace di {

/**
 * @brief Factory for creating and registering UIExtensionRegistry instances.
 */
class UIExtensionRegistryFactory {
public:
    /**
     * @brief Register the UIExtensionRegistry with the DI framework.
     * 
     * @param framework The DI framework instance.
     */
    static void registerServices(DIFramework& framework) {
        LOG_DEBUG("Registering UIExtensionRegistry factory");
        
        framework.registerSingletonFactory<IUIExtensionRegistry>(
            []() -> std::shared_ptr<IUIExtensionRegistry> {
                LOG_DEBUG("Creating UIExtensionRegistry instance");
                return std::make_shared<UIExtensionRegistry>();
            }
        );
    }
};

} // namespace di
} // namespace ai_editor 