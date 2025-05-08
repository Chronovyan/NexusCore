#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "Command.h"
#include <vector>
#include <memory>
#include <stack>

class Editor;

// CommandManager class - Manages command execution and undo/redo history
class CommandManager {
public:
    CommandManager() = default;
    
    // Execute a command and store it in the undo stack
    void executeCommand(CommandPtr command, Editor& editor) {
        command->execute(editor);
        undoStack_.push(std::move(command));
        
        // Clear the redo stack when a new command is executed
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
    }
    
    // Add a command to the undo stack without executing it
    void addCommand(CommandPtr command) {
        undoStack_.push(std::move(command));
        
        // Clear the redo stack when a new command is added
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
    }
    
    // Undo the most recent command
    bool undo(Editor& editor) {
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
    bool redo(Editor& editor) {
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
    bool canUndo() const {
        return !undoStack_.empty();
    }
    
    // Check if there are commands available to redo
    bool canRedo() const {
        return !redoStack_.empty();
    }
    
    // Get the number of commands in the undo stack
    size_t undoStackSize() const {
        return undoStack_.size();
    }
    
    // Get the number of commands in the redo stack
    size_t redoStackSize() const {
        return redoStack_.size();
    }
    
    // Clear both undo and redo stacks
    void clear() {
        while (!undoStack_.empty()) {
            undoStack_.pop();
        }
        
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
    }
    
private:
    std::stack<CommandPtr> undoStack_;
    std::stack<CommandPtr> redoStack_;
};

#endif // COMMAND_MANAGER_H 