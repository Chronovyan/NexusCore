#ifndef ICOMMAND_HPP
#define ICOMMAND_HPP

#include <string>

/**
 * @brief Interface for commands that can be registered and executed.
 * 
 * This interface defines the contract for commands that can be registered
 * with the command registry and executed by the editor.
 */
class ICommand {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~ICommand() = default;
    
    /**
     * @brief Execute the command.
     * 
     * This method is called when the command is executed.
     */
    virtual void execute() = 0;
    
    /**
     * @brief Get the display name of the command.
     * 
     * @return The human-readable name of the command.
     */
    virtual std::string getDisplayName() const = 0;
};

#endif // ICOMMAND_HPP 