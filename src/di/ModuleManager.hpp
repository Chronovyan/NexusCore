#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "Injector.hpp"
#include "../AppDebugLog.h"

/**
 * @class ModuleManager
 * @brief Manages the configuration of application modules
 * 
 * The ModuleManager class provides a centralized mechanism for registering
 * and configuring application modules, ensuring they are initialized in 
 * the correct order.
 */
class ModuleManager {
public:
    /**
     * @brief Constructor
     */
    ModuleManager() {
        LOG_DEBUG("ModuleManager created");
    }
    
    /**
     * @brief Destructor
     */
    ~ModuleManager() = default;
    
    /**
     * @brief Register a module configuration function
     * 
     * @param configureFunc The function to call to configure the module
     */
    void registerModule(std::function<void(di::Injector&)> configureFunc) {
        moduleConfigurators_.push_back(std::move(configureFunc));
        LOG_DEBUG("Module registered with ModuleManager");
    }
    
    /**
     * @brief Configure all registered modules using the given injector
     * 
     * @param injector The DI container to configure
     */
    void configureAll(di::Injector& injector) {
        LOG_DEBUG("Configuring all modules");
        
        for (const auto& configurator : moduleConfigurators_) {
            configurator(injector);
        }
        
        LOG_DEBUG("All modules configured successfully");
    }
    
private:
    std::vector<std::function<void(di::Injector&)>> moduleConfigurators_;
}; 