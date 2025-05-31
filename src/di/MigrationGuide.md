# Dependency Injection Framework Migration Guide

This guide explains how to use the simplified DI framework and how to migrate from the legacy interface to the modern interface.

## Overview

Our DI framework provides two interfaces for dependency resolution:

1. **Modern Interface**: Uses `get<T>()` with simpler factory registration
2. **Legacy Interface**: Uses `resolve<T>()` with factories that may reference the injector

The modern interface is recommended for all new code, while the legacy interface is maintained for backward compatibility.

## Using the Modern Interface

The modern interface uses `get<T>()` to resolve dependencies and focuses on constructor-based dependency injection.

### Registering Dependencies

```cpp
// Register a service with no dependencies
injector.registerFactory<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
});

// Register a service with dependencies resolved at registration time
auto config = loadConfiguration();
injector.registerFactory<IDatabase>([config]() {
    return std::make_shared<Database>(config.dbConnectionString);
});

// Register a service with dependencies from other factories
injector.registerFactory<IUserService>([]() {
    // Get dependencies from other sources - not from the injector
    auto logger = createLogger();
    auto db = createDatabase();
    return std::make_shared<UserService>(logger, db);
});
```

### Resolving Dependencies

```cpp
// Resolve a dependency
auto logger = injector.get<ILogger>();
logger->log("Application started");

// Use resolved dependencies
auto userService = injector.get<IUserService>();
userService->createUser("john.doe", "password123");
```

## Using the Legacy Interface

The legacy interface uses `resolve<T>()` to resolve dependencies and allows factories to access the injector during instantiation.

### Registering Dependencies

```cpp
// Register a service that needs to resolve its own dependencies
injector.registerFactory<IUserService>([](Injector& inj) {
    auto logger = inj.resolve<ILogger>();
    auto db = inj.resolve<IDatabase>();
    return std::make_shared<UserService>(logger, db);
});
```

### Resolving Dependencies

```cpp
// Resolve a dependency
auto userService = injector.resolve<IUserService>();
userService->createUser("john.doe", "password123");
```

## Migration Strategy

To migrate from the legacy interface to the modern interface:

1. **Step 1**: Update service resolution in new code
   ```cpp
   // Before
   auto logger = injector.resolve<ILogger>();
   
   // After
   auto logger = injector.get<ILogger>();
   ```

2. **Step 2**: Update factory registrations where possible
   ```cpp
   // Before (with injector dependency)
   injector.registerFactory<IUserService>([](Injector& inj) {
       auto logger = inj.resolve<ILogger>();
       auto db = inj.resolve<IDatabase>();
       return std::make_shared<UserService>(logger, db);
   });
   
   // After (using parameter capture instead)
   auto logger = injector.get<ILogger>();
   auto db = injector.get<IDatabase>();
   injector.registerFactory<IUserService>([logger, db]() {
       return std::make_shared<UserService>(logger, db);
   });
   ```

3. **Step 3**: Refactor complex dependency chains to use explicit registration
   
   Instead of having each service resolve its dependencies, resolve them at registration time.

4. **Step 4**: Update tests to use the modern interface
   
   Update tests to use `get<T>()` instead of `resolve<T>()` when testing service resolution.

## Best Practices

1. **Use Constructor Injection**: Pass dependencies through constructors rather than resolving them inside methods.

2. **Register Related Services Together**: Use a module pattern to register related services together.

3. **Resolve Early**: Resolve dependencies at startup or registration time when possible.

4. **Keep the Container Out of Business Logic**: The injector should only be used at composition roots, not in business logic.

5. **Prefer Explicit Dependencies**: Make dependencies explicit in method signatures rather than hiding them.

## Example: Complete Migration

### Before

```cpp
// Registration
injector.registerFactory<IUserRepository>([](Injector& inj) {
    auto db = inj.resolve<IDatabase>();
    return std::make_shared<UserRepository>(db);
});

injector.registerFactory<IAuthService>([](Injector& inj) {
    auto logger = inj.resolve<ILogger>();
    auto userRepo = inj.resolve<IUserRepository>();
    return std::make_shared<AuthService>(logger, userRepo);
});

// Usage
auto authService = injector.resolve<IAuthService>();
bool isValid = authService->validateCredentials("username", "password");
```

### After

```cpp
// Registration
injector.registerFactory<IUserRepository>([]() {
    // Dependencies resolved at registration time
    auto db = databaseProvider.getDatabase();
    return std::make_shared<UserRepository>(db);
});

injector.registerFactory<IAuthService>([]() {
    // Dependencies resolved at registration time
    auto logger = LoggerFactory::createLogger("AuthService");
    auto userRepo = injector.get<IUserRepository>();
    return std::make_shared<AuthService>(logger, userRepo);
});

// Usage
auto authService = injector.get<IAuthService>();
bool isValid = authService->validateCredentials("username", "password");
```

## Conclusion

The migration to the modern interface can be done gradually. Both interfaces will continue to work together, allowing for a smooth transition. The key is to start using `get<T>()` for new code and gradually update existing code as it's modified. 