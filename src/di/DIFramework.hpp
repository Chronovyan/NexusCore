#pragma once

#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <any>
#include <type_traits>

#include "Injector.hpp"
#include "LifetimeManager.hpp"
#include "AppDebugLog.h"

namespace di {

/**
 * @class DIFramework
 * @brief Central class for managing dependency injection in the application
 * 
 * The DIFramework class serves as the main entry point for the dependency injection system.
 * It combines the functionality of the Injector and LifetimeManager classes to provide
 * a comprehensive solution for managing component dependencies and lifetimes.
 * 
 * This class supports both synchronous and asynchronous component initialization,
 * comprehensive error handling, and diagnostic features.
 * 
 * @see Injector
 * @see LifetimeManager
 */
class DIFramework {
public:
    /**
     * @brief Create a new DIFramework instance
     */
    DIFramework() 
        : injector_(std::make_shared<Injector>())
        , lifetimeInjector_(std::make_shared<lifetime::LifetimeInjector>()) {
        LOG_DEBUG("Created DIFramework instance");
    }

    /**
     * @brief Destructor
     */
    ~DIFramework() {
        LOG_DEBUG("Destroying DIFramework instance");
        
        // Dispose lifetime-managed components
        if (lifetimeInjector_) {
            lifetimeInjector_->dispose();
        }
    }
    
    /**
     * @brief Non-copyable
     */
    DIFramework(const DIFramework&) = delete;
    DIFramework& operator=(const DIFramework&) = delete;
    
    /**
     * @brief Non-movable
     */
    DIFramework(DIFramework&&) = delete;
    DIFramework& operator=(DIFramework&&) = delete;

    /**
     * @brief Register a factory function that creates an instance of type T
     * 
     * This method registers a factory function for creating instances of type T.
     * The factory function is called when an instance of type T is requested.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     * @param lifetime The lifetime of the service (default: Transient)
     */
    template<typename T>
    void registerFactory(
        std::function<std::shared_ptr<T>()> factory,
        lifetime::ServiceLifetime lifetime = lifetime::ServiceLifetime::Transient) {
        
        LOG_DEBUG("Registering factory for type: " + std::string(typeid(T).name()));
        
        // Register with both injectors for compatibility
        injector_->registerFactory<T>(factory);
        lifetimeInjector_->registerFactory<T>(factory, lifetime);
    }
    
    /**
     * @brief Register a factory function that creates an instance of type T with injector access
     * 
     * This method registers a factory function for creating instances of type T.
     * The factory function is called with the injector as a parameter when an instance of type T is requested.
     * 
     * @note This method is provided for backward compatibility. New code should prefer the parameter-less
     *       registerFactory overload.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance, taking Injector& as parameter
     * @param lifetime The lifetime of the service (default: Transient)
     */
    template<typename T>
    void registerFactory(
        std::function<std::shared_ptr<T>(Injector&)> factory,
        lifetime::ServiceLifetime lifetime = lifetime::ServiceLifetime::Transient) {
        
        LOG_DEBUG("Registering factory with injector access for type: " + std::string(typeid(T).name()));
        
        // Register with both injectors for compatibility
        injector_->registerFactory<T>(factory);
        lifetimeInjector_->registerFactory<T>(factory, lifetime);
    }
    
    /**
     * @brief Register a singleton instance of type T
     * 
     * This method registers an existing instance as a singleton for type T.
     * The same instance will be returned whenever an instance of type T is requested.
     * 
     * @tparam T The interface type to register
     * @param instance The singleton instance
     */
    template<typename T>
    void registerSingleton(std::shared_ptr<T> instance) {
        LOG_DEBUG("Registering singleton instance for type: " + std::string(typeid(T).name()));
        
        // Create a factory that always returns the same instance
        auto factory = [instance]() { return instance; };
        
        // Register with both injectors for compatibility
        injector_->registerFactory<T>(factory);
        lifetimeInjector_->registerFactory<T>(factory, lifetime::ServiceLifetime::Singleton);
    }
    
