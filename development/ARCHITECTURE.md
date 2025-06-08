# Architecture Overview

This document provides an overview of the AI-First TextEditor's architecture, including its key components, their interactions, and the design principles guiding the project.

## Architectural Principles

The AI-First TextEditor is built on the following architectural principles:

1. **Separation of Concerns** - Components have clearly defined responsibilities
2. **Command Pattern** - Editing operations use commands for undo/redo support
3. **Observer Pattern** - UI components observe and react to model changes
4. **Composition over Inheritance** - Prefer composition for flexibility
5. **RAII (Resource Acquisition Is Initialization)** - Manage resources through object lifecycles

## High-Level Architecture

The editor is organized into the following high-level layers:

```
┌───────────────────────────────────────────────────┐
│                   UI Layer                         │
│  (ImGui-based UI components, rendering, input)     │
└───────────────────────────────────────────────────┘
                        ▲
                        │
                        ▼
┌───────────────────────────────────────────────────┐
│                  Model Layer                       │
│  (Editor, TextBuffer, CommandManager, UIModel)     │
└───────────────────────────────────────────────────┘
                        ▲
                        │
                        ▼
┌───────────────────────────────────────────────────┐
│                 Service Layer                      │
│  (WorkspaceManager, SyntaxHighlighter, AI)         │
└───────────────────────────────────────────────────┘
                        ▲
                        │
                        ▼
┌───────────────────────────────────────────────────┐
│                 Utility Layer                      │
│  (ErrorReporter, Configuration, File I/O)          │
└───────────────────────────────────────────────────┘
```

## Core Components

### UI Layer

The UI Layer handles user interaction, rendering, and visual representation.

- **EditorUI** - Main editor UI rendering and control
- **ImGuiWrapper** - Abstracts ImGui integration details
- **UIModel** - Maintains UI state and selection information

The UI Layer observes changes in the Model Layer and updates the view accordingly.

### Model Layer

The Model Layer contains the core editor data structures and business logic.

- **Editor** - Central component coordinating editing operations
- **TextBuffer** - Manages text content as lines
- **CommandManager** - Tracks command history for undo/redo
- **CursorManager** - Handles cursor position and movement

### Service Layer

The Service Layer provides specialized functionality to the editor.

- **SyntaxHighlightingManager** - Manages syntax highlighting with caching
- **AIAgentOrchestrator** - Coordinates AI features and conversation flows
- **WorkspaceManager** - Handles file system operations and workspace management

### Utility Layer

The Utility Layer provides common functionality used across the application.

- **ErrorReporter** - Centralized logging and error handling
- **Configuration** - Application settings and preferences
- **FileIO** - Low-level file operations

## Key Component Interactions

### Text Editing Flow

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│   User   │     │EditorUI  │     │  Editor  │     │TextBuffer│
└────┬─────┘     └────┬─────┘     └────┬─────┘     └────┬─────┘
     │ Key Press      │                │                │
     │───────────────>│                │                │
     │                │ Create Command │                │
     │                │───────────────>│                │
     │                │                │ Modify Buffer  │
     │                │                │───────────────>│
     │                │                │  Update State  │                
     │                │<───────────────│                │
     │ Render Update  │                │                │
     │<───────────────│                │                │
     │                │                │                │
```

### AI Interaction Flow

```
┌──────────┐     ┌──────────┐     ┌───────────────┐     ┌──────────┐
│   User   │     │  EditorUI│     │AIOrchestrator │     │OpenAI API│
└────┬─────┘     └────┬─────┘     └───────┬───────┘     └────┬─────┘
     │ Submit Prompt  │                   │                  │
     │───────────────>│                   │                  │
     │                │ Forward Prompt    │                  │
     │                │──────────────────>│                  │
     │                │                   │ API Request      │
     │                │                   │─────────────────>│
     │                │                   │                  │
     │                │                   │ API Response     │
     │                │                   │<─────────────────│
     │                │ Update UI State   │                  │
     │                │<──────────────────│                  │
     │ Display Result │                   │                  │
     │<───────────────│                   │                  │
     │                │                   │                  │
```

## Command Pattern Implementation

The editor uses the Command pattern for all editing operations:

```cpp
class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

class InsertTextCommand : public EditorCommand {
private:
    Editor& editor_;
    std::string text_;
    Position insertPosition_;
    // ...
    
public:
    InsertTextCommand(Editor& editor, const std::string& text, Position pos);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
};
```

This pattern provides several benefits:
1. **Undo/Redo Support** - Each command can be undone/redone
2. **Command Composition** - Complex operations can be composed of simpler commands
3. **Separation of Concerns** - Action execution is separated from invocation

## Observer Pattern Implementation

The editor uses the Observer pattern for UI updates:

```cpp
class TextBufferObserver {
public:
    virtual ~TextBufferObserver() = default;
    virtual void onTextChanged(int lineStart, int lineEnd) = 0;
    virtual void onLineInserted(int line) = 0;
    virtual void onLineDeleted(int line) = 0;
};

class SyntaxHighlightingManager : public TextBufferObserver {
    // ...
    void onTextChanged(int lineStart, int lineEnd) override;
    void onLineInserted(int line) override;
    void onLineDeleted(int line) override;
};
```

This pattern allows components to react to changes without tight coupling.

## Threading Model

The editor uses a multi-threaded architecture:

1. **Main Thread** - UI rendering and user input
2. **Syntax Highlighting Thread** - Background highlighting of visible content
3. **AI Processing Thread** - API communication and response processing
4. **File I/O Thread** - Asynchronous file operations
5. **Logging Thread** - Asynchronous log processing

Thread synchronization is primarily achieved through:
- Mutexes for shared state access
- Condition variables for signaling
- Thread-safe queues for work items

## Error Handling Strategy

The editor employs a comprehensive error handling strategy:

1. **Exception Hierarchy** - Domain-specific exception classes
2. **Centralized Logging** - Through the ErrorReporter class
3. **Recovery Mechanisms** - Graceful handling of recoverable errors
4. **User Feedback** - Clear error messages in the UI

## Planned Architectural Improvements

1. **Dependency Injection** - Implementing a DI container for better testability
2. **Plugin Architecture** - Supporting user extensions through a plugin system
3. **Improved Concurrency Model** - More explicit thread ownership and task-based parallelism
4. **Component Lifecycle Management** - Formalized component initialization and shutdown 