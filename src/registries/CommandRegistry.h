#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include "../interfaces/plugins/ICommandRegistry.hpp"
#include "../interfaces/plugins/ICommand.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

/**
 * @brief Implementation of the ICommandRegistry interface.
 * 
 * This class manages the registration and execution of commands that can be
 * used by plugins and the core editor.
 */
class CommandRegistry : public ICommandRegistry {
public:
    /**
     * @brief Constructor
     */
    CommandRegistry();
    
    /**
     * @brief Destructor
     */
    ~CommandRegistry() override = default;
    
    /**
     * @brief Register a new command with the editor.
     * 
     * @param commandId A unique identifier for the command.
     * @param command A shared pointer to the ICommand implementation.
     * @return true if registration succeeded, false if a command with the same ID already exists.
     */
    bool registerCommand(const std::string& commandId, 
                       std::shared_ptr<ICommand> command) override;
                       
    /**
     * @brief Unregister a command from the editor.
     * 
     * @param commandId The identifier of the command to unregister.
     * @return true if the command was found and unregistered, false otherwise.
     */
    bool unregisterCommand(const std::string& commandId) override;
    
    /**
     * @brief Check if a command with the specified ID exists.
     * 
     * @param commandId The identifier of the command to check.
     * @return true if the command exists, false otherwise.
     */
    bool hasCommand(const std::string& commandId) const override;
    
    /**
     * @brief Get a command by its ID.
     * 
     * @param commandId The identifier of the command to retrieve.
     * @return A shared pointer to the command, or nullptr if not found.
     */
    std::shared_ptr<ICommand> getCommand(const std::string& commandId) const override;
    
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
                           std::function<void()> func) override;
                           
    /**
     * @brief Execute a command by its ID.
     * 
     * @param commandId The identifier of the command to execute.
     * @return true if the command was found and executed, false otherwise.
     */
    bool executeCommand(const std::string& commandId) override;
    
    /**
     * @brief Get all registered command IDs.
     * 
     * @return A vector of all registered command IDs.
     */
    std::vector<std::string> getAllCommandIds() const override;
    
private:
    // Map of command IDs to command instances
    std::unordered_map<std::string, std::shared_ptr<ICommand>> commands_;
};

/**
 * @brief Simple command implementation that wraps a function.
 */
class FunctionCommand : public ICommand {
public:
    /**
     * @brief Constructor
     * 
     * @param displayName A human-readable name for the command.
     * @param func The function to execute when the command is invoked.
     */
    FunctionCommand(const std::string& displayName, std::function<void()> func)
        : displayName_(displayName), func_(func) {}
    
    /**
     * @brief Execute the command.
     */
    void execute() override {
        if (func_) {
            func_();
        }
    }
    
    /**
     * @brief Get the display name of the command.
     * 
     * @return The display name.
     */
    std::string getDisplayName() const override {
        return displayName_;
    }
    
private:
    std::string displayName_;
    std::function<void()> func_;
};

#endif // COMMAND_REGISTRY_H 