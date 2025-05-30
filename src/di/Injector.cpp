#include "Injector.hpp"
#include "AppDebugLog.h"

namespace di {

Injector::Injector() {
    LOG_DEBUG("Injector created");
}

Injector::~Injector() {
    LOG_DEBUG("Injector destroyed");
}

void Injector::clearFactories() {
    LOG_DEBUG("Clearing all registered factories");
    factories_.clear();
    LOG_DEBUG("All factories cleared");
}

} // namespace di 