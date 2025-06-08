#include "resource_config.h"

namespace chronovyan {

// Empty implementation file - all the functionality is in the header
// This file is needed to ensure the singleton is properly initialized
// and to avoid the "static initialization order fiasco" 

// Force instantiation of the singleton
ResourceConfig& initializeResourceConfig() {
    return ResourceConfig::getInstance();
}

// This will be executed during program startup
static ResourceConfig& g_resourceConfig = initializeResourceConfig();

} // namespace chronovyan 