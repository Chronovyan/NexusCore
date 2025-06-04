#pragma once

#include <memory>
#include "BasicPlugin.hpp"
#include "../plugins/PluginManager.hpp"
#include "../AppDebugLog.h"

namespace ai_editor {
namespace examples {

/**
 * @brief Factory for creating and registering BasicPlugin instances
 */
class BasicPluginFactory {
public:
    /**
     * @brief Register the BasicPlugin with the plugin manager
     * 
     * @param pluginManager The plugin manager instance
     * @return true if registration was successful, false otherwise
     */
    static bool registerPlugin(std::shared_ptr<PluginManager> pluginManager) {
        if (!pluginManager) {
            LOG_ERROR("Failed to register BasicPlugin: PluginManager is null");
            return false;
        }
        
        LOG_INFO("Registering BasicPlugin with the PluginManager");
        auto plugin = std::make_shared<BasicPlugin>();
        
        bool result = pluginManager->registerPlugin(plugin);
        
        if (result) {
            LOG_INFO("BasicPlugin registered successfully");
        } else {
            LOG_ERROR("Failed to register BasicPlugin");
        }
        
        return result;
    }
};

} // namespace examples
} // namespace ai_editor 