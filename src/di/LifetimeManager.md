# LifetimeManager - Advanced Service Lifetime Management

The `LifetimeManager` is an extension to the DI framework that provides sophisticated lifetime management for services. It allows you to control how services are instantiated, shared, and disposed of throughout your application.

## Features

- **Multiple Lifetime Options**: Support for singleton, transient, and scoped lifetimes
- **Thread Safety**: Thread-safe service creation and caching
- **Resource Management**: Automatic cleanup of disposable resources
- **Scoped Resolution**: Support for hierarchical scopes
- **Seamless Integration**: Works with both modern and legacy DI interfaces

## Lifetime Types

The `LifetimeManager` supports three types of service lifetimes:

1. **Singleton**: Services are created once and shared for all requests. These instances exist for the entire lifetime of the application.

2. **Transient**: Services are created anew for each request. Each time a service is resolved, a new instance is created.

3. **Scoped**: Services are created once per scope. These instances exist for the lifetime of the scope, which is typically shorter than the application lifetime (e.g., for the duration of a web request).

## Key Components

### ServiceLifetime Enum

```cpp
enum class ServiceLifetime {
    Singleton,   // Services are created once and shared for all requests
    Transient,   // Services are created for each request
    Scoped       // Services are created once per scope
};
```

### IDisposable Interface

```cpp
class IDisposable {
public:
    virtual ~IDisposable() = default;
    virtual void dispose() = 0;
};
```

Services that need to perform cleanup when they're no longer needed should implement this interface.

### LifetimeManager Class

The core class that manages the lifetime of services:

```cpp
class LifetimeManager {
public:
    // Create instance based on lifetime
    template<typename T>
    std::shared_ptr<T> getInstance(
        std::function<std::shared_ptr<T>()> factory,
        ServiceLifetime lifetime = ServiceLifetime::Transient
    );
    
    // Register disposable instance
    void registerDisposable(std::shared_ptr<IDisposable> instance);
    
    // Create a new scope
    std::shared_ptr<LifetimeManager> createScope();
    
    // Dispose all instances in this scope
    void dispose();
};
```

### LifetimeInjector Class

A decorator for `Injector` that adds lifetime management:

```cpp
class LifetimeInjector {
public:
    // Register factory with lifetime
    template<typename T>
    void registerFactory(
        std::function<std::shared_ptr<T>()> factory,
        ServiceLifetime lifetime = ServiceLifetime::Transient
    );
    
    // Register factory with injector access and lifetime
    template<typename T>
    void registerFactory(
        std::function<std::shared_ptr<T>(Injector&)> factory,
        ServiceLifetime lifetime = ServiceLifetime::Transient
    );
    
    // Get instance
    template<typename T>
    std::shared_ptr<T> get();
    
    // Resolve instance (legacy)
    template<typename T>
    std::shared_ptr<T> resolve();
    
    // Create a new scope
    std::shared_ptr<LifetimeInjector> createScope();
    
    // Dispose all scoped instances
    void dispose();
};
```

## Usage Patterns

### Basic Registration and Resolution

```cpp
// Create a lifetime injector
LifetimeInjector injector;

// Register a service with singleton lifetime
injector.registerFactory<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
}, ServiceLifetime::Singleton);

// Register a service with transient lifetime
injector.registerFactory<IUserService>([]() {
    return std::make_shared<UserService>();
}, ServiceLifetime::Transient);

// Get instances
auto logger1 = injector.get<ILogger>();
auto logger2 = injector.get<ILogger>();
// logger1 and logger2 will be the same instance

auto service1 = injector.get<IUserService>();
auto service2 = injector.get<IUserService>();
// service1 and service2 will be different instances
```

### Working with Scopes

```cpp
// Create a lifetime injector
LifetimeInjector injector;

// Register services
injector.registerFactory<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
}, ServiceLifetime::Singleton);

injector.registerFactory<IUserRepository>([]() {
    return std::make_shared<UserRepository>();
}, ServiceLifetime::Scoped);

// Create a scope
auto scope = injector.createScope();

// Get scoped services
auto repo1 = scope->get<IUserRepository>();
auto repo2 = scope->get<IUserRepository>();
// repo1 and repo2 will be the same instance within this scope

// Create another scope
auto scope2 = injector.createScope();
auto repo3 = scope2->get<IUserRepository>();
// repo3 will be a different instance than repo1/repo2

// Clean up when done
scope->dispose();
scope2->dispose();
```

### Disposable Resources

