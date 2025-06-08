# Dependency Injection Framework

## Overview

This directory contains a lightweight, flexible Dependency Injection (DI) framework for C++. The framework provides a simple way to manage dependencies between components in your application, making it easier to write modular, testable code.

## Key Features

- **Dual Interface**: Supports both modern (`get<T>()`) and legacy (`resolve<T>()`) interfaces for flexibility and backward compatibility
- **Type-Safe**: Uses templates and static typing to ensure type safety at compile time
- **Flexible Registration**: Supports multiple factory registration styles for different use cases
- **Advanced Lifetime Management**: Support for singleton, transient, and scoped lifetimes with thread safety
- **Resource Cleanup**: Automatic disposal of services that implement the `IDisposable` interface
- **Extensible**: Can be enhanced with additional patterns like singletons, decorators, and more
- **Header-Only Core**: The core functionality is header-only for easy integration
- **Minimal Dependencies**: Depends only on the C++ standard library

## Files

- `Injector.hpp` - Core DI container implementation
- `CoreModule.hpp` - Example module for registering core services
- `DIPatterns.hpp` - Advanced patterns like singletons and scoped lifetimes
- `LifetimeManager.hpp` - Thread-safe lifetime management with support for singletons, transient, and scoped services
- `MigrationGuide.md` - Guide for migrating from legacy to modern interface
- `LifetimeManager.md` - Documentation for the advanced lifetime management system
- `AppDebugLog.h` - Simple logging utilities for the DI framework

## Quick Start

```cpp
#include "di/Injector.hpp"
#include "di/CoreModule.hpp"

using namespace di;

// Define an interface
class IGreeter {
public:
    virtual ~IGreeter() = default;
    virtual std::string greet(const std::string& name) = 0;
};

// Implement the interface
class SimpleGreeter : public IGreeter {
public:
    SimpleGreeter(std::shared_ptr<ISimpleLogger> logger) : logger_(logger) {}
    
    std::string greet(const std::string& name) override {
        logger_->log("Greeting: " + name);
        return "Hello, " + name + "!";
    }
    
private:
    std::shared_ptr<ISimpleLogger> logger_;
};

int main() {
    // Create the injector
    Injector injector;
    
    // Configure core services
    CoreModule::configure(injector);
    
    // Register your own services
    injector.registerFactory<IGreeter>([]() {
        auto logger = std::make_shared<ConsoleLogger>();
        return std::make_shared<SimpleGreeter>(logger);
    });
    
    // Resolve and use services
    auto greeter = injector.get<IGreeter>();
    std::cout << greeter->greet("World") << std::endl;
    
    return 0;
}
```

## Advanced Usage

### Service Lifetimes

For advanced lifetime management, use the `LifetimeInjector`:

```cpp
#include "di/LifetimeManager.hpp"

using namespace di;
using namespace di::lifetime;

// Create a lifetime injector
LifetimeInjector injector;

// Register singleton service (created once for the application)
injector.registerFactory<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
}, ServiceLifetime::Singleton);

// Register transient service (created new each time)
injector.registerFactory<IUserService>([]() {
    return std::make_shared<UserService>();
}, ServiceLifetime::Transient);

// Register scoped service (created once per scope)
injector.registerFactory<IRequestHandler>([]() {
    return std::make_shared<RequestHandler>();
}, ServiceLifetime::Scoped);

// Create a scope
auto scope = injector.createScope();

// Get scoped services
auto handler = scope->get<IRequestHandler>();
// ...use the handler...

// Dispose the scope when done
scope->dispose();
```

For more advanced patterns, see the `DIPatterns.hpp` file, `MigrationGuide.md`, and `LifetimeManager.md` for detailed examples and documentation.

## Testing

The framework includes comprehensive test coverage in:

- `tests/StandaloneDITest.cpp` - Tests for the standalone DI implementation
- `tests/SimpleDITest.cpp` - Tests for the simplified DI implementation
- `tests/LifetimeManagerTest.cpp` - Tests for the advanced lifetime management system

## Design Philosophy

This framework is designed with the following principles in mind:

1. **Simplicity**: Easy to understand and use, with a minimal learning curve
2. **Flexibility**: Adaptable to different usage patterns and requirements
3. **Maintainability**: Clear code structure and good documentation
4. **Compatibility**: Supports both modern and legacy code
5. **Performance**: Minimal runtime overhead
6. **Thread Safety**: Core operations are thread-safe, especially in the lifetime management system

## Contributing

When contributing to this framework, please follow these guidelines:

1. Keep the core functionality in `Injector.hpp` simple and focused
2. Add advanced patterns to `DIPatterns.hpp` and `LifetimeManager.hpp`
3. Maintain backward compatibility with the legacy interface
4. Add tests for new features
5. Update documentation for significant changes 