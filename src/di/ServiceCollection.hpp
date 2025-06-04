#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>

#include "DIFramework.hpp"
#include "LifetimeManager.hpp"
#include "AppDebugLog.h"

namespace di {

// Forward declarations
class ServiceProvider;

/**
 * @brief Key for identifying a service by type and name
 */
struct ServiceKey {
    std::type_index type;
    std::string name;
    
    bool operator==(const ServiceKey& other) const {
        return type == other.type && name == other.name;
    }
};

/**
 * @brief Hash function for ServiceKey
 */
struct ServiceKeyHash {
    std::size_t operator()(const ServiceKey& key) const {
        return std::hash<std::type_index>()(key.type) ^ std::hash<std::string>()(key.name);
    }
};

// Type definitions for factory functions
using FactoryFunc = std::function<std::shared_ptr<void>(const ServiceProvider&)>;
using ScopedFactoryFunc = std::function<std::shared_ptr<void>(const ServiceProvider&, const std::string&)>;

/**
 * @class ServiceDescriptor
 * @brief Describes a service registration
 * 
 * This class encapsulates the information needed to register a service with the DI container.
 */
class ServiceDescriptor {
public:
    /**
     * @brief Create a new ServiceDescriptor
     * 
     * @param serviceType The type index of the service interface
     * @param implementationType The type index of the implementation
     * @param lifetime The lifetime of the service
     * @param factory The factory function that creates the service
     */
    ServiceDescriptor(
        std::type_index serviceType,
        std::type_index implementationType,
        lifetime::ServiceLifetime lifetime,
        std::function<std::shared_ptr<void>()> factory)
        : serviceType_(serviceType)
        , implementationType_(implementationType)
        , lifetime_(lifetime)
        , factory_(factory) {
    }

    /**
     * @brief Get the service type
     * 
     * @return The type index of the service interface
     */
    std::type_index getServiceType() const { return serviceType_; }
    
    /**
     * @brief Get the implementation type
     * 
     * @return The type index of the implementation
     */
    std::type_index getImplementationType() const { return implementationType_; }
    
    /**
     * @brief Get the lifetime
     * 
     * @return The lifetime of the service
     */
    lifetime::ServiceLifetime getLifetime() const { return lifetime_; }
    
    /**
     * @brief Get the factory function
     * 
     * @return The factory function that creates the service
     */
    std::function<std::shared_ptr<void>()> getFactory() const { return factory_; }

private:
    std::type_index serviceType_;
    std::type_index implementationType_;
    lifetime::ServiceLifetime lifetime_;
    std::function<std::shared_ptr<void>()> factory_;
};

/**
 * @class ServiceCollection
 * @brief A collection of service descriptors
 * 
 * This class provides a fluent API for registering services with the DI container.
 * It allows for registering services by interface or concrete type with various lifetime options.
 */
class ServiceCollection {
public:
    /**
     * @brief Create a new ServiceCollection
     */
    ServiceCollection() {
        LOG_DEBUG("Created ServiceCollection");
    }
    
    /**
     * @brief Destructor
     */
    ~ServiceCollection() {
        LOG_DEBUG("Destroying ServiceCollection");
    }
    
