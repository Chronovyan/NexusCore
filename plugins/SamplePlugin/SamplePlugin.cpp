#include "SamplePlugin.h"
#include "../../src/interfaces/plugins/PluginAPI.hpp"
#include "../../src/interfaces/plugins/ICommandRegistry.hpp"
#include "../../src/interfaces/plugins/IUIExtensionRegistry.hpp"
#include "../../src/interfaces/IErrorReporter.hpp"
#include <iostream>
#include <functional>

// Use the macro to implement the plugin creation function
IMPLEMENT_PLUGIN(SamplePlugin)

SamplePlugin::SamplePlugin() 
    : commandId_("sample.helloWorld"),
      menuId_("plugins"),
      menuItemId_("plugins.helloWorld") {
    // Nothing to do here
}

std::string SamplePlugin::getName() const {
    return "SamplePlugin";
}

std::string SamplePlugin::getVersion() const {
    return "1.0.0";
}

std::string SamplePlugin::getDescription() const {
    return "A simple sample plugin that demonstrates basic functionality.";
}

bool SamplePlugin::initialize(std::shared_ptr<IEditorServices> services) {
    if (!services) {
        std::cerr << "SamplePlugin: Null IEditorServices provided to initialize()" << std::endl;
        return false;
    }
    
    // Store the services for later use
    services_ = services;
    
    try {
        // Get the command registry
        auto commandRegistry = services->getCommandRegistry();
        if (!commandRegistry) {
            services->getErrorReporter()->reportError("SamplePlugin: Failed to get ICommandRegistry");
            return false;
        }
        
        // Register the hello world command
        if (!commandRegistry->registerCommand(
            commandId_,
            "Hello World",
            "Display a hello world message",
            std::bind(&SamplePlugin::executeHelloWorldCommand, this)
        )) {
            services->getErrorReporter()->reportError("SamplePlugin: Failed to register command: " + commandId_);
            return false;
        }
        
        // Get the UI extension registry
        auto uiRegistry = services->getUIExtensionRegistry();
        if (!uiRegistry) {
            services->getErrorReporter()->reportError("SamplePlugin: Failed to get IUIExtensionRegistry");
            commandRegistry->unregisterCommand(commandId_);
            return false;
        }
        
        // Create a "Plugins" menu if it doesn't exist
        if (!uiRegistry->menuExists(menuId_)) {
            if (!uiRegistry->createMenu(menuId_, "Plugins")) {
                services->getErrorReporter()->reportError("SamplePlugin: Failed to create Plugins menu");
                commandRegistry->unregisterCommand(commandId_);
                return false;
            }
        }
        
        // Add a menu item that executes our command
        MenuItem menuItem;
        menuItem.id = menuItemId_;
        menuItem.label = "Hello World";
        menuItem.parentMenuId = menuId_;
        menuItem.commandId = commandId_;
        menuItem.shortcutKey = "Ctrl+Alt+H";
        
        if (!uiRegistry->addMenuItem(menuItem)) {
            services->getErrorReporter()->reportError("SamplePlugin: Failed to add menu item: " + menuItemId_);
            commandRegistry->unregisterCommand(commandId_);
            return false;
        }
        
        std::cout << "SamplePlugin: Successfully initialized" << std::endl;
        
        // Try to get the event registry and publish an initialization event
        auto eventRegistry = services->getEventRegistry();
        if (eventRegistry) {
            struct PluginLoadedEvent {
                std::string pluginName;
                std::string pluginVersion;
            };
            
            PluginLoadedEvent event{getName(), getVersion()};
            eventRegistry->publish("plugin.loaded", event);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        if (services && services->getErrorReporter()) {
            services->getErrorReporter()->reportError("SamplePlugin: Exception during initialization: " + std::string(e.what()));
        }
        else {
            std::cerr << "SamplePlugin: Exception during initialization: " << e.what() << std::endl;
        }
        return false;
    }
}

void SamplePlugin::shutdown() {
    if (!services_) {
        std::cerr << "SamplePlugin: Null IEditorServices during shutdown()" << std::endl;
        return;
    }
    
    try {
        // Unregister the menu item
        auto uiRegistry = services_->getUIExtensionRegistry();
        if (uiRegistry) {
            uiRegistry->removeMenuItem(menuItemId_);
        }
        
        // Unregister the command
        auto commandRegistry = services_->getCommandRegistry();
        if (commandRegistry) {
            commandRegistry->unregisterCommand(commandId_);
        }
        
        // Publish an event that the plugin is unloading
        auto eventRegistry = services_->getEventRegistry();
        if (eventRegistry) {
            struct PluginUnloadedEvent {
                std::string pluginName;
            };
            
            PluginUnloadedEvent event{getName()};
            eventRegistry->publish("plugin.unloaded", event);
        }
        
        std::cout << "SamplePlugin: Successfully shut down" << std::endl;
    }
    catch (const std::exception& e) {
        if (services_ && services_->getErrorReporter()) {
            services_->getErrorReporter()->reportError("SamplePlugin: Exception during shutdown: " + std::string(e.what()));
        }
        else {
            std::cerr << "SamplePlugin: Exception during shutdown: " << e.what() << std::endl;
        }
    }
    
    // Clear the services pointer
    services_.reset();
}

void SamplePlugin::executeHelloWorldCommand() {
    if (!services_) {
        std::cerr << "SamplePlugin: Null IEditorServices during command execution" << std::endl;
        return;
    }
    
    // Get the error reporter to display a message
    auto errorReporter = services_->getErrorReporter();
    if (errorReporter) {
        errorReporter->reportInfo("Hello, World! This message is from the SamplePlugin.");
    }
    else {
        std::cout << "Hello, World! This message is from the SamplePlugin." << std::endl;
    }
    
    // Get the event registry and publish an event for the command execution
    auto eventRegistry = services_->getEventRegistry();
    if (eventRegistry) {
        struct CommandExecutedEvent {
            std::string commandId;
            std::string pluginName;
        };
        
        CommandExecutedEvent event{commandId_, getName()};
        eventRegistry->publish("command.executed", event);
    }
} 