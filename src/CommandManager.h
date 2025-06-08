#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "Command.h"
#include "interfaces/ICommandManager.hpp"
#include <vector>
#include <memory>
#include <stack>
#include <string>

class Editor;

// CommandManager class - Manages command execution and undo/redo history
class CommandManager : public ICommandManager {
public:
    CommandManager() = default;
    
    // Execute a command and store it in the undo stack
    void executeCommand(CommandPtr command, Editor& editor) override {
        command->execute(editor);
        undoStack_.push(std::move(command));
        
        // Clear the redo stack when a new command is executed
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
    }
    
    // Add a command to the undo stack without executing it
    void addCommand(CommandPtr command) override {
        undoStack_.push(std::move(command));
        
        // Clear the redo stack when a new command is added
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
    }
    
    // Undo the most recent command
    bool undo(Editor& editor) override {
        if (undoStack_.empty()) {
            return false;
        }
        
        CommandPtr command = std::move(undoStack_.top());
        undoStack_.pop();
        
        command->undo(editor);
        redoStack_.push(std::move(command));
        
        return true;
    }
    
    // Redo the most recently undone command
    bool redo(Editor& editor) override {
        if (redoStack_.empty()) {
            return false;
        }
        
        CommandPtr command = std::move(redoStack_.top());
        redoStack_.pop();
        
        command->execute(editor);
        undoStack_.push(std::move(command));
        
        return true;
    }
    
    // Check if there are commands available to undo
    bool canUndo() const override {
        return !undoStack_.empty();
    }
    
    // Check if there are commands available to redo
    bool canRedo() const override {
        return !redoStack_.empty();
    }
    
    // Get the number of commands in the undo stack
    size_t undoStackSize() const override {
        return undoStack_.size();
    }
    
    // Get the number of commands in the redo stack
    size_t redoStackSize() const override {
        return redoStack_.size();
    }
    
    // Clear both undo and redo stacks
    void clear() override {
        while (!undoStack_.empty()) {
            undoStack_.pop();
        }
        
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
    }
    
    // Transaction methods (no-op implementations for basic CommandManager)
    
    // Begin a new transaction (no-op in base class)
    bool beginTransaction([[maybe_unused]] const std::string& name = "") override {
        // Not supported in basic CommandManager
        return false;
    }
    
    // End the current transaction (no-op in base class)
    bool endTransaction() override {
        // Not supported in basic CommandManager
        return false;
    }
    
    // Cancel the current transaction (no-op in base class)
    bool cancelTransaction() override {
        // Not supported in basic CommandManager
        return false;
    }
    
    // Check if a transaction is active (always false in base class)
    bool isInTransaction() const override {
        // Not supported in basic CommandManager
        return false;
    }
    
    // Get transaction depth (always 0 in base class)
    size_t getTransactionDepth() const override {
        // Not supported in basic CommandManager
        return 0;
    }

protected:
    std::stack<CommandPtr> undoStack_;
    std::stack<CommandPtr> redoStack_;
};

#endif // COMMAND_MANAGER_H 