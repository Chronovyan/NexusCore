#include "ModuleManager.hpp"

namespace di {

ModuleManager::ModuleManager() {
    LOG_DEBUG("ModuleManager created");
}

void ModuleManager::registerModule(ModuleConfigFunction configFunc) {
    LOG_DEBUG("Registering module");
    modules_.push_back(configFunc);
    LOG_DEBUG("Module registered successfully");
}

void ModuleManager::configureAll(Injector& injector) {
    LOG_DEBUG("Configuring all modules (%zu total)", modules_.size());
    
    for (auto& moduleConfig : modules_) {
        moduleConfig(injector);
    }
    
    LOG_DEBUG("All modules configured successfully");
}

} // namespace di 