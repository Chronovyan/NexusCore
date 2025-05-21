# Dependency Injection Interfaces

This document outlines the interfaces created for the AI-First TextEditor's Dependency Injection framework.

## Core Interfaces

We have identified and created interfaces for the following key components of the editor:

### 1. `ITextBuffer`

The `ITextBuffer` interface defines the contract for components that store and manipulate text content. It includes methods for:

- Basic line operations (add, insert, delete, replace)
- Multi-line operations
- Text accessors
- Character and string manipulations
- File operations

This interface is crucial for decoupling the text storage implementation from components that need to work with text.

### 2. `ICommandManager`

The `ICommandManager` interface defines the contract for components that handle command pattern execution with undo/redo capabilities. It includes methods for:

- Executing commands
- Adding commands to the undo stack
- Undo/redo operations
- Querying command stack state

This interface is important for allowing different command management strategies while maintaining a consistent API.

### 3. `IWorkspaceManager`

The `IWorkspaceManager` interface defines the contract for components that handle file operations within a workspace directory. It includes methods for:

- Reading and writing files
- Checking file existence
- Listing files
- Creating and managing directories

This interface allows for easy mocking of file operations in tests and potentially supporting different storage backends.

### 4. `ISyntaxHighlightingManager`

The `ISyntaxHighlightingManager` interface defines the contract for components that manage syntax highlighting for text buffers. It includes methods for:

- Handling highlighting styles
- Managing caching
- Setting visible ranges
- Configuring highlighting behavior

This interface enables separation of the highlighting logic from the rest of the editor.

### 5. `IErrorReporter`

The `IErrorReporter` interface defines the contract for components that handle logging and error reporting. It includes methods for:

- Logging at different severity levels
- Managing log destinations
- Configuring asynchronous logging
- Tracking retry operations

This interface allows for different logging implementations and better testability.

### 6. `IEditorServices`

The `IEditorServices` interface acts as a central registry for accessing all editor services. It provides methods to obtain references to each of the core services:

- Text Buffer
- Command Manager
- Workspace Manager
- Syntax Highlighting Manager
- Error Reporter

This interface simplifies dependency management by providing a single point of access to all services.

## Using with Dependency Injection

These interfaces will be used with our DI framework in the following ways:

### Component Registration

During application startup, concrete implementations of each interface will be registered with the `Injector`:

```cpp
// Create the injector
di::Injector injector;

// Register interface implementations
injector.registerType<ITextBuffer, TextBuffer>();
injector.registerType<ICommandManager, CommandManager>();
injector.registerType<IWorkspaceManager, WorkspaceManager>(di::Lifetime::Singleton);
injector.registerType<ISyntaxHighlightingManager, SyntaxHighlightingManager>();
injector.registerType<IErrorReporter, ErrorReporter>(di::Lifetime::Singleton);

// Register the editor services as a facade for all components
injector.registerType<IEditorServices, EditorServices>(di::Lifetime::Singleton);
```

### Constructor Injection

Components that need access to services will declare their dependencies in their constructors:

```cpp
class Editor {
public:
    // Constructor injection
    Editor(
        std::shared_ptr<ITextBuffer> textBuffer,
        std::shared_ptr<ICommandManager> commandManager,
        std::shared_ptr<ISyntaxHighlightingManager> highlightingManager
    ) : textBuffer_(textBuffer),
        commandManager_(commandManager),
        highlightingManager_(highlightingManager) {
        // Initialize editor using injected dependencies
    }

private:
    std::shared_ptr<ITextBuffer> textBuffer_;
    std::shared_ptr<ICommandManager> commandManager_;
    std::shared_ptr<ISyntaxHighlightingManager> highlightingManager_;
};
```

### Facade Injection

Some components may prefer to receive the entire `IEditorServices` rather than individual components:

```cpp
class AIAgentOrchestrator {
public:
    // Single facade injection
    explicit AIAgentOrchestrator(std::shared_ptr<IEditorServices> services)
        : services_(services) {
        // Initialize using the services
    }
    
    void processRequest(const std::string& request) {
        // Access needed services as required
        auto workspaceManager = services_->getWorkspaceManager();
        auto errorReporter = services_->getErrorReporter();
        
        // Use the services
        // ...
    }

private:
    std::shared_ptr<IEditorServices> services_;
};
```

### Application Startup

The main function will create the injector, register components, and resolve the main editor instance:

```cpp
int main() {
    // Create and configure the injector
    di::Injector injector;
    
    // Register all components (as shown above)
    
    // Create the main editor instance
    auto editor = injector.resolve<Editor>();
    
    // Run the editor
    return editor->run();
}
```

## Next Steps

1. **Create Adapter Classes**: Update existing concrete implementations to implement the new interfaces
2. **Update Component Constructors**: Modify components to use constructor injection
3. **Create Editor Services Implementation**: Implement the `EditorServices` class
4. **Update Application Startup**: Modify the main function to use the DI container
5. **Update Tests**: Refactor tests to use mock implementations of the interfaces 