    /**
     * @brief Register a singleton instance of type T
     * 
     * This method registers an existing instance as a singleton for type T.
     * The same instance will be returned whenever an instance of type T is requested.
     * 
     * @tparam T The interface type to register
     * @param instance The singleton instance
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename T>
    ServiceCollection& addSingleton(std::shared_ptr<T> instance) {
        LOG_DEBUG("Adding singleton instance for type: " + std::string(typeid(T).name()));
        
        // Create a factory that always returns the same instance
        auto factory = [instance]() -> std::shared_ptr<void> { return instance; };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(T)),
            std::type_index(typeid(*instance)),
            lifetime::ServiceLifetime::Singleton,
            factory);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Register a factory function that creates a singleton instance of type T
     * 
     * This method registers a factory function for creating a singleton instance of type T.
     * The factory function is called once when the first instance of type T is requested,
     * and the same instance is returned for all subsequent requests.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename T>
    ServiceCollection& addSingleton(std::function<std::shared_ptr<T>()> factory) {
        LOG_DEBUG("Adding singleton factory for type: " + std::string(typeid(T).name()));
        
        // Create a factory wrapper that casts to void
        auto factoryWrapper = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(T)),
            std::type_index(typeid(T)), // Use the same type for implementation
            lifetime::ServiceLifetime::Singleton,
            factoryWrapper);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Register a concrete type as a singleton implementation for an interface
     * 
     * This method registers a concrete type as the implementation for an interface.
     * The first time an instance of the interface is requested, an instance of the concrete type
     * will be created, and the same instance will be returned for all subsequent requests.
     * 
     * @tparam TInterface The interface type to register
     * @tparam TImplementation The concrete type that implements the interface
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename TInterface, typename TImplementation>
    ServiceCollection& addSingleton() {
        LOG_DEBUG("Adding singleton type " + std::string(typeid(TImplementation).name()) + 
                  " for interface " + std::string(typeid(TInterface).name()));
        
        // Create a factory that creates a new instance of the concrete type
        auto factory = []() -> std::shared_ptr<void> {
            return std::make_shared<TImplementation>();
        };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(TInterface)),
            std::type_index(typeid(TImplementation)),
            lifetime::ServiceLifetime::Singleton,
            factory);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Register a factory function that creates a scoped instance of type T
     * 
     * This method registers a factory function for creating a scoped instance of type T.
     * The factory function is called once per scope when an instance of type T is requested,
     * and the same instance is returned for all subsequent requests within the same scope.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename T>
    ServiceCollection& addScoped(std::function<std::shared_ptr<T>()> factory) {
        LOG_DEBUG("Adding scoped factory for type: " + std::string(typeid(T).name()));
        
        // Create a factory wrapper that casts to void
        auto factoryWrapper = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(T)),
            std::type_index(typeid(T)), // Use the same type for implementation
            lifetime::ServiceLifetime::Scoped,
            factoryWrapper);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Register a concrete type as a scoped implementation for an interface
     * 
     * This method registers a concrete type as the implementation for an interface.
     * Each time a new scope is created and an instance of the interface is requested,
     * a new instance of the concrete type will be created for that scope.
     * 
     * @tparam TInterface The interface type to register
     * @tparam TImplementation The concrete type that implements the interface
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename TInterface, typename TImplementation>
    ServiceCollection& addScoped() {
        LOG_DEBUG("Adding scoped type " + std::string(typeid(TImplementation).name()) + 
                  " for interface " + std::string(typeid(TInterface).name()));
        
        // Create a factory that creates a new instance of the concrete type
        auto factory = []() -> std::shared_ptr<void> {
            return std::make_shared<TImplementation>();
        };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(TInterface)),
            std::type_index(typeid(TImplementation)),
            lifetime::ServiceLifetime::Scoped,
            factory);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Register a factory function that creates a transient instance of type T
     * 
     * This method registers a factory function for creating a transient instance of type T.
     * The factory function is called each time an instance of type T is requested.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename T>
    ServiceCollection& addTransient(std::function<std::shared_ptr<T>()> factory) {
        LOG_DEBUG("Adding transient factory for type: " + std::string(typeid(T).name()));
        
        // Create a factory wrapper that casts to void
        auto factoryWrapper = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(T)),
            std::type_index(typeid(T)), // Use the same type for implementation
            lifetime::ServiceLifetime::Transient,
            factoryWrapper);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Register a concrete type as a transient implementation for an interface
     * 
     * This method registers a concrete type as the implementation for an interface.
     * Each time an instance of the interface is requested, a new instance of the concrete type will be created.
     * 
     * @tparam TInterface The interface type to register
     * @tparam TImplementation The concrete type that implements the interface
     * @return A reference to this ServiceCollection for method chaining
     */
    template<typename TInterface, typename TImplementation>
    ServiceCollection& addTransient() {
        LOG_DEBUG("Adding transient type " + std::string(typeid(TImplementation).name()) + 
                  " for interface " + std::string(typeid(TInterface).name()));
        
        // Create a factory that creates a new instance of the concrete type
        auto factory = []() -> std::shared_ptr<void> {
            return std::make_shared<TImplementation>();
        };
        
        // Create a service descriptor
        ServiceDescriptor descriptor(
            std::type_index(typeid(TInterface)),
            std::type_index(typeid(TImplementation)),
            lifetime::ServiceLifetime::Transient,
            factory);
        
        // Add the descriptor to the collection
        descriptors_.push_back(descriptor);
        
        return *this;
    }
    
