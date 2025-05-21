# Dependency Injection Framework for AI-First TextEditor

This document outlines the design and implementation of the dependency injection framework for the AI-First TextEditor project.

## 1. Introduction & Motivation

The AI-First TextEditor project has evolved with various interconnected components including an AI Agent Orchestrator, Text Buffers, Workspace Management, and UI components. As we move into Phase 3 of our development roadmap, we're implementing a dependency injection framework to enhance testability, modularity, and maintainability.

### Key Goals

- **Improved Testability**: Enable easy mocking/stubbing of dependencies in unit tests to isolate components.
- **Better Modularity**: Each component only declares what it needs without pulling in transitive dependencies.
- **Reduced Coupling**: Move dependency creation logic out of business classes.
- **Enhanced Configurability**: Allow changing concrete implementations without modifying code.
- **Clear Ownership & Lifetime Management**: Ensure proper RAII-based resource management.

## 2. Requirements & Non-Goals

### Requirements

1. Provide a simple, C++17-compatible DI container.
2. Support constructor injection as the primary injection style.
3. Support different lifetime scopes: Singleton vs. Transient.
4. Enable easy testing through dependency replacement.
5. Support factory methods for complex initialization.

### Non-Goals

1. **Runtime Dynamic Configuration**: The container is configured at startup, not dynamically at runtime.
2. **Annotation-Based Injection**: Unlike Java Spring, we won't require annotations/decorators.
3. **Ultra-Fast Performance**: While performance is important, clarity and testability take precedence for this internal tool.
4. **Thread Safety**: The container itself is not designed to be thread-safe. Registrations should be made at startup.

## 3. Architecture

The DI framework consists of these main components:

### 3.1 Interfaces

- **IDependencyResolver**: Core interface for resolving types
  ```cpp
  struct IDependencyResolver {
      virtual std::shared_ptr<void> resolveType(const std::type_index& type) = 0;
      
      template<typename T>
      std::shared_ptr<T> resolve();
  };
  ```

### 3.2 Main Container/Injector 

- **Injector**: The central container that implements IDependencyResolver
  ```cpp
  class Injector : public IDependencyResolver {
  public:
      // Registration APIs
      template<typename Interface, typename Implementation>
      void registerType(Lifetime lifetime = Lifetime::Transient);
      
      template<typename Interface>
      void registerInstance(std::shared_ptr<Interface> instance);
      
      template<typename Interface>
      void registerFactory(
          std::function<std::shared_ptr<Interface>(Injector&)> factory,
          Lifetime lifetime = Lifetime::Transient
      );
      
      // Resolution API
      std::shared_ptr<void> resolveType(const std::type_index& type) override;
      
      // Other helper methods
      Injector createChildInjector() const;
      
      template<typename Interface>
      bool isRegistered() const;
  };
  ```

### 3.3 Type Traits & Constructor Injection

The framework uses C++ template metaprogramming to detect constructor signatures and inject dependencies:

```cpp
// TypeTraits.hpp
namespace detail {
    // Specialize for different constructor patterns
    template<typename T, typename = void>
    struct ConstructorArguments;
    
    // Default constructor
    template<typename T>
    struct ConstructorArguments<T, std::enable_if_t<std::is_default_constructible_v<T>>>;
    
    // 1-argument constructor
    template<typename T, typename Arg1>
    struct ConstructorArguments<T, 
        std::enable_if_t<has_1arg_ctor<T, std::shared_ptr<Arg1>>::value>>;
    
    // 2-argument and 3-argument constructors follow the same pattern
}
```

### 3.4 Lifetime Management

The framework supports two scopes:

1. **Singleton**: One shared instance for the life of the container.
2. **Transient**: A new instance is created on every resolve.

## 4. Integration with Existing Components

### 4.1 Editor Integration

The current `Editor` class likely creates many of its dependencies directly. With the DI framework:

