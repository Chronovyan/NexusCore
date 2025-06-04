#pragma once

#include <memory>
#include <string>
#include "../interfaces/plugins/IPlugin.hpp"
#include "../interfaces/plugins/ICommand.hpp"
#include "../interfaces/IEditorServices.hpp"
#include "../AppDebugLog.h"

namespace ai_editor {
namespace examples {

/**
 * @brief A basic command that logs a message when executed
 */
class BasicCommand : public ICommand {
public:
    BasicCommand(const std::string& id, const std::string& name, const std::string& description)
        : id_(id), name_(name), description_(description) {}
    
    ~BasicCommand() override {
        LOG_DEBUG("BasicCommand destroyed: " + id_);
    }
    
    std::string getId() const override {
        return id_;
    }
    
    std::string getName() const override {
        return name_;
    }
    
    std::string getDescription() const override {
        return description_;
    }
    
    bool isEnabled() const override {
        return true;
    }
    
    void execute() override {
        LOG_INFO("Executing basic command: " + id_);
    }
    
private:
    std::string id_;
    std::string name_;
    std::string description_;
};

/**
 * @brief A basic plugin that demonstrates the plugin architecture
 */
class BasicPlugin : public IPlugin {
public:
    BasicPlugin() {
        LOG_DEBUG("BasicPlugin constructed");
    }
    
    ~BasicPlugin() override {
        LOG_DEBUG("BasicPlugin destroyed");
    }
    
    std::string getId() const override {
        return "com.ai-editor.basic-plugin";
    }
    
    std::string getName() const override {
        return "Basic Plugin";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "A basic plugin that demonstrates the plugin architecture";
    }
    
    bool initialize(std::shared_ptr<IEditorServices> services) override {
        LOG_INFO("Initializing BasicPlugin");
        
        if (!services) {
            LOG_ERROR("Failed to initialize BasicPlugin: EditorServices is null");
            return false;
        }
        
        services_ = services;
        
        // Register commands
        auto commandRegistry = services_->getCommandRegistry();
        if (commandRegistry) {
            auto helloCommand = std::make_shared<BasicCommand>(
                "basic-plugin.hello",
                "Hello",
                "Prints a hello message to the log"
            );
            
            commandRegistry->registerCommand(helloCommand);
            LOG_INFO("Registered 'Hello' command");
            
            auto worldCommand = std::make_shared<BasicCommand>(
                "basic-plugin.world",
                "World",
                "Prints a world message to the log"
            );
            
            commandRegistry->registerCommand(worldCommand);
            LOG_INFO("Registered 'World' command");
        } else {
            LOG_ERROR("Failed to register commands: CommandRegistry is null");
        }
        
        // Register a menu item in the Plugins menu
        auto uiRegistry = services_->getUIExtensionRegistry();
        if (uiRegistry) {
            uiRegistry->addMenuItem(
                "plugins", // Menu ID (plugins menu)
                "basic-plugin.hello", // Command ID
                "Hello Command", // Display text
                "Ctrl+Alt+H" // Shortcut
            );
            
            uiRegistry->addMenuItem(
                "plugins", // Menu ID (plugins menu)
                "basic-plugin.world", // Command ID
                "World Command", // Display text
                "Ctrl+Alt+W" // Shortcut
            );
            
            LOG_INFO("Registered menu items");
        } else {
            LOG_ERROR("Failed to register menu items: UIExtensionRegistry is null");
        }
        
        LOG_INFO("BasicPlugin initialized successfully");
        return true;
    }
    
    void shutdown() override {
        LOG_INFO("Shutting down BasicPlugin");
        
        if (services_) {
            // Unregister commands
            auto commandRegistry = services_->getCommandRegistry();
            if (commandRegistry) {
                commandRegistry->unregisterCommand("basic-plugin.hello");
                commandRegistry->unregisterCommand("basic-plugin.world");
                LOG_INFO("Unregistered commands");
            }
            
            // Unregister menu items
            auto uiRegistry = services_->getUIExtensionRegistry();
            if (uiRegistry) {
                uiRegistry->removeMenuItem("plugins", "basic-plugin.hello");
                uiRegistry->removeMenuItem("plugins", "basic-plugin.world");
                LOG_INFO("Unregistered menu items");
            }
        }
        
        services_.reset();
        LOG_INFO("BasicPlugin shutdown complete");
    }
    
private:
    std::shared_ptr<IEditorServices> services_;
};

} // namespace examples
} // namespace ai_editor 