```cpp
// Define a disposable service
class Database : public IDisposable {
public:
    void dispose() override {
        // Clean up resources
        disconnect();
    }
    
    void disconnect() {
        // Close connection
    }
};

// Register the service
injector.registerFactory<IDatabase>([]() {
    auto db = std::make_shared<Database>();
    db->connect();
    return db;
}, ServiceLifetime::Singleton);

// Use the service
auto db = injector.get<IDatabase>();

// When done, dispose of resources
injector.dispose();
// This will call dispose() on the Database instance
```

## Best Practices

1. **Choose Appropriate Lifetimes**:
   - Use **Singleton** for services that are stateless or that naturally exist as a single instance (e.g., configuration, logging).
   - Use **Transient** for services that maintain state specific to each operation or that aren't safe to share (e.g., non-thread-safe objects).
   - Use **Scoped** for services that should be shared within a logical operation but not across operations (e.g., database connections, user context).

2. **Manage Resources Properly**:
   - Implement `IDisposable` for services that manage resources (file handles, network connections, etc.).
   - Always call `dispose()` on scopes when you're done with them.
   - Call `dispose()` on the root injector when shutting down the application.

3. **Be Careful with Injector References**:
   - When using factories that take an `Injector&` parameter, be careful not to create circular dependencies.
   - Don't store the injector reference in services; resolve all dependencies upfront.

4. **Thread Safety Considerations**:
   - The `LifetimeManager` is thread-safe for its primary operations, but your services should also be thread-safe if they'll be shared across threads.
   - Singleton services should generally be stateless or thread-safe.

5. **Scope Lifetime**:
   - Scopes should have a clear lifetime, typically tied to a logical operation (e.g., a web request).
   - Don't keep scopes alive longer than necessary, as they may hold onto resources.

## Example Application Structure

```cpp
// Application entry point
int main() {
    // Create the root injector and register services
    LifetimeInjector rootInjector;
    
    // Register singleton services
    rootInjector.registerFactory<IConfiguration>([]() {
        return std::make_shared<AppConfiguration>();
    }, ServiceLifetime::Singleton);
    
    rootInjector.registerFactory<ILogger>([]() {
        return std::make_shared<ConsoleLogger>();
    }, ServiceLifetime::Singleton);
    
    // Register services with different lifetimes
    rootInjector.registerFactory<IDatabase>([](Injector& inj) {
        auto config = inj.resolve<IConfiguration>();
        auto logger = inj.resolve<ILogger>();
        return std::make_shared<Database>(config, logger);
    }, ServiceLifetime::Singleton);
    
    rootInjector.registerFactory<IUserRepository>([](Injector& inj) {
        auto db = inj.resolve<IDatabase>();
        return std::make_shared<UserRepository>(db);
    }, ServiceLifetime::Scoped);
    
    // Application loop
    while (running) {
        // For each request or operation, create a scope
        auto scope = rootInjector.createScope();
        
        // Use scoped services
        auto repo = scope->get<IUserRepository>();
        // ... perform operations ...
        
        // Clean up the scope when done
        scope->dispose();
    }
    
    // Clean up the application
    rootInjector.dispose();
    
    return 0;
}
```

## Advanced Scenarios

### Combining with Decorators

You can combine the `LifetimeManager` with decorators to add cross-cutting concerns:

```cpp
// Create a logging decorator
template<typename T>
std::function<std::shared_ptr<T>()> createLoggingFactory(
    std::function<std::shared_ptr<T>()> factory,
    std::shared_ptr<ILogger> logger
) {
    return [factory, logger]() {
        logger->log("Creating instance of " + typeid(T).name());
        auto instance = factory();
        logger->log("Instance created");
        return instance;
    };
}

// Register with both lifetime management and logging
auto logger = std::make_shared<ConsoleLogger>();

injector.registerFactory<IUserService>(
    createLoggingFactory<IUserService>(
        []() { return std::make_shared<UserService>(); },
        logger
    ),
    ServiceLifetime::Scoped
);
```

### Custom Lifetime Management

You can create custom lifetime behaviors by extending the `LifetimeManager`:

```cpp
class CustomLifetimeManager : public LifetimeManager {
public:
    // Add custom lifetime behaviors
    template<typename T>
    std::shared_ptr<T> getPooledInstance(
        std::function<std::shared_ptr<T>()> factory,
        int maxPoolSize
    ) {
        // Implement object pooling logic
    }
};
```

## Conclusion

The `LifetimeManager` extends the DI framework with sophisticated lifetime management capabilities, allowing you to control how services are instantiated, shared, and disposed of throughout your application. By choosing the appropriate lifetime for each service and following best practices for resource management, you can build applications that are both flexible and robust. 