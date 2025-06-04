#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "../interfaces/plugins/ICommandRegistry.hpp"
#include "../interfaces/plugins/ICommand.hpp"
#include "../AppDebugLog.h"

/**
 * @brief Implementation of the ICommandRegistry interface.
 * 
 * This class manages the registration and execution of commands in the editor.
 */
class CommandRegistry : public ICommandRegistry {
public:
    /**
     * @brief Constructor.
     */
    CommandRegistry() {
        LOG_INFO("CommandRegistry initialized");
    }

    /**
     * @brief Destructor.
     */
    ~CommandRegistry() {
        LOG_INFO("CommandRegistry destroyed");
    }

    /**
     * @brief Register a new command with the editor.
     *
     * @param commandId A unique identifier for the command.
     * @param command A shared pointer to the ICommand implementation.
     * @return true if registration succeeded, false if a command with the same ID already exists.
     */
    bool registerCommand(const std::string& commandId, std::shared_ptr<ICommand> command) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (commands_.find(commandId) != commands_.end()) {
            LOG_WARNING("Command with ID '" + commandId + "' already exists");
            return false;
        }

        commands_[commandId] = command;
        LOG_INFO("Registered command: " + commandId + " (" + command->getDisplayName() + ")");
        return true;
    }

    /**
     * @brief Unregister a command from the editor.
     *
     * @param commandId The identifier of the command to unregister.
     * @return true if the command was found and unregistered, false otherwise.
     */
    bool unregisterCommand(const std::string& commandId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = commands_.find(commandId);
        if (it == commands_.end()) {
            LOG_WARNING("Command with ID '" + commandId + "' not found for unregistration");
            return false;
        }

        commands_.erase(it);
        LOG_INFO("Unregistered command: " + commandId);
        return true;
    }

    /**
     * @brief Check if a command with the specified ID exists.
     * 
     * @param commandId The identifier of the command to check.
     * @return true if the command exists, false otherwise.
     */
    bool hasCommand(const std::string& commandId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return commands_.find(commandId) != commands_.end();
    }

    /**
     * @brief Get a command by its ID.
     *
     * @param commandId The identifier of the command to retrieve.
     * @return A shared pointer to the command, or nullptr if not found.
     */
    std::shared_ptr<ICommand> getCommand(const std::string& commandId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = commands_.find(commandId);
        if (it == commands_.end()) {
            return nullptr;
        }
        
        return it->second;
    }
    
    /**
     * @brief Register a simple command with a function.
     *
     * This is a convenience method for registering commands that can be
     * represented as simple functions without any state.
     *
     * @param commandId A unique identifier for the command.
     * @param displayName A human-readable name for the command.
     * @param func The function to execute when the command is invoked.
     * @return true if registration succeeded, false if a command with the same ID already exists.
     */
    bool registerCommandFunc(const std::string& commandId, 
                           const std::string& displayName,
                           std::function<void()> func) override {
        // Create a FunctionCommand that wraps the function
        auto command = std::make_shared<FunctionCommand>(displayName, func);
        return registerCommand(commandId, command);
    }

    /**
     * @brief Execute a command by its ID.
     *
     * @param commandId The identifier of the command to execute.
     * @return true if the command was found and executed, false otherwise.
     */
    bool executeCommand(const std::string& commandId) override {
        std::shared_ptr<ICommand> command;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = commands_.find(commandId);
            if (it == commands_.end()) {
                LOG_WARNING("Command with ID '" + commandId + "' not found for execution");
                return false;
            }
            command = it->second;
        }
        
        try {
            command->execute();
            LOG_DEBUG("Executed command: " + commandId);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Error executing command '" + commandId + "': " + e.what());
            return false;
        }
    }

    /**
     * @brief Get all registered command IDs.
     * 
     * @return A vector of all registered command IDs.
     */
    std::vector<std::string> getAllCommandIds() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> ids;
        ids.reserve(commands_.size());
        
        for (const auto& pair : commands_) {
            ids.push_back(pair.first);
        }
        
        return ids;
    }

private:
    /**
     * @brief Command implementation that wraps a function.
     */
    class FunctionCommand : public ICommand {
    public:
        FunctionCommand(const std::string& displayName, std::function<void()> func)
            : displayName_(displayName), func_(func) {}
        
        void execute() override {
            func_();
        }
        
        std::string getDisplayName() const override {
            return displayName_;
        }
        
    private:
        std::string displayName_;
        std::function<void()> func_;
    };

    std::unordered_map<std::string, std::shared_ptr<ICommand>> commands_;
    mutable std::mutex mutex_;  // Protects commands_
}; 