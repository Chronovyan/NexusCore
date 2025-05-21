#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include "TypeTraits.hpp"

namespace di {

/// Specifies lifetime of a binding
enum class Lifetime {
    Singleton,   // one shared instance for the life of the container
    Transient    // a new instance on every resolve()
};

/// Core resolver interface: Injector implements this
struct IDependencyResolver {
    virtual ~IDependencyResolver() = default;

    /// Resolve by run‐time type
    virtual std::shared_ptr<void> resolveType(const std::type_index& type) = 0;

    /// Templated convenience wrapper
    template<typename T>
    std::shared_ptr<T> resolve() {
        auto ptr = resolveType(typeid(T));
        if (!ptr) {
            throw std::runtime_error("Failed to resolve type: " + std::string(typeid(T).name()));
        }
        return std::static_pointer_cast<T>(ptr);
    }
};

/// Simple DI container / injector
class Injector : public IDependencyResolver {
public:
    Injector() = default;
    ~Injector() = default;

    //── Registration APIs ─────────────────────────────────────────────────────

    /// Bind Interface → concrete Impl, specifying lifetime.
    template<typename Interface, typename Implementation>
    void registerType(Lifetime lifetime = Lifetime::Transient) {
        static_assert(std::is_base_of<Interface, Implementation>::value || 
                     std::is_same<Interface, Implementation>::value,
                     "Implementation must inherit from Interface");

        auto factory = [](Injector& injector) -> std::shared_ptr<void> {
            return TypeInfo<Implementation>::create(injector);
        };

        registerBinding<Interface>(factory, lifetime);
    }

    /// Bind Interface → this pre-constructed instance
    template<typename Interface>
    void registerInstance(std::shared_ptr<Interface> instance) {
        if (!instance) {
            throw std::invalid_argument("Cannot register null instance");
        }
        
        auto factory = [instance](Injector&) -> std::shared_ptr<void> {
            return instance;
        };
        
        registerBinding<Interface>(factory, Lifetime::Singleton);
    }

    /// Bind Interface → factory function (takes Injector&), with lifetime.
    template<typename Interface>
    void registerFactory(
        std::function<std::shared_ptr<Interface>(Injector&)> factory,
        Lifetime lifetime = Lifetime::Transient
    ) {
        if (!factory) {
            throw std::invalid_argument("Cannot register null factory");
        }
        
        auto voidFactory = [factory](Injector& injector) -> std::shared_ptr<void> {
            return factory(injector);
        };
        
        registerBinding<Interface>(voidFactory, lifetime);
    }

    //── Resolution API ───────────────────────────────────────────────────────

    /// implement IDependencyResolver
    std::shared_ptr<void> resolveType(const std::type_index& type) override {
        auto it = _bindings.find(type);
        if (it == _bindings.end()) {
            return nullptr;
        }

        Binding& binding = it->second;

        if (binding.lifetime == Lifetime::Singleton) {
            if (!binding.singletonInstance) {
                binding.singletonInstance = binding.factory(*this);
            }
            return binding.singletonInstance;
        } else {
            return binding.factory(*this);
        }
    }

    //── (Optional) Scope / Child Containers ─────────────────────────────────

    /// Create a child injector that inherits current registrations
    Injector createChildInjector() const {
        Injector child;
        child._bindings = _bindings;  // Copy all bindings
        return child;
    }

    //── (Optional) Introspection ─────────────────────────────────────────────

    /// Check if Interface is already registered
    template<typename Interface>
    bool isRegistered() const {
        return _bindings.find(typeid(Interface)) != _bindings.end();
    }

private:
    template<typename Interface>
    void registerBinding(
        std::function<std::shared_ptr<void>(Injector&)> factory,
        Lifetime lifetime
    ) {
        Binding binding;
        binding.factory = factory;
        binding.lifetime = lifetime;
        binding.singletonInstance = nullptr;
        
        _bindings[typeid(Interface)] = binding;
    }

    struct Binding {
        Lifetime lifetime;
        std::function<std::shared_ptr<void>(Injector&)> factory;
        std::shared_ptr<void> singletonInstance;  // for singletons
    };
    
    std::unordered_map<std::type_index, Binding> _bindings;
};

} // namespace di 