#pragma once

#include <vector>
#include <functional>
#include "Injector.hpp"

namespace di {

/**
 * ModuleManager manages the registration and configuration of modules in the DI container.
 * Modules are responsible for registering their services with the container.
 */
class ModuleManager {
public:
    /**
     * @brief Constructor
     */
    ModuleManager();
    
    /**
     * @brief Destructor
     */
    ~ModuleManager() = default;
    
    /**
     * @brief Register a module configuration function with the manager.
     * 
     * @param configureFunc The function that configures the module
     * @param priority The priority of the module (higher priority modules are configured first)
     */
    void registerModule(std::function<void(Injector&)> configureFunc, int priority = 0);
    
    /**
     * @brief Configure all registered modules in priority order.
     * 
     * @param injector The injector to configure
     */
    void configureAll(Injector& injector);
    
private:
    /**
     * @brief Structure to hold a module configurator and its priority
     */
    struct ModuleConfigurator {
        std::function<void(Injector&)> configureFunc;
        int priority;
    };
    
    std::vector<ModuleConfigurator> modules_;
};

} // namespace di 