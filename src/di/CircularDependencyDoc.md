# Circular Dependency Detection in the DI Framework

## Overview

The DI Framework now includes a robust circular dependency detection mechanism that helps identify and resolve dependency cycles in your application. This document explains how the detection mechanism works, why circular dependencies are problematic, and provides strategies to resolve them.

## How Circular Dependency Detection Works

### Detection Mechanism

The framework uses a thread-local tracking system to detect circular dependencies during service resolution:

1. **Resolution Stack Tracking**: When a service is being resolved, its type is added to a thread-local stack.
2. **Cycle Detection**: Before resolving a dependency, the framework checks if that type is already in the resolution stack.
3. **Maximum Depth Protection**: To prevent undetected cycles or excessive recursion, the framework also enforces a maximum resolution depth.
4. **Graceful Handling**: When a cycle is detected, the framework returns `nullptr` instead of entering an infinite loop, and logs detailed information about the dependency cycle.

### Implementation Details

The key components of the cycle detection system are:

- `thread_local` stack to track types currently being resolved
- Automatic cleanup using RAII pattern with guard objects
- Detailed logging of the resolution stack when a cycle is detected
- Suggestions for resolving the detected circular dependency

## Why Circular Dependencies Are Problematic

Circular dependencies occur when two or more services depend on each other, directly or indirectly. They cause several issues:

1. **Infinite Recursion**: Without detection, service resolution would never complete, causing stack overflow or program crashes.
2. **Initialization Order Problems**: Services in a circular dependency may not be fully initialized when used by other services.
3. **Tight Coupling**: Circular dependencies often indicate a design issue where components are too tightly coupled.
4. **Testing Difficulties**: Components in circular dependencies are harder to test in isolation.

## Common Circular Dependency Scenarios

### Direct Circular Dependencies

The simplest form is a direct cycle between two services:

```cpp
class ServiceA {
public:
    ServiceA(std::shared_ptr<ServiceB> b) : b_(b) {}
private:
    std::shared_ptr<ServiceB> b_;
};

class ServiceB {
public:
    ServiceB(std::shared_ptr<ServiceA> a) : a_(a) {}
private:
    std::shared_ptr<ServiceA> a_;
};
```

### Indirect Circular Dependencies

More complex cycles involve multiple services:

```cpp
class ServiceA {
public:
    ServiceA(std::shared_ptr<ServiceB> b) : b_(b) {}
private:
    std::shared_ptr<ServiceB> b_;
};

class ServiceB {
public:
    ServiceB(std::shared_ptr<ServiceC> c) : c_(c) {}
private:
    std::shared_ptr<ServiceC> c_;
};

class ServiceC {
public:
    ServiceC(std::shared_ptr<ServiceA> a) : a_(a) {}
private:
    std::shared_ptr<ServiceA> a_;
};
```

## Strategies to Resolve Circular Dependencies

### 1. Redesign the Dependencies

The best solution is to refactor your code to eliminate the circular dependency:

- Extract common functionality into a separate service that both dependent services can use
- Apply the Single Responsibility Principle to ensure services have clear, focused roles
- Consider if the circular dependency indicates a design issue in your domain model

### 2. Use Property Injection

Instead of constructor injection, use property (setter) injection for one of the dependencies:

```cpp
class ServiceA {
public:
    ServiceA() {}
    void setServiceB(std::shared_ptr<ServiceB> b) { b_ = b; }
private:
    std::shared_ptr<ServiceB> b_;
};

class ServiceB {
public:
    ServiceB(std::shared_ptr<ServiceA> a) : a_(a) {}
private:
    std::shared_ptr<ServiceA> a_;
};
```

### 3. Use Interface Abstraction

Refactor your code to depend on interfaces rather than concrete types:

```cpp
class IServiceA {
public:
    virtual ~IServiceA() = default;
    virtual void doSomething() = 0;
};

class IServiceB {
public:
    virtual ~IServiceB() = default;
    virtual void doSomethingElse() = 0;
};

class ServiceA : public IServiceA {
public:
    ServiceA(std::shared_ptr<IServiceB> b) : b_(b) {}
    void doSomething() override { /* implementation */ }
private:
    std::shared_ptr<IServiceB> b_;
};

class ServiceB : public IServiceB {
public:
    ServiceB(std::shared_ptr<IServiceA> a) : a_(a) {}
    void doSomethingElse() override { /* implementation */ }
private:
    std::shared_ptr<IServiceA> a_;
};
```

### 4. Lazy Initialization

For singletons or services that don't need immediate initialization:

```cpp
class ServiceA {
public:
    ServiceA(std::function<std::shared_ptr<ServiceB>()> bFactory) 
        : bFactory_(bFactory) {}
    
    void doSomething() {
        // Get ServiceB only when needed
        auto b = bFactory_();
        // Use b
    }
private:
    std::function<std::shared_ptr<ServiceB>()> bFactory_;
};
```

### 5. Service Locator Pattern

While generally not recommended due to hidden dependencies, the service locator pattern can break circular dependencies:

```cpp
class ServiceLocator {
public:
    static std::shared_ptr<ServiceA> getServiceA() { return serviceA_; }
    static std::shared_ptr<ServiceB> getServiceB() { return serviceB_; }
    
    static void setServiceA(std::shared_ptr<ServiceA> service) { serviceA_ = service; }
    static void setServiceB(std::shared_ptr<ServiceB> service) { serviceB_ = service; }

private:
    static std::shared_ptr<ServiceA> serviceA_;
    static std::shared_ptr<ServiceB> serviceB_;
};

// Then ServiceB doesn't need ServiceA injected directly
class ServiceB {
public:
    ServiceB() {}
    
    void doSomethingWithA() {
        auto serviceA = ServiceLocator::getServiceA();
        // Use serviceA
    }
};
```

## Best Practices

1. **Use Dependency Injection Properly**: Inject only what is necessary, and consider whether a dependency is truly required.
2. **Follow SOLID Principles**: Especially the Single Responsibility Principle and Interface Segregation Principle.
3. **Favor Composition Over Inheritance**: Composition generally leads to more flexible designs with fewer dependency issues.
4. **Use Interfaces**: Depend on abstractions rather than concrete implementations.
5. **Consider Domain-Driven Design**: A well-designed domain model can naturally avoid many circular dependency issues.

## Conclusion

Circular dependencies often indicate architectural issues in your application. While the DI framework now detects and prevents infinite recursion caused by these dependencies, it's best to refactor your code to eliminate them completely.

By applying the strategies outlined in this document, you can create more maintainable, testable, and robust applications with cleaner dependency hierarchies. 