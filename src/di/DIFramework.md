# DIFramework: Enhanced Dependency Injection for C++

## Overview

DIFramework is an enhanced dependency injection (DI) system for C++ that builds upon and extends the functionality of the core Injector and LifetimeManager classes. It provides a more intuitive API, better error handling, and improved lifetime management for components.

## Key Features

- **Simplified API**: More intuitive and fluent interface for service registration and resolution
- **Advanced Lifetime Management**: Support for transient, singleton, and scoped services
- **Fluent Registration**: ServiceCollection for clean, chainable service registration
- **Comprehensive Error Handling**: Detailed error messages and logging
- **Thread Safety**: Thread-safe service creation and management
- **Resource Cleanup**: Automatic disposal of services when scopes are disposed
- **Modern C++ Design**: Uses modern C++ features and idioms

## Components

### DIFramework

The central class that manages dependency injection for the application. It combines the functionality of the Injector and LifetimeManager classes to provide a comprehensive solution for managing component dependencies and lifetimes.

```cpp
// Create a new DIFramework instance
auto framework = std::make_shared<DIFramework>();

// Register a transient service
framework->registerFactory<IGreeter>([]() {
    return std::make_shared<SimpleGreeter>();
});

// Register a singleton service
framework->registerFactory<ILogger>(
    []() { return std::make_shared<ConsoleLogger>(); },
    lifetime::ServiceLifetime::Singleton);

// Register a concrete type as implementation for an interface
framework->registerType<ITimeProvider, SimpleTimeProvider>();

// Register an existing instance as a singleton
auto config = std::make_shared<Configuration>();
framework->registerSingleton<IConfiguration>(config);

// Resolve a service
auto greeter = framework->get<IGreeter>();
greeter->greet("World");
```

### ServiceCollection

A class that provides a fluent API for registering services with the DI container. It allows for registering services by interface or concrete type with various lifetime options.

```cpp
// Create a service collection
ServiceCollection services;

// Register services with various lifetimes
services.addTransient<IGreeter, SimpleGreeter>()
        .addSingleton<ILogger>([]() { return std::make_shared<ConsoleLogger>(); })
        .addScoped<ITimeProvider, SimpleTimeProvider>()
        .addSingleton<IConfiguration>(config);

// Build the service provider
auto provider = services.buildServiceProvider();

// Resolve services
auto greeter = provider->get<IGreeter>();
auto logger = provider->get<ILogger>();
```

### ServiceLifetime

An enum that defines the different service lifetimes:

- **Singleton**: Services are created once and shared for all requests
- **Transient**: Services are created for each request
- **Scoped**: Services are created once per scope

### Scopes

Scopes allow for creating isolated environments for service resolution. Scoped services are created once per scope and shared within the scope.

```cpp
// Create a scope
auto scope = framework->createScope();

// Resolve services within the scope
auto greeter = scope->get<IGreeter>();
auto logger = scope->get<ILogger>();

// Dispose the scope when done
scope->dispose();
```

### Disposable Services

Services that implement the `IDisposable` interface will have their `dispose()` method called when the scope they belong to is disposed.

```cpp
class MyDisposableService : public IDisposable {
public:
    void dispose() override {
        // Release resources
    }
};
```

## Usage Examples

### Basic Usage

```cpp
// Create a DIFramework
auto framework = std::make_shared<DIFramework>();

// Register services
framework->registerFactory<IGreeter>([]() {
    return std::make_shared<SimpleGreeter>();
});

framework->registerFactory<ITimeProvider>([]() {
    return std::make_shared<SimpleTimeProvider>();
});

// Register a service that depends on other services
framework->registerFactory<GreetingService>([framework]() {
    auto greeter = framework->get<IGreeter>();
    auto timeProvider = framework->get<ITimeProvider>();
    return std::make_shared<GreetingService>(greeter, timeProvider);
});

// Resolve and use a service
auto service = framework->get<GreetingService>();
std::cout << service->greetWithTime("World") << std::endl;
```

### Using ServiceCollection

```cpp
// Create a service collection
ServiceCollection services;

// Register services
services.addTransient<IGreeter, SimpleGreeter>()
        .addSingleton<ITimeProvider, SimpleTimeProvider>()
        .addTransient<GreetingService>();

// Build the service provider
auto provider = services.buildServiceProvider();

// Resolve and use a service
auto service = provider->get<GreetingService>();
std::cout << service->greetWithTime("World") << std::endl;
```

