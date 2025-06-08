#pragma once

#include <memory>
#include <string>

// Forward declarations
class Command;
class Editor;
using CommandPtr = std::unique_ptr<Command>;

/**
 * @interface ICommandManager
 * @brief Interface for command manager components
 * 
 * This interface defines the contract for command manager implementations,
 * providing methods for executing, undoing, and redoing commands.
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
     * @param editor The editor context for the command
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
     * @param editor The editor context for the command
     * @return True if a command was undone, false if no commands to undo
     */
    virtual bool undo(Editor& editor) = 0;
    
    /**
     * @brief Redo the most recently undone command
     * 
     * @param editor The editor context for the command
     * @return True if a command was redone, false if no commands to redo
     */
    virtual bool redo(Editor& editor) = 0;
    
    /**
     * @brief Check if there are commands available to undo
     * 
     * @return True if there are commands to undo
     */
    virtual bool canUndo() const = 0;
    
    /**
     * @brief Check if there are commands available to redo
     * 
     * @return True if there are commands to redo
     */
    virtual bool canRedo() const = 0;
    
    /**
     * @brief Get the number of commands in the undo stack
     * 
     * @return The size of the undo stack
     */
    virtual size_t undoStackSize() const = 0;
    
    /**
     * @brief Get the number of commands in the redo stack
     * 
     * @return The size of the redo stack
     */
    virtual size_t redoStackSize() const = 0;
    
    /**
     * @brief Clear both undo and redo stacks
     */
    virtual void clear() = 0;
    
    /**
     * @brief Begin a new transaction
     * 
     * Starts a new transaction group that will collect all commands executed until
     * endTransaction is called. The entire group will be treated as a single
     * undoable/redoable unit.
     *
     * @param name Optional name for the transaction (used in descriptions)
     * @return True if successfully started, false otherwise
     */
    virtual bool beginTransaction(const std::string& name = "") = 0;
    
    /**
     * @brief End the current transaction and commit it
     * 
     * Ends the current transaction and commits it to the undo stack as a single unit.
     *
     * @return True if transaction was successfully committed, false if no active transaction
     */
    virtual bool endTransaction() = 0;
    
    /**
     * @brief Cancel the current transaction without committing it
     * 
     * Cancels the current transaction and discards all commands within it.
     *
     * @return True if transaction was successfully canceled, false if no active transaction
     */
    virtual bool cancelTransaction() = 0;
    
    /**
     * @brief Check if a transaction is currently active
     * 
     * @return True if a transaction is active
     */
    virtual bool isInTransaction() const = 0;
    
    /**
     * @brief Get the current transaction depth (number of nested transactions)
     * 
     * @return The transaction depth (0 if no active transaction)
     */
    virtual size_t getTransactionDepth() const = 0;
}; 