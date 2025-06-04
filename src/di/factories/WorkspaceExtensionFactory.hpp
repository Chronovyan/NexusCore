#pragma once

#include "di/DIFramework.hpp"
#include "plugins/WorkspaceExtension.hpp"
#include "interfaces/plugins/IWorkspaceExtension.hpp"
#include "AppDebugLog.h"

namespace ai_editor {
namespace di {

/**
 * @brief Factory for creating and registering WorkspaceExtension instances.
 */
class WorkspaceExtensionFactory {
public:
    /**
     * @brief Register the WorkspaceExtension with the DI framework.
     * 
     * @param framework The DI framework instance.
     */
    static void registerServices(DIFramework& framework) {
        LOG_DEBUG("Registering WorkspaceExtension factory");
        
        framework.registerSingletonFactory<IWorkspaceExtension>(
            []() -> std::shared_ptr<IWorkspaceExtension> {
                LOG_DEBUG("Creating WorkspaceExtension instance");
                return std::make_shared<WorkspaceExtension>();
            }
        );
    }
};

} // namespace di
} // namespace ai_editor 