#ifndef PLUGIN_API_HPP
#define PLUGIN_API_HPP

#include "IPlugin.hpp"
#include <memory>

/**
 * @brief Function signature for plugin creation.
 *
 * This function is exported by all plugin libraries and called by the PluginManager
 * to create an instance of the plugin.
 *
 * @return A shared pointer to a newly created IPlugin instance.
 */
typedef std::shared_ptr<IPlugin> (*CreatePluginFunc)();

/**
 * @brief Define the plugin API function to be exported by all plugins.
 *
 * This macro helps plugin authors properly export the required function.
 * Each plugin must define this function to create and return an instance of their plugin.
 */
#ifdef _WIN32
    // Windows platforms
    #define PLUGIN_API extern "C" __declspec(dllexport)
#else
    // Unix-like platforms
    #define PLUGIN_API extern "C" __attribute__((visibility("default")))
#endif

/**
 * @brief The name of the function that all plugins must export.
 *
 * This function will be looked up by the PluginManager using dlsym/GetProcAddress.
 */
#define PLUGIN_CREATE_FUNCTION_NAME "createPlugin"

/**
 * @brief Helper macro for implementing the plugin creation function.
 *
 * Plugin authors should use this macro to properly implement the creation function.
 *
 * @param PluginClass The class name of the plugin implementation.
 */
#define IMPLEMENT_PLUGIN(PluginClass) \
    PLUGIN_API std::shared_ptr<IPlugin> createPlugin() { \
        return std::make_shared<PluginClass>(); \
    }

#endif // PLUGIN_API_HPP 