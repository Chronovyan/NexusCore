#ifndef NAMED_TRANSACTION_COMMAND_H
#define NAMED_TRANSACTION_COMMAND_H

#include "Command.h"
#include <string>
#include <memory>

/**
 * @class NamedTransactionCommand
 * @brief A decorator for CompoundCommand that provides a custom description
 *
 * This class wraps a CompoundCommand and provides a custom description for
 * the transaction, which is useful for providing more meaningful information
 * in UI elements or logs about what a transaction does.
 */
class NamedTransactionCommand : public Command {
public:
    /**
     * @brief Constructor
     * 
     * @param transaction The compound command to decorate
     * @param name The name/description for this transaction
     */
    NamedTransactionCommand(std::unique_ptr<CompoundCommand> transaction, const std::string& name)
        : transaction_(std::move(transaction)), name_(name) {}

    /**
     * @brief Execute the transaction
     * 
     * @param editor The editor context for the command
     */
    void execute(Editor& editor) override {
        transaction_->execute(editor);
    }

    /**
     * @brief Undo the transaction
     * 
     * @param editor The editor context for the command
     */
    void undo(Editor& editor) override {
        transaction_->undo(editor);
    }

    /**
     * @brief Get the custom description of the transaction
     * 
     * @return The transaction name/description
     */
    std::string getDescription() const override {
        return name_;
    }

    /**
     * @brief Check if the transaction is empty
     * 
     * @return True if the transaction contains no commands
     */
    bool isEmpty() const {
        return transaction_->isEmpty();
    }

    /**
     * @brief Add a command to the transaction
     * 
     * @param command The command to add
     */
    void addCommand(CommandPtr command) {
        transaction_->addCommand(std::move(command));
    }

private:
    std::unique_ptr<CompoundCommand> transaction_;
    std::string name_;
};

#endif // NAMED_TRANSACTION_COMMAND_H 