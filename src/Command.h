#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <memory>
#include <vector>

// Forward declaration to avoid circular dependency
class Editor;

// Abstract Command base class
class Command {
public:
    virtual ~Command() = default;
    
    // Execute the command
    virtual void execute(Editor& editor) = 0;
    
    // Undo the command
    virtual void undo(Editor& editor) = 0;
    
    // Get a description of the command (for potential logging/UI)
    virtual std::string getDescription() const = 0;
};

// Type alias for smart pointer to Command
using CommandPtr = std::unique_ptr<Command>;

// CompoundCommand - Groups multiple commands together as a single operation
class CompoundCommand : public Command {
public:
    CompoundCommand() = default;
    
    void addCommand(CommandPtr command) {
        commands_.push_back(std::move(command));
    }
    
    void execute(Editor& editor) override {
        for (auto& command : commands_) {
            command->execute(editor);
        }
    }
    
    void undo(Editor& editor) override {
        // Undo commands in reverse order
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->undo(editor);
        }
    }
    
    std::string getDescription() const override {
        return "Compound operation (" + std::to_string(commands_.size()) + " steps)";
    }
    
    bool isEmpty() const {
        return commands_.empty();
    }
    
private:
    std::vector<CommandPtr> commands_;
};

#endif // COMMAND_H 