#ifndef IPLUGIN_HPP
#define IPLUGIN_HPP

#include <string>
#include <memory>
#include "../IEditorServices.hpp"

/**
 * @brief Interface that defines the contract for all plugins in the AI-First TextEditor.
 * 
 * Plugins must implement this interface to be loaded and managed by the editor.
 * The interface provides methods for initialization, identification, and lifecycle management.
 */
class IPlugin {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IPlugin() = default;
    
    /**
     * @brief Get the name of the plugin.
     * 
     * This name should be unique among all plugins and is used for identification.
     * 
     * @return The name of the plugin.
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get the version of the plugin.
     * 
     * Version should follow semantic versioning (e.g., "1.0.0").
     * 
     * @return The version string of the plugin.
     */
    virtual std::string getVersion() const = 0;
    
    /**
     * @brief Get a human-readable description of the plugin.
     * 
     * This description should explain what the plugin does and may include
     * information about its author, license, etc.
     * 
     * @return The description of the plugin.
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Initialize the plugin with editor services.
     * 
     * This method is called when the plugin is loaded. The plugin should
     * use this opportunity to register its components, commands, UI elements, etc.
     * with the editor services. If initialization fails, the plugin should
     * return false.
     * 
     * @param services A shared pointer to the editor services interface.
     * @return true if initialization succeeded, false otherwise.
     */
    virtual bool initialize(std::shared_ptr<IEditorServices> services) = 0;
    
    /**
     * @brief Shut down the plugin.
     * 
     * This method is called when the plugin is being unloaded. The plugin should
     * clean up any resources it has allocated and unregister any components
     * it has registered with the editor.
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if the plugin is compatible with the current editor version.
     * 
     * This method allows the plugin manager to verify that a plugin will work
     * with the current version of the editor before loading it.
     * 
     * @param editorVersion The version of the editor.
     * @return true if the plugin is compatible, false otherwise.
     */
    virtual bool isCompatible(const std::string& editorVersion) const { 
        return true; // Default implementation assumes compatibility
    }
    
    /**
     * @brief Get the loading priority of the plugin.
     * 
     * Higher values mean the plugin loads earlier. This can be used to ensure
     * that plugins with dependencies on other plugins are loaded in the correct order.
     * 
     * @return The loading priority value (default is 0).
     */
    virtual int getPriority() const { return 0; }
};

#endif // IPLUGIN_HPP 