    /**
     * @brief Adds a scoped service to the collection with a factory that takes the current scope ID
     * 
     * Scoped services are created once per scope and reused within that scope.
     * The factory function receives both the service provider and the scope ID (request ID).
     * 
     * @tparam T The interface type to register
     * @param name Optional name for the service
     * @param factory Factory function to create the service, receiving the service provider and scope ID
     * @return Reference to this ServiceCollection for method chaining
     */
    template<typename T>
    ServiceCollection& addScoped(const std::string& name, 
                                std::function<std::shared_ptr<T>(const ServiceProvider&, const std::string&)> factory) {
        auto typeIndex = std::type_index(typeid(T));
        LOG_DEBUG("Adding scoped service " + std::string(typeid(T).name()) + 
                 (name.empty() ? "" : " with name '" + name + "'"));

        factories_[{typeIndex, name}] = [factory](const ServiceProvider& provider) -> std::shared_ptr<void> {
            // The actual creation will happen in the RequestScopeManager
            // Here we just register the factory
            return nullptr;
        };
        
        // Store the scoped factory separately to be used by the RequestScopeManager
        scopedFactories_[{typeIndex, name}] = [factory](const ServiceProvider& provider, const std::string& scopeId) -> std::shared_ptr<void> {
            try {
                auto instance = factory(provider, scopeId);
                return std::static_pointer_cast<void>(instance);
            } catch (const std::exception& ex) {
                LOG_ERROR("Error creating scoped service " + std::string(typeid(T).name()) + 
                         (name.empty() ? "" : " with name '" + name + "'") + ": " + ex.what());
                throw;
            }
        };
        
        return *this;
    }
    
    /**
     * @brief Build a DIFramework from this service collection
     * 
     * This method creates a new DIFramework and registers all services from this collection with it.
     * 
     * @return A shared pointer to the new DIFramework
     */
    std::shared_ptr<DIFramework> buildServiceProvider() {
        LOG_DEBUG("Building service provider from ServiceCollection");
        
        // Create a new DIFramework
        auto framework = std::make_shared<DIFramework>();
        
        // Register all services with the framework
        for (const auto& descriptor : descriptors_) {
            // Get the factory and lifetime
            auto factory = descriptor.getFactory();
            auto lifetime = descriptor.getLifetime();
            auto serviceType = descriptor.getServiceType();
            
            // Register the factory with the framework
            // Since we can't use template arguments at runtime, we use the raw registerFactory method
            // on the LifetimeInjector to register the factory.
            framework->getLifetimeInjector()->registerRaw(serviceType, factory, lifetime);
        }
        
        return framework;
    }
    
private:
    std::vector<ServiceDescriptor> descriptors_;
    
    // Map of type + name to factory function
    std::unordered_map<ServiceKey, FactoryFunc, ServiceKeyHash> factories_;
    
    // Map of type + name to scoped factory function (used by RequestScopeManager)
    std::unordered_map<ServiceKey, ScopedFactoryFunc, ServiceKeyHash> scopedFactories_;
    
    // Reference to the parent service provider (for scoped services)
    std::shared_ptr<ServiceProvider> parentProvider_;
};

} // namespace di 