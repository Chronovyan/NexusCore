/**
 * @brief Creates a new scope for request-scoped services
 * 
 * @return A new ServiceProvider for the scope
 */
std::shared_ptr<ServiceProvider> createScope() {
    LOG_DEBUG("Creating new scope from ServiceProvider");
    return std::make_shared<ServiceProvider>(*this);
} 