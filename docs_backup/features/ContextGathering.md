# Context Gathering System

The Context Gathering System is a sophisticated component of the AI-First TextEditor that provides rich, relevant code context to the AI system, enabling more accurate and helpful AI suggestions.

## Overview

The system collects information about the user's current editing context, including:

- The current file and cursor position
- The symbol being edited (function, class, variable, etc.)
- Related symbols (callers, callees, base classes, derived classes, etc.)
- Related files (header/implementation pairs, imported files, etc.)
- Code snippets from relevant parts of the codebase
- Project structure information and dependencies

This information is then processed, prioritized based on relevance, and formatted into a contextual prompt that is sent to the AI system.

## Key Features

### Relevance Scoring

The system uses a sophisticated relevance scoring mechanism to prioritize the most important information:

- Symbols are scored based on their relationship to the current context
- Files are scored based on their relationship to the current file
- Code snippets are scored based on their relevance to the current editing task
- Custom scorers can be registered for project-specific relevance criteria

### Token Management

To optimize the use of limited token budgets in AI models:

- The system estimates token counts for different pieces of context
- Context is trimmed based on relevance scores when token limits are reached
- The most relevant information is always prioritized

### Customizable Context Options

The system provides flexible options for controlling context gathering:

```cpp
// Create options with custom settings
CodeContextProvider::ContextOptions options;
options.includeDefinitions = true;
options.includeReferences = true;
options.includeRelationships = true;
options.maxRelatedSymbols = 10;
options.maxRelatedFiles = 5;
options.maxCodeSnippets = 15;
options.minRelevanceScore = 0.3;
options.maxTokens = 2000;
options.scopeDepth = 2;

// Get context with custom options
CodeContext context = contextProvider.getContext(filePath, line, column, selectedText, visibleFiles, options);
```

## Usage

### Basic Usage

To use the context gathering system in its simplest form:

```cpp
// Create a context provider with a codebase index
auto codebaseIndex = std::make_shared<CodebaseIndexer>();
auto contextProvider = std::make_shared<CodeContextProvider>(codebaseIndex);

// Get context for the current editing position
CodeContext context = contextProvider.getContext(filePath, line, column, selectedText, visibleFiles);

// Generate a contextual prompt
std::string prompt = contextProvider.generateContextualPrompt(userInput, context);
```

### Registering Custom Relevance Scorers

You can register custom relevance scorers to better prioritize information for your specific project:

```cpp
// Register a custom symbol relevance scorer
contextProvider.registerSymbolRelevanceScorer([](const Symbol& symbol, const Symbol& currentSymbol) {
    // Give higher relevance to symbols in the same namespace
    if (symbol.ns == currentSymbol.ns) {
        return 0.8;
    }
    return 0.5;
});

// Register a custom file relevance scorer
contextProvider.registerFileRelevanceScorer([](const std::string& filePath, const std::string& currentFilePath) {
    // Give higher relevance to files in the same directory
    if (getDirectory(filePath) == getDirectory(currentFilePath)) {
        return 0.9;
    }
    return 0.4;
});
```

## Integration with AI System

The context gathering system integrates with the AI system through the `AIAgentOrchestrator`:

```cpp
// Create and connect the components
auto codebaseIndex = std::make_shared<CodebaseIndexer>();
auto contextProvider = std::make_shared<CodeContextProvider>(codebaseIndex);
auto aiOrchestrator = std::make_shared<AIAgentOrchestrator>(aiProvider, contextProvider);

// Enable context-aware prompts
aiOrchestrator->enableContextAwarePrompts(true);

// Update the current editing context when the cursor moves
aiOrchestrator->updateEditingContext(filePath, line, column, selectedText, visibleFiles);
```

## Customizing Context Presentation

You can customize how context is presented to the AI by overriding the `generateContextualPrompt` method:

```cpp
class CustomContextProvider : public CodeContextProvider {
public:
    CustomContextProvider(std::shared_ptr<ICodebaseIndex> codebaseIndex) 
        : CodeContextProvider(codebaseIndex) {}
    
    std::string generateContextualPrompt(const std::string& userInput, const CodeContext& context) override {
        // Custom prompt generation logic
        std::string prompt = "USER QUERY: " + userInput + "\n\n";
        prompt += "CONTEXT:\n";
        
        // Add custom context formatting
        
        return prompt;
    }
};
```

## Testing and Debugging

The `CodeContextProviderTest` program allows you to test and debug the context gathering system:

```bash
# Build and run the test program
cmake --build . --target CodeContextProviderTest
./CodeContextProviderTest /path/to/project
```

The test program provides an interactive shell for exploring the context gathering capabilities, including commands to:

- Set the current file and cursor position
- Retrieve and display context
- Generate and display contextual prompts
- Search for symbols and files
- Test different context options

## Performance Considerations

- The context gathering system relies on the codebase index, which must be built before context can be gathered
- Context gathering can be computationally intensive for large codebases
- Consider using the token management features to limit the amount of context gathered
- For very large codebases, consider limiting the scope of the context gathering to specific directories or files

## Future Enhancements

Planned enhancements to the context gathering system include:

1. Machine learning-based relevance scoring
2. User feedback mechanisms to improve context quality
3. Caching frequently used context to improve performance
4. Gathering context from external sources like documentation and online references 