#pragma once

#include "Injector.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace di {
namespace patterns {

/**
 * @brief Creates a singleton factory for a service
 * 
 * This function wraps a factory function to ensure that it only creates
 * one instance of the service, which is then returned for all subsequent calls.
 * 
 * @tparam T The interface type to create a singleton for
 * @param factory The factory function that creates the instance
 * @return A factory function that always returns the same instance
 */
template<typename T>
std::function<std::shared_ptr<T>()> createSingleton(std::function<std::shared_ptr<T>()> factory) {
    // Use a static variable to cache the instance
    static std::shared_ptr<T> instance;
    static std::once_flag initFlag;
    
    return [factory]() {
        std::call_once(initFlag, [&]() {
            instance = factory();
        });
        return instance;
    };
}

/**
 * @brief Singleton extension for the Injector
 * 
 * This class wraps an Injector to provide singleton behavior for registered services.
 * It ensures that each service is only created once, and the same instance is returned
 * for all subsequent calls to get<T>() or resolve<T>().
 */
class SingletonInjector {
public:
    /**
     * @brief Construct a new Singleton Injector
     */
    SingletonInjector() = default;
    
    /**
     * @brief Register a factory function that creates a singleton instance of type T
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     */
    template<typename T>
    void registerSingleton(std::function<std::shared_ptr<T>()> factory) {
        injector_.registerFactory<T>(createSingleton<T>(factory));
    }
    
    /**
     * @brief Register a factory function that creates a singleton instance of type T with injector access
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance, taking Injector& as parameter
     */
    template<typename T>
    void registerSingleton(std::function<std::shared_ptr<T>(Injector&)> factory) {
        // Capture this to access the injector
        auto selfRef = this;
        injector_.registerFactory<T>(createSingleton<T>([factory, selfRef]() {
            return factory(selfRef->injector_);
        }));
    }
    
    /**
     * @brief Get an instance of type T
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the singleton instance
     */
    template<typename T>
    std::shared_ptr<T> get() {
        return injector_.get<T>();
    }
    
    /**
     * @brief Resolve an instance of type T (legacy interface)
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the singleton instance
     */
    template<typename T>
    std::shared_ptr<T> resolve() {
        return injector_.resolve<T>();
    }
    
private:
    Injector injector_;
};

/**
 * @brief Creates a decorated factory that logs creation events
 * 
 * This function wraps a factory function to add logging behavior when
 * a service is created.
 * 
 * @tparam T The interface type to decorate
 * @param factory The factory function that creates the instance
 * @param logger The logger to use for logging creation events
 * @return A factory function that logs when creating instances
 */
template<typename T>
std::function<std::shared_ptr<T>()> createLoggingFactory(
    std::function<std::shared_ptr<T>()> factory,
    std::shared_ptr<ISimpleLogger> logger
) {
    return [factory, logger]() {
        logger->log("Creating instance of " + std::string(typeid(T).name()));
        auto instance = factory();
        logger->log("Instance of " + std::string(typeid(T).name()) + " created");
        return instance;
    };
}

/**
 * @brief Factory for creating scoped lifetimes
 * 
 * This class helps create a new scope for dependency resolution.
 * It allows for registering services that are only available within a specific scope.
 */
class ScopedInjector {
public:
    /**
     * @brief Construct a new Scoped Injector with a parent injector
     * 
     * @param parent The parent injector to delegate to if a service is not found in this scope
     */
    explicit ScopedInjector(Injector& parent) : parent_(parent) {}
    
    /**
     * @brief Register a factory function that creates an instance of type T
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     */
    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>()> factory) {
        injector_.registerFactory<T>(factory);
    }
    
    /**
     * @brief Get an instance of type T
     * 
     * This method first tries to resolve the service from the scoped injector.
     * If not found, it delegates to the parent injector.
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the instance
     */
    template<typename T>
    std::shared_ptr<T> get() {
        try {
            return injector_.get<T>();
        } catch (const std::runtime_error&) {
            // If not found in this scope, try the parent
            return parent_.get<T>();
        }
    }
    
private:
    Injector injector_;
    Injector& parent_;
};

} // namespace patterns
} // namespace di 