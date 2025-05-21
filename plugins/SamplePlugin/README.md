# SamplePlugin

A demonstration plugin for the AI-First TextEditor, showcasing the plugin architecture capabilities.

## Features

- Registers a command (`sample.helloWorld`) that displays a greeting message
- Adds a menu item to the "Plugins" menu that triggers the command
- Demonstrates the use of various editor services:
  - Command Registry
  - UI Extension Registry
  - Event Registry
  - Error Reporter

## Building

The plugin is automatically built as part of the main project when you build the AI-First TextEditor. The compiled plugin will be placed in the `build/plugins` directory.

## Usage

1. Start the AI-First TextEditor application
2. The plugin will be automatically loaded if it's in the `plugins` directory
3. Access the "Plugins" menu in the main menu bar
4. Click on "Hello World" to trigger the command
5. A notification message will appear

## Implementation Details

This plugin demonstrates:

- Using the `IMPLEMENT_PLUGIN` macro from the PluginAPI
- Implementing the IPlugin interface
- Registering and unregistering commands
- Creating UI elements
- Publishing events
- Error reporting and handling
- Resource cleanup during shutdown

## Development Notes

When developing your own plugins, use this sample as a starting point and follow these best practices:

1. Always check for null pointers from service retrievals
2. Clean up all registered resources during shutdown
3. Use structured error handling with try-catch blocks
4. Provide clear, descriptive error messages
5. Use meaningful plugin and command identifiers 