    /**
     * @brief Register a concrete type as implementation for an interface
     * 
     * This method registers a concrete type as the implementation for an interface.
     * When an instance of the interface is requested, an instance of the concrete type will be created.
     * 
     * @tparam TInterface The interface type to register
     * @tparam TImplementation The concrete type that implements the interface
     * @param lifetime The lifetime of the service (default: Transient)
     */
    template<typename TInterface, typename TImplementation>
    void registerType(lifetime::ServiceLifetime lifetime = lifetime::ServiceLifetime::Transient) {
        static_assert(std::is_base_of<TInterface, TImplementation>::value, 
                     "TImplementation must derive from TInterface");
        
        LOG_DEBUG("Registering concrete type " + std::string(typeid(TImplementation).name()) + 
                  " for interface " + std::string(typeid(TInterface).name()));
        
        // Create a factory function that directly creates the concrete type
        // This is safer than a recursive lambda that could cause deadlocks
        auto factory = []() -> std::shared_ptr<TInterface> { 
            auto instance = std::make_shared<TImplementation>(); 
            return instance;
        };
        
        // Register with both injectors - injector_ first to avoid circular references
        injector_->registerFactory<TInterface>(factory);
        
        // Now register with the lifetime injector
        if (lifetimeInjector_) {
            lifetimeInjector_->registerFactory<TInterface>(factory, lifetime);
        }
        else {
            LOG_ERROR("Lifetime injector is null when registering type " + 
                      std::string(typeid(TInterface).name()));
        }
    }
    
    /**
     * @brief Get an instance of type T
     * 
     * This method resolves an instance of type T from the container.
     * If no factory is registered for type T, an exception is thrown.
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the instance
     * @throws std::runtime_error if no factory is registered for type T
     */
    template<typename T>
    std::shared_ptr<T> get() {
        LOG_DEBUG("Resolving instance of type: " + std::string(typeid(T).name()));
        
        try {
            // Prefer the lifetime injector for more advanced lifetime management
            return lifetimeInjector_->get<T>();
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Failed to resolve type: " + std::string(typeid(T).name()) + 
                      ", error: " + std::string(ex.what()));
            throw;
        }
    }
    
    /**
     * @brief Resolve an instance of type T (legacy method)
     * 
     * This method resolves an instance of type T from the container.
     * If no factory is registered for type T, an exception is thrown.
     * 
     * @note This method is provided for backward compatibility. New code should prefer the get<T>() method.
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the instance
     * @throws std::runtime_error if no factory is registered for type T
     */
    template<typename T>
    std::shared_ptr<T> resolve() {
        LOG_DEBUG("Resolving instance of type (legacy method): " + std::string(typeid(T).name()));
        
        try {
            // Avoid potential recursive calls by creating a local reference to the injector
            auto injector = lifetimeInjector_;
            if (!injector) {
                LOG_ERROR("Lifetime injector is null");
                throw std::runtime_error("Lifetime injector is null");
            }
            
            // Use the get<T> method which should be more stable
            return injector->get<T>();
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Failed to resolve type (legacy method): " + std::string(typeid(T).name()) + 
                      ", error: " + std::string(ex.what()));
            throw;
        }
    }
    
    /**
     * @brief Create a new scope
     * 
     * This method creates a new scope for resolving scoped services.
     * Scoped services are created once per scope and shared within the scope.
     * 
     * @return A shared pointer to a new DIFramework instance representing the scope
     */
    std::shared_ptr<DIFramework> createScope() {
        LOG_DEBUG("Creating new scope");
        
        // Create a new lifetime scope
        auto scopedInjector = lifetimeInjector_->createScope();
        
        // Create a new framework with the scoped injector
        return std::shared_ptr<DIFramework>(new DIFramework(scopedInjector));
    }
    
    /**
     * @brief Dispose the scope
     * 
     * This method disposes the scope and all scoped services.
     * It should be called when the scope is no longer needed.
     */
    void dispose() {
        LOG_DEBUG("Disposing scope");
        
        if (lifetimeInjector_) {
            lifetimeInjector_->dispose();
        }
    }
    
    /**
     * @brief Get the underlying injector
     * 
     * This method returns the underlying injector instance.
     * It should be used only when direct access to the injector is required.
     * 
     * @return A shared pointer to the injector
     */
    std::shared_ptr<Injector> getInjector() {
        return injector_;
    }
    
    /**
     * @brief Get the underlying lifetime injector
     * 
     * This method returns the underlying lifetime injector instance.
     * It should be used only when direct access to the lifetime injector is required.
     * 
     * @return A shared pointer to the lifetime injector
     */
    std::shared_ptr<lifetime::LifetimeInjector> getLifetimeInjector() {
        return lifetimeInjector_;
    }

    // Private constructor for creating scopes
    DIFramework(std::shared_ptr<lifetime::LifetimeInjector> injector) 
        : injector_(std::make_shared<Injector>())
        , lifetimeInjector_(injector) {
        LOG_DEBUG("Created DIFramework instance with provided injector");
    }

private:
    std::shared_ptr<Injector> injector_;
    std::shared_ptr<lifetime::LifetimeInjector> lifetimeInjector_;
};

} // namespace di 