#ifndef AUTO_TRANSACTION_MANAGER_H
#define AUTO_TRANSACTION_MANAGER_H

#include "TransactionCommandManager.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <typeindex>
#include <typeinfo>
#include <unordered_set>

/**
 * @class AutoTransactionManager
 * @brief Command manager that automatically groups related commands into transactions
 *
 * This class extends TransactionCommandManager to provide automatic transaction grouping
 * based on timing, command type, and other heuristics. It's designed to create a more
 * natural undo/redo experience without requiring explicit transaction management.
 */
class AutoTransactionManager : public TransactionCommandManager {
public:
    /**
     * @brief Constructor
     * 
     * @param groupingTimeThresholdMs Time threshold in milliseconds for auto-grouping (default: 1000ms)
     */
    AutoTransactionManager(unsigned int groupingTimeThresholdMs = 1000)
        : TransactionCommandManager(), 
          groupingTimeThresholdMs_(groupingTimeThresholdMs),
          lastCommandTime_(),
          autoTransactionActive_(false) {
        LOG_DEBUG("AutoTransactionManager created with " + std::to_string(groupingTimeThresholdMs) + "ms threshold");
        
        // Register command types that should always be grouped with the previous command
        alwaysGroupWithPrevious_ = {
            std::type_index(typeid(InsertCharCommand)), // Character insertions
            std::type_index(typeid(DeleteCharCommand)), // Character deletions
            std::type_index(typeid(DeleteCharForwardCommand)) // Forward character deletions
        };
    }
    
    /**
     * @brief Destructor
     */
    ~AutoTransactionManager() override {
        // End any active auto-transaction
        if (autoTransactionActive_) {
            LOG_DEBUG("AutoTransactionManager: Ending auto-transaction on destruction");
            TransactionCommandManager::endTransaction();
        }
    }
    
    /**
     * @brief Execute a command with automatic transaction grouping
     * 
     * This method overrides TransactionCommandManager::executeCommand to provide
     * automatic transaction grouping based on timing and command type.
     * 
     * @param command The command to execute
     * @param editor The editor context for the command
     */
    void executeCommand(CommandPtr command, Editor& editor) override {
        auto now = std::chrono::steady_clock::now();
        
        // Get the type of the command
        std::type_index commandType = std::type_index(typeid(*command));
        
        // Decide if we should start a new auto-transaction
        bool shouldStartNewTransaction = false;
        bool shouldEndCurrentTransaction = false;
        
        if (!autoTransactionActive_) {
            // Start a new transaction if none is active
            shouldStartNewTransaction = true;
        } else {
            // Check if we should end the current transaction based on time threshold
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastCommandTime_).count();
                
            // End the current transaction if the time threshold has been exceeded,
            // unless this command type should always be grouped with the previous command
            if (elapsed > groupingTimeThresholdMs_ && 
                alwaysGroupWithPrevious_.find(commandType) == alwaysGroupWithPrevious_.end()) {
                shouldEndCurrentTransaction = true;
                shouldStartNewTransaction = true;
            }
        }
        
        // Handle transaction management
        if (shouldEndCurrentTransaction && autoTransactionActive_) {
            LOG_DEBUG("AutoTransactionManager: Ending auto-transaction due to time threshold");
            TransactionCommandManager::endTransaction();
            autoTransactionActive_ = false;
        }
        
        if (shouldStartNewTransaction) {
            LOG_DEBUG("AutoTransactionManager: Starting new auto-transaction");
            TransactionCommandManager::beginTransaction("Auto Transaction");
            autoTransactionActive_ = true;
        }
        
        // Execute the command and add it to the current transaction
        TransactionCommandManager::executeCommand(std::move(command), editor);
        
        // Update the last command time
        lastCommandTime_ = now;
    }
    
    /**
     * @brief Force the end of the current auto-transaction
     * 
     * This method can be called to explicitly end the current auto-transaction,
     * which is useful when a logical group of commands has been completed.
     * 
     * @return True if an auto-transaction was ended, false otherwise
     */
    bool forceEndAutoTransaction() {
        if (autoTransactionActive_) {
            LOG_DEBUG("AutoTransactionManager: Forcing end of auto-transaction");
            TransactionCommandManager::endTransaction();
            autoTransactionActive_ = false;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Register a command type to always be grouped with the previous command
     * 
     * This method can be used to register command types that should always be grouped
     * with the previous command, regardless of timing.
     * 
     * @param commandType The type index of the command
     */
    void registerAlwaysGroupWithPrevious(const std::type_index& commandType) {
        alwaysGroupWithPrevious_.insert(commandType);
    }
    
    /**
     * @brief Set the time threshold for auto-grouping
     * 
     * @param thresholdMs Time threshold in milliseconds
     */
    void setGroupingTimeThreshold(unsigned int thresholdMs) {
        groupingTimeThresholdMs_ = thresholdMs;
        LOG_DEBUG("AutoTransactionManager: Grouping time threshold set to " + std::to_string(thresholdMs) + "ms");
    }
    
    /**
     * @brief Get the current time threshold for auto-grouping
     * 
     * @return Time threshold in milliseconds
     */
    unsigned int getGroupingTimeThreshold() const {
        return groupingTimeThresholdMs_;
    }
    
    /**
     * @brief Check if an auto-transaction is currently active
     * 
     * @return True if an auto-transaction is active
     */
    bool isAutoTransactionActive() const {
        return autoTransactionActive_;
    }
    
    /**
     * @brief Override begin transaction to handle potential auto-transactions
     * 
     * If an auto-transaction is active, it will be ended before starting a new manual transaction.
     * 
     * @param name Optional name for the transaction
     * @return True if successfully started, false otherwise
     */
    bool beginTransaction(const std::string& name = "") override {
        // End any active auto-transaction first
        if (autoTransactionActive_) {
            LOG_DEBUG("AutoTransactionManager: Ending auto-transaction before manual transaction");
            TransactionCommandManager::endTransaction();
            autoTransactionActive_ = false;
        }
        
        // Start a new manual transaction
        return TransactionCommandManager::beginTransaction(name);
    }

private:
    unsigned int groupingTimeThresholdMs_;
    std::chrono::steady_clock::time_point lastCommandTime_;
    bool autoTransactionActive_;
    std::unordered_set<std::type_index> alwaysGroupWithPrevious_;
};

#endif // AUTO_TRANSACTION_MANAGER_H 