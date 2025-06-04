#pragma once

#include "Injector.hpp"
#include "../interfaces/IDiffEngine.hpp"
#include "../interfaces/IMergeEngine.hpp"
#include "../diff/DiffMergeFactory.h"
#include "AppDebugLog.h"
#include <iostream>

namespace di {

/**
 * @brief Configures the dependency injection container for diff and merge services
 * 
 * This module registers the diff and merge services with the DI container.
 */
class DiffMergeModule {
public:
    /**
     * @brief Configure the diff and merge services
     * 
     * This method registers all diff and merge related services with the injector.
     * 
     * @param injector The injector to configure
     */
    static void configure(Injector& injector) {
        LOG_DEBUG("Configuring DiffMergeModule...");
        
        // Register IDiffEngine implementation
        injector.registerFactory<IDiffEngine>([]() {
            LOG_DEBUG("Creating new DiffEngine");
            return DiffMergeFactory::createDiffEngine();
        });
        
        // Register IMergeEngine implementation
        injector.registerFactory<IMergeEngine>([](Injector& inj) {
            LOG_DEBUG("Creating new MergeEngine");
            auto diffEngine = inj.get<IDiffEngine>();
            return DiffMergeFactory::createMergeEngine(diffEngine);
        });
        
        LOG_DEBUG("DiffMergeModule configured successfully");
    }
};

} // namespace di 