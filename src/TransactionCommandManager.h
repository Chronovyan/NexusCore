#ifndef TRANSACTION_COMMAND_MANAGER_H
#define TRANSACTION_COMMAND_MANAGER_H

#include "CommandManager.h"
#include "Command.h"
#include "AppDebugLog.h"
#include <string>
#include <memory>
#include <stack>
#include <unordered_map>

/**
 * @class TransactionCommandManager
 * @brief Extended command manager with support for transaction grouping
 *
 * This class extends the CommandManager to provide transaction grouping functionality.
 * Transactions allow grouping multiple commands into a single undoable/redoable unit.
 * Transactions can be nested, and the manager keeps track of the active transaction stack.
 */
class TransactionCommandManager : public CommandManager {
public:
    /**
     * @brief Constructor
     */
    TransactionCommandManager() : CommandManager(), isInTransaction_(false) {
        LOG_DEBUG("TransactionCommandManager created");
    }

    /**
     * @brief Destructor
     */
    ~TransactionCommandManager() override {
        // If we have an active transaction when destroyed, commit it to prevent command loss
        if (isInTransaction_) {
            LOG_WARNING("TransactionCommandManager destroyed with active transaction - auto-committing");
            endTransaction();
        }
    }

    /**
     * @brief Begin a new transaction
     * 
     * @param name Optional name for the transaction (used in descriptions)
     * @return True if successfully started, false otherwise
     */
    bool beginTransaction(const std::string& name = "") {
        // Create a new transaction compound command
        auto transaction = std::make_unique<CompoundCommand>();
        
        // Store the name if provided
        if (!name.empty()) {
            transactionNames_[transaction.get()] = name;
        }
        
        // Push this transaction onto the stack
        transactionStack_.push(std::move(transaction));
        
        // Mark that we're in a transaction
        isInTransaction_ = true;
        
        LOG_DEBUG("Transaction started" + (name.empty() ? "" : ": " + name));
        return true;
    }

    /**
     * @brief End the current transaction and commit it
     * 
     * @return True if transaction was successfully committed, false if no active transaction
     */
    bool endTransaction() {
        if (!isInTransaction_ || transactionStack_.empty()) {
            LOG_WARNING("Attempted to end transaction when none is active");
            return false;
        }

        // Get the current transaction
        auto transaction = std::move(transactionStack_.top());
        transactionStack_.pop();

        // Get the transaction name if it exists
        std::string transactionName;
        auto nameIter = transactionNames_.find(transaction.get());
        if (nameIter != transactionNames_.end()) {
            transactionName = nameIter->second;
            transactionNames_.erase(nameIter);
        }

        // If this transaction is empty, just discard it
        if (transaction->isEmpty()) {
            LOG_DEBUG("Empty transaction discarded" + (transactionName.empty() ? "" : ": " + transactionName));
            
            // If this was the last transaction, mark that we're no longer in a transaction
            if (transactionStack_.empty()) {
                isInTransaction_ = false;
            }
            
            return true;
        }

        // If we're inside another transaction, add this transaction to the parent
        if (!transactionStack_.empty()) {
            transactionStack_.top()->addCommand(std::move(transaction));
            LOG_DEBUG("Nested transaction committed" + (transactionName.empty() ? "" : ": " + transactionName));
        } else {
            // This is the root transaction, add it to the command manager
            LOG_DEBUG("Root transaction committed" + (transactionName.empty() ? "" : ": " + transactionName));
            CommandManager::addCommand(std::move(transaction));
            isInTransaction_ = false;
        }

        return true;
    }

    /**
     * @brief Cancel the current transaction without committing it
     * 
     * @return True if transaction was successfully canceled, false if no active transaction
     */
    bool cancelTransaction() {
        if (!isInTransaction_ || transactionStack_.empty()) {
            LOG_WARNING("Attempted to cancel transaction when none is active");
            return false;
        }

        // Get the transaction name if it exists
        std::string transactionName;
        auto nameIter = transactionNames_.find(transactionStack_.top().get());
        if (nameIter != transactionNames_.end()) {
            transactionName = nameIter->second;
            transactionNames_.erase(nameIter);
        }

        // Discard the current transaction
        transactionStack_.pop();
        LOG_DEBUG("Transaction canceled" + (transactionName.empty() ? "" : ": " + transactionName));

        // If this was the last transaction, mark that we're no longer in a transaction
        if (transactionStack_.empty()) {
            isInTransaction_ = false;
        }

        return true;
    }

    /**
     * @brief Check if a transaction is currently active
     * 
     * @return True if a transaction is active
     */
    bool isInTransaction() const {
        return isInTransaction_;
    }

    /**
     * @brief Get the current transaction depth (number of nested transactions)
     * 
     * @return The transaction depth (0 if no active transaction)
     */
    size_t getTransactionDepth() const {
        return transactionStack_.size();
    }

    /**
     * @brief Execute a command and add it to the current transaction or undo stack
     * 
     * Overrides CommandManager::executeCommand to add the command to the current
     * transaction if one is active, otherwise behaves normally.
     * 
     * @param command The command to execute
     * @param editor The editor context for the command
     */
    void executeCommand(CommandPtr command, Editor& editor) override {
        // Execute the command
        command->execute(editor);

        // If we're in a transaction, add it to the current transaction
        if (isInTransaction_ && !transactionStack_.empty()) {
            transactionStack_.top()->addCommand(std::move(command));
        } else {
            // Otherwise, use the normal command manager behavior
            CommandManager::addCommand(std::move(command));
        }
    }

    /**
     * @brief Add a command without executing it
     * 
     * Overrides CommandManager::addCommand to add the command to the current
     * transaction if one is active, otherwise behaves normally.
     * 
     * @param command The command to add
     */
    void addCommand(CommandPtr command) override {
        // If we're in a transaction, add it to the current transaction
        if (isInTransaction_ && !transactionStack_.empty()) {
            transactionStack_.top()->addCommand(std::move(command));
        } else {
            // Otherwise, use the normal command manager behavior
            CommandManager::addCommand(std::move(command));
        }
    }

private:
    bool isInTransaction_;
    std::stack<std::unique_ptr<CompoundCommand>> transactionStack_;
    std::unordered_map<Command*, std::string> transactionNames_;
};

#endif // TRANSACTION_COMMAND_MANAGER_H 