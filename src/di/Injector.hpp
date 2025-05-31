#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>
#include <iostream>

namespace di {

/**
 * Core resolver interface that Injector implements
 * 
 * This interface is provided for backward compatibility with existing code
 * that relies on the resolve<T>() method. New code should prefer using
 * Injector::get<T>() directly for clarity.
 * 
 * @see Injector::get<T>() - The modern alternative to resolve<T>()
 */
class IDependencyResolver {
public:
    virtual ~IDependencyResolver() = default;
    
    /// Resolve by run-time type
    virtual std::shared_ptr<void> resolveType(const std::type_index& type) = 0;
    
    /**
     * Resolves a dependency by type (Legacy Interface)
     * 
     * @note This method is maintained for backward compatibility.
     * @note New code should prefer Injector::get<T>() for clarity and to signal intent to 
     *       transition away from the legacy interface.
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the instance
     * @throws std::runtime_error if no factory is registered for type T
     */
    template<typename T>
    std::shared_ptr<T> resolve() {
        auto ptr = resolveType(typeid(T));
        if (!ptr) {
            throw std::runtime_error("Failed to resolve type: " + std::string(typeid(T).name()));
        }
        return std::static_pointer_cast<T>(ptr);
    }
};

/**
 * A simple dependency injection container that manages object creation and lifetime.
 * This container allows registering factory functions for interfaces and resolving them when needed.
 * 
 * The Injector supports two styles of usage:
 * 1. Modern style: Use get<T>() with lambda factories that don't need injector reference
 *    - Preferred for all new code
 *    - More explicit about intent
 *    - Simpler to understand and use
 * 
 * 2. Legacy style: Use resolve<T>() with factories that accept an injector reference
 *    - Maintained for backward compatibility
 *    - Still fully functional and supported
 *    - Allows gradual migration of existing code
 * 
 * Usage Examples:
 * 
 * 1. Modern Style:
 * ```cpp
 * // Registration
 * injector.registerFactory<ILogger>([]() {
 *     return std::make_shared<ConsoleLogger>();
 * });
 * 
 * // Resolution
 * auto logger = injector.get<ILogger>();
 * ```
 * 
 * 2. Legacy Style:
 * ```cpp
 * // Registration with injector reference
 * injector.registerFactory<IService>([](Injector& inj) {
 *     auto logger = inj.resolve<ILogger>();
 *     return std::make_shared<Service>(logger);
 * });
 * 
 * // Resolution
 * auto service = injector.resolve<IService>();
 * ```
 * 
 * Both styles can be mixed as needed to facilitate gradual migration.
 */
class Injector : public IDependencyResolver {
public:
    Injector() = default;
    ~Injector() = default;

    /**
     * Register a factory function that creates an instance of type T
     * 
     * This is the preferred modern style for registering factories.
     * Use this when the factory doesn't need to resolve additional dependencies.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance
     */
    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>()> factory) {
        auto typeIndex = std::type_index(typeid(T));
        factories_[typeIndex] = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
    }

    /**
     * Register a factory function that creates an instance of type T with injector access
     * 
     * This style is provided for backward compatibility. It allows the factory to
     * resolve additional dependencies from the injector.
     * 
     * @note For new code, consider structuring your factories to use constructor injection
     *       and the parameter-less registerFactory overload instead.
     * 
     * @tparam T The interface type to register
     * @param factory The factory function that creates the instance, taking Injector& as parameter
     */
    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>(Injector&)> factory) {
        auto typeIndex = std::type_index(typeid(T));
        auto selfRef = this; // Capture this pointer
        factories_[typeIndex] = [factory, selfRef]() -> std::shared_ptr<void> {
            return factory(*selfRef);
        };
    }

    /**
     * Register a factory function by type index
     * 
     * This is an advanced method used by the LifetimeManager to register factories
     * for types that are not known at compile time.
     * 
     * @param typeIndex The type index to register
     * @param factory The factory function that creates the instance
     */
    void registerFactory(const std::type_index& typeIndex, std::function<std::shared_ptr<void>()> factory) {
        factories_[typeIndex] = factory;
    }

    /**
     * Get an instance of type T (Modern Interface)
     * 
     * This is the preferred modern style for resolving dependencies.
     * It behaves identically to resolve<T>() but with a clearer name.
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the instance
     * @throws std::runtime_error if no factory is registered for type T
     */
    template<typename T>
    std::shared_ptr<T> get() {
        return resolveImpl<T>();
    }

    /**
     * Implementation for resolveType which can be used by both get() and resolve()
     * 
     * This method fulfills the IDependencyResolver interface requirement.
     * 
     * @param type The type index to resolve
     * @return A shared pointer to the void-cast instance
     * @throws std::runtime_error if no factory is registered for the given type index
     */
    std::shared_ptr<void> resolveType(const std::type_index& type) override {
        std::cout << "Injector::resolveType(" << type.name() << ")" << std::endl;
        
        auto factoryIt = factories_.find(type);
        if (factoryIt == factories_.end()) {
            std::cerr << "No factory registered for type index: " << type.name() << std::endl;
            std::cerr << "Registered types: ";
            for (const auto& factory : factories_) {
                std::cerr << factory.first.name() << ", ";
            }
            std::cerr << std::endl;
            throw std::runtime_error("No factory registered for type index");
        }
        
        auto instance = factoryIt->second();
        if (!instance) {
            std::cerr << "Failed to create instance from factory for type: " << type.name() << std::endl;
            throw std::runtime_error("Failed to create instance from factory");
        }
        
        return instance;
    }

    /**
     * Clear all registered factories.
     * 
     * This resets the injector to its initial state, removing all registered types.
     */
    void clear() {
        factories_.clear();
    }

    /**
     * Get the factory function for a type without lifetime management
     * 
     * This method is used by the LifetimeInjector to get the original factory
     * function for a type, without applying any lifetime management.
     * 
     * @param type The type index to get the factory for
     * @return The factory function or nullptr if not found
     */
    std::function<std::shared_ptr<void>()> getFactoryWithoutLifetimeManagement(const std::type_index& type) const {
        auto it = factories_.find(type);
        if (it != factories_.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief Get direct access to the factories map
     * 
     * This is an advanced method used by the LifetimeManager to access factories
     * for child scope creation and dependency transfer.
     * 
     * @return A const reference to the factories map
     */
    const std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>>& getFactories() const {
        return factories_;
    }

private:
    /**
     * Common implementation used by both get<T>() and resolve<T>()
     * 
     * This private method encapsulates the shared implementation logic between
     * the modern get<T>() method and the legacy resolve<T>() method.
     * 
     * @tparam T The interface type to resolve
     * @return A shared pointer to the instance
     */
    template<typename T>
    std::shared_ptr<T> resolveImpl() {
        auto typeIndex = std::type_index(typeid(T));
        auto instance = std::static_pointer_cast<T>(resolveType(typeIndex));
        return instance;
    }

    // Map of type indexes to factory functions
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> factories_;
};

} // namespace di 