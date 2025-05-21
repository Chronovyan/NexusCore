# AI-First TextEditor Plugins

This directory contains plugins for the AI-First TextEditor application. Plugins extend the functionality of the editor by adding new commands, UI elements, file handlers, and more.

## Available Plugins

- [SamplePlugin](./SamplePlugin/README.md): A demonstration plugin showcasing the basic plugin capabilities.

## Plugin Architecture

The AI-First TextEditor uses a flexible plugin system that allows for dynamic loading and unloading of plugins. Plugins are implemented as shared libraries (.dll on Windows, .so on Linux, .dylib on macOS) that export a standard interface.

### Core Components

1. **Plugin Manager**: Handles the discovery, loading, and unloading of plugins.
2. **Editor Services**: Provides access to core editor functionality through well-defined interfaces.
3. **Plugin API**: Defines the interface that all plugins must implement.

### Plugin Lifecycle

1. **Discovery**: The Plugin Manager scans designated directories for plugin shared libraries.
2. **Loading**: Valid plugins are loaded and initialized with access to the Editor Services.
3. **Operation**: Plugins register commands, UI elements, and event handlers to integrate with the editor.
4. **Unloading**: When the editor shuts down, plugins are given a chance to clean up resources.

## Creating a Plugin

To create a new plugin:

1. Create a new directory under `plugins/`.
2. Create a class that implements the `IPlugin` interface.
3. Use the `IMPLEMENT_PLUGIN` macro to export the plugin creation function.
4. Implement the required methods:
   - `getName()`
   - `getVersion()`
   - `getDescription()`
   - `initialize(std::shared_ptr<IEditorServices>)`
   - `shutdown()`
5. Create a CMakeLists.txt file to build the plugin as a shared library.
6. Add your plugin directory to `plugins/CMakeLists.txt`.

For a complete example, see the [SamplePlugin](./SamplePlugin/).

## Building Plugins

Plugins are automatically built when you build the main project. The compiled plugins will be placed in the `build/plugins` directory.

## Plugin Services

Plugins have access to the following services through the `IEditorServices` interface:

- `ICommandRegistry`: For registering and executing commands.
- `IUIExtensionRegistry`: For adding UI elements like menus and toolbars.
- `IEventRegistry`: For subscribing to and publishing events.
- `ISyntaxHighlightingRegistry`: For registering syntax highlighters.
- `IWorkspaceExtension`: For handling file types and workspace scanning.
- `IErrorReporter`: For reporting errors and information to the user.

## Best Practices

- Always check for null pointers when retrieving services.
- Clean up all registered resources during shutdown.
- Use structured error handling with try-catch blocks.
- Provide meaningful error messages for better debugging.
- Use namespaced identifiers for commands and UI elements to avoid conflicts.
- Validate inputs and handle errors gracefully. 