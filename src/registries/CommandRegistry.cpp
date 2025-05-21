#include "CommandRegistry.h"
#include "../AppDebugLog.h"
#include <algorithm>
#include <iostream>

CommandRegistry::CommandRegistry() 
    : commands_() {
    // Initialize with any built-in commands if needed
    LOG_DEBUG("CommandRegistry initialized");
}

bool CommandRegistry::registerCommand(const std::string& commandId, 
                                   std::shared_ptr<ICommand> command) {
    // Check if command ID already exists
    if (commands_.find(commandId) != commands_.end()) {
        std::cerr << "CommandRegistry: Command ID '" << commandId 
                  << "' already registered." << std::endl;
        return false;
    }
    
    // Check for null command
    if (!command) {
        std::cerr << "CommandRegistry: Cannot register null command for ID '" 
                  << commandId << "'." << std::endl;
        return false;
    }
    
    // Register the command
    commands_[commandId] = command;
    return true;
}

bool CommandRegistry::unregisterCommand(const std::string& commandId) {
    auto it = commands_.find(commandId);
    if (it == commands_.end()) {
        std::cerr << "CommandRegistry: Command ID '" << commandId 
                  << "' not found for unregistration." << std::endl;
        return false;
    }
    
    commands_.erase(it);
    return true;
}

bool CommandRegistry::hasCommand(const std::string& commandId) const {
    return commands_.find(commandId) != commands_.end();
}

std::shared_ptr<ICommand> CommandRegistry::getCommand(const std::string& commandId) const {
    auto it = commands_.find(commandId);
    if (it == commands_.end()) {
        return nullptr;
    }
    
    return it->second;
}

bool CommandRegistry::registerCommandFunc(const std::string& commandId,
                                       const std::string& displayName,
                                       std::function<void()> func) {
    // Create a FunctionCommand wrapper
    auto command = std::make_shared<FunctionCommand>(displayName, func);
    
    // Register it
    return registerCommand(commandId, command);
}

bool CommandRegistry::executeCommand(const std::string& commandId) {
    auto command = getCommand(commandId);
    if (!command) {
        std::cerr << "CommandRegistry: Cannot execute command '" << commandId 
                  << "', not found." << std::endl;
        return false;
    }
    
    try {
        command->execute();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CommandRegistry: Exception while executing command '" 
                  << commandId << "': " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "CommandRegistry: Unknown exception while executing command '" 
                  << commandId << "'." << std::endl;
        return false;
    }
}

std::vector<std::string> CommandRegistry::getAllCommandIds() const {
    std::vector<std::string> ids;
    ids.reserve(commands_.size());
    
    for (const auto& pair : commands_) {
        ids.push_back(pair.first);
    }
    
    // Sort the IDs for consistent ordering
    std::sort(ids.begin(), ids.end());
    
    return ids;
} 