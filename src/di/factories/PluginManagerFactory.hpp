#pragma once

#include <memory>
#include "../../plugins/PluginManager.hpp"
#include "../../interfaces/IEditorServices.hpp"
#include "../../plugins/examples/BasicPluginFactory.hpp"
#include "../../AppDebugLog.h"

namespace ai_editor {

/**
 * @brief Factory for creating PluginManager instances
 * 
 * This factory creates and initializes PluginManager instances, providing them
 * with the required IEditorServices dependency.
 */
class PluginManagerFactory {
public:
    /**
     * @brief Create a new PluginManager instance
     * 
     * @param editorServices The editor services instance
     * @return A shared pointer to the created PluginManager
     */
    static std::shared_ptr<PluginManager> createPluginManager(
        std::shared_ptr<IEditorServices> editorServices) {
        
        LOG_DEBUG("Creating PluginManager instance");
        
        if (!editorServices) {
            LOG_ERROR("Failed to create PluginManager: EditorServices is null");
            return nullptr;
        }
        
        auto pluginManager = std::make_shared<PluginManager>(editorServices);
        
        // Register built-in plugins
        registerBuiltInPlugins(pluginManager);
        
        return pluginManager;
    }
    
    /**
     * @brief Register the PluginManager with the DIFramework
     * 
     * @param framework The DIFramework instance to register with
     */
    static void registerPluginManager(DIFramework& framework) {
        LOG_DEBUG("Registering PluginManager with DIFramework");
        framework.registerFactory<PluginManager>([&framework]() {
            auto services = framework.get<IEditorServices>();
            return createPluginManager(services);
        }, lifetime::ServiceLifetime::Singleton);
    }

private:
    /**
     * @brief Register built-in plugins with the plugin manager
     * 
     * @param pluginManager The plugin manager instance
     */
    static void registerBuiltInPlugins(std::shared_ptr<PluginManager> pluginManager) {
        LOG_DEBUG("Registering built-in plugins");
        
        // Register example plugins
        examples::BasicPluginFactory::registerPlugin(pluginManager);
    }
};

} // namespace ai_editor 