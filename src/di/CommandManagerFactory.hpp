#pragma once

#include "di/Injector.h"
#include "interfaces/ICommandManager.hpp"
#include "CommandManager.h"
#include "TransactionCommandManager.h"
#include "AutoTransactionManager.h"
#include <memory>

/**
 * @brief Factory for creating and registering command manager components
 * 
 * This class provides factory methods to register command manager implementations
 * with the dependency injection system.
 */
class CommandManagerFactory {
public:
    /**
     * @brief Register command manager components with the DI system
     * 
     * This method registers the following:
     * 1. Default command manager (AutoTransactionManager)
     * 2. Transaction command manager (explicit transactions)
     * 3. Basic command manager without transaction support
     * 
     * @param injector The DI injector to register components with
     */
    static void registerComponents(di::Injector& injector) {
        // Register the default command manager (with auto-transaction support)
        injector.registerFactory<ICommandManager>([](const di::Injector& inj) {
            return std::make_shared<AutoTransactionManager>();
        }, di::Lifetime::Singleton);
        
        // Register the transaction command manager (explicit transactions)
        injector.registerFactory<ICommandManager>("transaction", [](const di::Injector& inj) {
            return std::make_shared<TransactionCommandManager>();
        }, di::Lifetime::Singleton);
        
        // Register the basic command manager (without transaction support)
        injector.registerFactory<ICommandManager>("basic", [](const di::Injector& inj) {
            return std::make_shared<CommandManager>();
        }, di::Lifetime::Singleton);
        
        // Register the auto-transaction manager with custom settings
        injector.registerFactory<ICommandManager>("auto_transaction", [](const di::Injector& inj) {
            // We could get configuration from the injector if needed
            return std::make_shared<AutoTransactionManager>(500); // 500ms grouping threshold
        }, di::Lifetime::Singleton);
    }
}; 