#pragma once

#include "di/DIFramework.hpp"
#include "plugins/SyntaxHighlightingRegistry.hpp"
#include "interfaces/plugins/ISyntaxHighlightingRegistry.hpp"
#include "AppDebugLog.h"

namespace ai_editor {
namespace di {

/**
 * @brief Factory for creating and registering SyntaxHighlightingRegistry instances.
 */
class SyntaxHighlightingRegistryFactory {
public:
    /**
     * @brief Register the SyntaxHighlightingRegistry with the DI framework.
     * 
     * @param framework The DI framework instance.
     */
    static void registerServices(DIFramework& framework) {
        LOG_DEBUG("Registering SyntaxHighlightingRegistry factory");
        
        framework.registerSingletonFactory<ISyntaxHighlightingRegistry>(
            []() -> std::shared_ptr<ISyntaxHighlightingRegistry> {
                LOG_DEBUG("Creating SyntaxHighlightingRegistry instance");
                return std::make_shared<SyntaxHighlightingRegistry>();
            }
        );
    }
};

} // namespace di
} // namespace ai_editor 