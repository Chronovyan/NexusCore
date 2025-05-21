#pragma once

#include <memory>
#include <string>

// Forward declarations
class Editor;
class Command;

/**
 * @brief Alias for a command smart pointer
 */
using CommandPtr = std::unique_ptr<Command>;

/**
 * @interface ICommandManager
 * @brief Interface for command execution and management
 * 
 * Defines the contract for components that handle command pattern execution
 * with undo/redo capabilities.
 */
class ICommandManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~ICommandManager() = default;

    /**
     * @brief Execute a command and store it in the undo stack
     * 
     * @param command The command to execute
     * @param editor Reference to the editor
     */
    virtual void executeCommand(CommandPtr command, Editor& editor) = 0;
    
    /**
     * @brief Add a command to the undo stack without executing it
     * 
     * @param command The command to add
     */
    virtual void addCommand(CommandPtr command) = 0;
    
    /**
     * @brief Undo the most recent command
     * 
     * @param editor Reference to the editor
     * @return bool True if a command was undone, false if no command to undo
     */
    virtual bool undo(Editor& editor) = 0;
    
    /**
     * @brief Redo the most recently undone command
     * 
     * @param editor Reference to the editor
     * @return bool True if a command was redone, false if no command to redo
     */
    virtual bool redo(Editor& editor) = 0;
    
    /**
     * @brief Check if there are commands available to undo
     * 
     * @return bool True if there are commands to undo
     */
    virtual bool canUndo() const = 0;
    
    /**
     * @brief Check if there are commands available to redo
     * 
     * @return bool True if there are commands to redo
     */
    virtual bool canRedo() const = 0;
    
    /**
     * @brief Get the number of commands in the undo stack
     * 
     * @return size_t The undo stack size
     */
    virtual size_t undoStackSize() const = 0;
    
    /**
     * @brief Get the number of commands in the redo stack
     * 
     * @return size_t The redo stack size
     */
    virtual size_t redoStackSize() const = 0;
    
    /**
     * @brief Clear both undo and redo stacks
     */
    virtual void clear() = 0;
}; 