```cpp
// Without DI:
class Editor {
public:
    Editor() {
        _textBuffer = new TextBuffer();
        _fileSystem = new FileSystem();
        // ... more manual creation
    }
};

// With DI:
class Editor {
public:
    Editor(std::shared_ptr<ITextBuffer> textBuffer,
           std::shared_ptr<IFileSystem> fileSystem,
           std::shared_ptr<ILogger> logger)
        : _textBuffer(textBuffer),
          _fileSystem(fileSystem),
          _logger(logger) {}
};
```

### 4.2 AIAgentOrchestrator Integration

Similarly, the `AIAgentOrchestrator` would be refactored:

```cpp
// With DI:
class AIAgentOrchestrator {
public:
    AIAgentOrchestrator(
        std::shared_ptr<IOpenAIClient> apiClient,
        std::shared_ptr<IUIModel> uiModel,
        std::shared_ptr<IWorkspaceManager> workspaceManager,
        std::shared_ptr<ILogger> logger)
        : _apiClient(apiClient),
          _uiModel(uiModel),
          _workspaceManager(workspaceManager),
          _logger(logger) {}
};
```

### 4.3 Application Startup

The main application startup would configure the container:

```cpp
int main() {
    di::Injector injector;
    
    // Register core services
    injector.registerType<ILogger, FileLogger>(di::Lifetime::Singleton);
    injector.registerType<IFileSystem, FileSystem>(di::Lifetime::Singleton);
    injector.registerType<ITextBuffer, TextBuffer>(di::Lifetime::Transient);
    injector.registerType<ISyntaxHighlighter, SyntaxHighlighter>(di::Lifetime::Singleton);
    injector.registerType<IOpenAIClient, OpenAIClient>(di::Lifetime::Singleton);
    injector.registerType<IWorkspaceManager, WorkspaceManager>(di::Lifetime::Singleton);
    injector.registerType<IUIModel, UIModel>(di::Lifetime::Singleton);
    
    // Register the main components
    injector.registerType<AIAgentOrchestrator, AIAgentOrchestrator>(di::Lifetime::Singleton);
    injector.registerType<Editor, Editor>(di::Lifetime::Transient);
    
    // Start the application
    auto editor = injector.resolve<Editor>();
    editor->run();
    
    return 0;
}
```

## 5. Testing Strategy

The DI framework enables much easier testing by allowing mock replacement:

```cpp
TEST(EditorTests, OpenFile_FileExists_ReturnsTrue) {
    // Setup
    di::Injector injector;
    auto mockLogger = std::make_shared<MockLogger>();
    auto mockFileSystem = std::make_shared<MockFileSystem>();
    auto mockTextBuffer = std::make_shared<MockTextBuffer>();
    
    // Configure mocks
    EXPECT_CALL(*mockFileSystem, fileExists("test.txt")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile("test.txt")).WillOnce(Return("test content"));
    EXPECT_CALL(*mockTextBuffer, insertLine(0, "test content")).Times(1);
    
    // Register mocks in container
    injector.registerInstance<ILogger>(mockLogger);
    injector.registerInstance<IFileSystem>(mockFileSystem);
    injector.registerInstance<ITextBuffer>(mockTextBuffer);
    
    // Resolve the system under test
    auto editor = injector.resolve<Editor>();
    
    // Act
    bool result = editor->openFile("test.txt");
    
    // Assert
    EXPECT_TRUE(result);
}
```

## 6. Roll-Out & Migration Strategy

The migration to the DI framework will proceed in phases:

1. **Phase 1**: Create the DI framework itself.
2. **Phase 2**: Create interfaces for all major components.
3. **Phase 3**: Refactor one component at a time to use constructor injection.
   - Start with the `Editor` class or another central component.
   - Gradually refactor dependent components.
4. **Phase 4**: Update tests to use the DI container for mock injection.
5. **Phase 5**: Integrate DI container into the main application.

## 7. Conclusion

The dependency injection framework will significantly improve the testability, modularity, and maintainability of the AI-First TextEditor. By decoupling component creation from component use, the codebase will be easier to extend with new features, test in isolation, and maintain for the future.

## 8. References

- C++17 Standard
- [Google Test Mocking Framework](https://github.com/google/googletest)
- [Dependency Injection in C++](https://github.com/boost-ext/di) 