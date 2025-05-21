#pragma once

#include "../../src/interfaces/plugins/IPlugin.hpp"
#include <string>
#include <memory>

/**
 * @class SamplePlugin
 * @brief A simple plugin that demonstrates basic functionality.
 * 
 * This plugin demonstrates how to:
 * - Register commands with the CommandRegistry
 * - Add menu items with the UIExtensionRegistry
 * - Handle initialization and shutdown
 */
class SamplePlugin : public IPlugin {
public:
    /**
     * @brief Constructor.
     */
    SamplePlugin();
    
    /**
     * @brief Destructor.
     */
    ~SamplePlugin() override = default;
    
    /**
     * @brief Get the name of the plugin.
     * 
     * @return The name of the plugin.
     */
    std::string getName() const override;
    
    /**
     * @brief Get the version of the plugin.
     * 
     * @return The version string of the plugin.
     */
    std::string getVersion() const override;
    
    /**
     * @brief Get a human-readable description of the plugin.
     * 
     * @return The description of the plugin.
     */
    std::string getDescription() const override;
    
    /**
     * @brief Initialize the plugin with editor services.
     * 
     * This method is called when the plugin is loaded. It registers
     * commands and UI elements.
     * 
     * @param services A shared pointer to the editor services interface.
     * @return true if initialization succeeded, false otherwise.
     */
    bool initialize(std::shared_ptr<IEditorServices> services) override;
    
    /**
     * @brief Shut down the plugin.
     * 
     * This method is called when the plugin is being unloaded. It
     * unregisters commands and UI elements.
     */
    void shutdown() override;
    
private:
    /**
     * @brief Execute the hello world command.
     * 
     * This is the callback function for the sample.helloWorld command.
     */
    void executeHelloWorldCommand();
    
    std::shared_ptr<IEditorServices> services_;  // Store the editor services for use in commands
    
    // IDs of registered UI elements for cleanup
    std::string commandId_;
    std::string menuId_;
    std::string menuItemId_;
}; 