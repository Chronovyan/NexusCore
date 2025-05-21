#ifndef PLUGIN_MANAGER_HPP
#define PLUGIN_MANAGER_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "../interfaces/plugins/IPlugin.hpp"
#include "../interfaces/IEditorServices.hpp"

/**
 * @brief Manager class for discovering, loading, and managing plugins.
 */
class PluginManager {
public:
    /**
     * @brief Constructor that takes editor services to provide to plugins.
     * 
     * @param editorServices Shared pointer to the editor services interface.
     */
    explicit PluginManager(std::shared_ptr<IEditorServices> editorServices);
    
    /**
     * @brief Destructor that ensures all plugins are properly unloaded.
     */
    ~PluginManager();
    
    /**
     * @brief Discover and load all plugins from the specified directory.
     * 
     * This method scans the directory for plugin libraries (DLLs, SOs) and
     * attempts to load each one.
     * 
     * @param pluginsDirectory The directory to search for plugins.
     * @return The number of plugins successfully loaded.
     */
    size_t loadPlugins(const std::string& pluginsDirectory);
    
    /**
     * @brief Load a specific plugin from a file.
     * 
     * @param pluginPath The path to the plugin library file.
     * @return true if the plugin was loaded successfully, false otherwise.
     */
    bool loadPlugin(const std::string& pluginPath);
    
    /**
     * @brief Unload a specific plugin by its ID.
     * 
     * @param pluginId The unique identifier of the plugin to unload.
     * @return true if the plugin was found and unloaded, false otherwise.
     */
    bool unloadPlugin(const std::string& pluginId);
    
    /**
     * @brief Unload all loaded plugins.
     * 
     * @return The number of plugins successfully unloaded.
     */
    size_t unloadAllPlugins();
    
    /**
     * @brief Get a plugin by its ID.
     * 
     * @param pluginId The unique identifier of the plugin to retrieve.
     * @return A shared pointer to the plugin, or nullptr if not found.
     */
    std::shared_ptr<IPlugin> getPlugin(const std::string& pluginId) const;
    
    /**
     * @brief Get all loaded plugins.
     * 
     * @return A vector of all loaded plugins.
     */
    std::vector<std::shared_ptr<IPlugin>> getLoadedPlugins() const;
    
    /**
     * @brief Check if a plugin with the specified ID is loaded.
     * 
     * @param pluginId The unique identifier of the plugin to check.
     * @return true if the plugin is loaded, false otherwise.
     */
    bool isPluginLoaded(const std::string& pluginId) const;
    
    /**
     * @brief Get the number of loaded plugins.
     * 
     * @return The number of loaded plugins.
     */
    size_t getPluginCount() const;
    
private:
    /**
     * @brief Handle for a dynamically loaded library.
     */
    struct LibraryHandle {
        void* handle = nullptr;  ///< Platform-specific library handle
        std::string path;        ///< Path to the library file
    };
    
    /**
     * @brief Load a shared library and return its handle.
     * 
     * @param libraryPath The path to the library file.
     * @return A LibraryHandle structure containing the handle and path.
     */
    LibraryHandle loadLibrary(const std::string& libraryPath);
    
    /**
     * @brief Unload a shared library.
     * 
     * @param handle The LibraryHandle structure for the library to unload.
     * @return true if the library was unloaded successfully, false otherwise.
     */
    bool unloadLibrary(const LibraryHandle& handle);
    
    /**
     * @brief Create a plugin instance from a loaded library.
     * 
     * @param handle The LibraryHandle structure for the library containing the plugin.
     * @return A shared pointer to the plugin instance, or nullptr if creation failed.
     */
    std::shared_ptr<IPlugin> createPluginInstance(const LibraryHandle& handle);
    
    std::shared_ptr<IEditorServices> editorServices_;  ///< Editor services to provide to plugins
    std::unordered_map<std::string, std::shared_ptr<IPlugin>> plugins_;  ///< Map of plugin IDs to plugin instances
    std::unordered_map<std::string, LibraryHandle> libraryHandles_;  ///< Map of plugin IDs to library handles
};

#endif // PLUGIN_MANAGER_HPP 