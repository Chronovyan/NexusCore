#ifndef ICOMMAND_REGISTRY_HPP
#define ICOMMAND_REGISTRY_HPP

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include "ICommand.hpp"

/**
 * @brief Interface for registering and managing commands in the editor.
 * 
 * Plugins can use this interface to register custom commands that can be
 * executed by the editor, bound to keyboard shortcuts, or added to menus
 * and toolbars.
 */
class ICommandRegistry {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~ICommandRegistry() = default;
    
    /**
     * @brief Register a new command with the editor.
     * 
     * @param commandId A unique identifier for the command.
     * @param command A shared pointer to the ICommand implementation.
     * @return true if registration succeeded, false if a command with the same ID already exists.
     */
    virtual bool registerCommand(const std::string& commandId, 
                               std::shared_ptr<ICommand> command) = 0;
                               
    /**
     * @brief Unregister a command from the editor.
     * 
     * @param commandId The identifier of the command to unregister.
     * @return true if the command was found and unregistered, false otherwise.
     */
    virtual bool unregisterCommand(const std::string& commandId) = 0;
    
    /**
     * @brief Check if a command with the specified ID exists.
     * 
     * @param commandId The identifier of the command to check.
     * @return true if the command exists, false otherwise.
     */
    virtual bool hasCommand(const std::string& commandId) const = 0;
    
    /**
     * @brief Get a command by its ID.
     * 
     * @param commandId The identifier of the command to retrieve.
     * @return A shared pointer to the command, or nullptr if not found.
     */
    virtual std::shared_ptr<ICommand> getCommand(const std::string& commandId) const = 0;
    
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
    virtual bool registerCommandFunc(const std::string& commandId,
                                   const std::string& displayName,
                                   std::function<void()> func) = 0;
                                   
    /**
     * @brief Execute a command by its ID.
     * 
     * @param commandId The identifier of the command to execute.
     * @return true if the command was found and executed, false otherwise.
     */
    virtual bool executeCommand(const std::string& commandId) = 0;
    
    /**
     * @brief Get all registered command IDs.
     * 
     * @return A vector of all registered command IDs.
     */
    virtual std::vector<std::string> getAllCommandIds() const = 0;
};

#endif // ICOMMAND_REGISTRY_HPP 