### Using Scopes

```cpp
// Create a DIFramework
auto framework = std::make_shared<DIFramework>();

// Register services
framework->registerFactory<IUserRepository>(
    []() { return std::make_shared<UserRepository>(); },
    lifetime::ServiceLifetime::Scoped);

// Create scopes for different requests
auto scope1 = framework->createScope();
auto scope2 = framework->createScope();

// Resolve services in different scopes
auto repo1 = scope1->get<IUserRepository>();
auto repo2 = scope1->get<IUserRepository>(); // Same instance as repo1
auto repo3 = scope2->get<IUserRepository>(); // Different instance from repo1 and repo2

// Clean up when done
scope1->dispose();
scope2->dispose();
```

## Integration with the Application

To use DIFramework in your application, you can follow this pattern:

1. Create a `ServiceCollection` and register all your services
2. Build a `DIFramework` from the service collection
3. Resolve your application entry point and run it

```cpp
int main() {
    try {
        // Create a service collection
        ServiceCollection services;

        // Register all services
        services.addSingleton<ITextBuffer>([]() { 
            auto buffer = std::make_shared<TextBuffer>();
            buffer->addLine(""); // Initialize with an empty line
            return buffer;
        });

        services.addSingleton<ISyntaxHighlightingManager, SyntaxHighlightingManager>();
        services.addSingleton<ICommandManager, CommandManager>();

        // Register application services with dependencies
        services.addSingleton<IEditor>([](std::shared_ptr<DIFramework> provider) { 
            auto textBuffer = provider->get<ITextBuffer>();
            auto syntaxHighlighter = provider->get<ISyntaxHighlightingManager>();
            auto commandManager = provider->get<ICommandManager>();
            
            return std::make_shared<Editor>(textBuffer, syntaxHighlighter, commandManager);
        });

        services.addSingleton<IApplication>([](std::shared_ptr<DIFramework> provider) { 
            auto editor = provider->get<IEditor>();
            return std::make_shared<Application>(editor);
        });

        // Build the service provider
        auto serviceProvider = services.buildServiceProvider();

        // Resolve the application and run it
        auto app = serviceProvider->get<IApplication>();
        app->run();

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}
```

## Best Practices

1. **Register by Interface**: Always register services by their interfaces, not their concrete types.
2. **Use Appropriate Lifetimes**: Choose the appropriate lifetime for each service based on its usage pattern.
3. **Dispose Scopes**: Always dispose scopes when they are no longer needed to release resources.
4. **Avoid Service Locator Pattern**: Inject dependencies through constructors rather than resolving them inside methods.
5. **Keep Registration Centralized**: Register all services in a central location to make dependencies clear.
6. **Use ServiceCollection**: Prefer the fluent API of ServiceCollection for better readability and maintainability.
7. **Handle Exceptions**: Wrap service resolution in try-catch blocks to handle exceptions gracefully.
8. **Avoid Circular Dependencies**: Design your services to avoid circular dependencies.

## Advanced Scenarios

### Factory Registration with Parameters

```cpp
// Register a factory that takes parameters
framework->registerFactory<IUserService>([](std::string userName) {
    return std::make_shared<UserService>(userName);
});

// Resolve with parameters
auto userService = framework->get<IUserService>("John");
```

### Decorator Pattern

```cpp
// Register the base service
framework->registerFactory<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
});

// Register the decorator
framework->registerFactory<ILogger>([framework]() {
    auto baseLogger = framework->get<ILogger>();
    return std::make_shared<TimestampLoggerDecorator>(baseLogger);
});
```

### Conditional Registration

```cpp
// Register services conditionally
if (useRemoteDataSource) {
    framework->registerFactory<IDataSource>([]() {
        return std::make_shared<RemoteDataSource>();
    });
} else {
    framework->registerFactory<IDataSource>([]() {
        return std::make_shared<LocalDataSource>();
    });
}
```

## Troubleshooting

### Common Issues and Solutions

1. **Service Not Found**: Ensure the service is registered before resolving it.
2. **Circular Dependencies**: Restructure your services to break circular dependencies.
3. **Memory Leaks**: Make sure to dispose scopes and use appropriate lifetimes.
4. **Thread Safety Issues**: Be careful with singleton services in multithreaded environments.

### Debugging Tips

1. Enable logging in the DIFramework to see what's happening.
2. Check registration and resolution order for potential issues.
3. Use the DIFrameworkTest as a reference for correct usage patterns. 