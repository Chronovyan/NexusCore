#include "ModuleManager.hpp"
#include <algorithm>
#include <iostream>

namespace di {

ModuleManager::ModuleManager() {
    std::cout << "ModuleManager created" << std::endl;
}

void ModuleManager::registerModule(std::function<void(Injector&)> configureFunc, int priority) {
    if (!configureFunc) {
        throw std::invalid_argument("Configure function cannot be null");
    }
    
    ModuleConfigurator configurator;
    configurator.configureFunc = configureFunc;
    configurator.priority = priority;
    
    modules_.push_back(configurator);
    
    std::cout << "Module registered with priority " << priority << std::endl;
}

void ModuleManager::configureAll(Injector& injector) {
    // Sort modules by priority (higher priority first)
    std::sort(modules_.begin(), modules_.end(), 
        [](const ModuleConfigurator& a, const ModuleConfigurator& b) {
            return a.priority > b.priority;
        });
    
    // Configure each module
    for (const auto& configurator : modules_) {
        try {
            configurator.configureFunc(injector);
            std::cout << "Module with priority " << configurator.priority << " configured successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error configuring module: " << e.what() << std::endl;
            throw; // Re-throw to allow caller to handle
        }
    }
    
    std::cout << "All modules configured successfully" << std::endl;
}

} // namespace di 