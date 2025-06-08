#include "../CodeContextProvider.hpp"
#include "../CodebaseIndexer.hpp"
#include "../ProjectKnowledgeBase.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace ai_editor;

// Utility function to print a divider
void printDivider() {
    std::cout << "\n" << std::string(80, '-') << "\n\n";
}

// Utility function to print a knowledge entry
void printKnowledgeEntry(const KnowledgeEntry& entry) {
    std::cout << "[" << entry.id << "] " << entry.title << " (Relevance: " << entry.relevanceScore << ")\n";
    std::cout << "Category: " << knowledgeCategoryToString(entry.category);
    
    if (!entry.customCategory.empty()) {
        std::cout << " (" << entry.customCategory << ")";
    }
    
    std::cout << "\n";
    
    if (!entry.tags.empty()) {
        std::cout << "Tags: ";
        for (size_t i = 0; i < entry.tags.size(); ++i) {
            std::cout << entry.tags[i];
            if (i < entry.tags.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "Created: " << entry.createdAt << "\n";
    if (entry.updatedAt > entry.createdAt) {
        std::cout << "Updated: " << entry.updatedAt << "\n";
    }
    
    std::cout << "\nContent:\n";
    std::cout << entry.content << "\n\n";
}

// Utility function to print a code snippet
void printSnippet(const CodeContext::ContextSnippet& snippet) {
    std::cout << "File: " << snippet.filePath << "\n";
    if (!snippet.symbolName.empty()) {
        std::cout << "Symbol: " << snippet.symbolName << "\n";
    }
    std::cout << "Lines: " << snippet.startLine << "-" << snippet.endLine << "\n";
    std::cout << "Relevance: " << snippet.relevanceScore << "\n";
    std::cout << "Content:\n";
    std::cout << snippet.content << "\n\n";
}

// Utility function to print a context
void printContext(const CodeContext& context) {
    std::cout << "Current File: " << context.currentFile << "\n";
    std::cout << "Cursor Position: Line " << context.cursorLine << ", Column " << context.cursorColumn << "\n";
    
    if (!context.selectedText.empty()) {
        std::cout << "Selected Text:\n";
        std::cout << context.selectedText << "\n";
    }
    
    if (context.currentSymbol.has_value()) {
        std::cout << "\nCurrent Symbol:\n";
        const auto& symbol = context.currentSymbol.value();
        std::cout << "Name: " << symbol.name << "\n";
        std::cout << "Type: " << static_cast<int>(symbol.type) << "\n";
        std::cout << "File: " << symbol.filePath << "\n";
        std::cout << "Line: " << symbol.line << ", Column: " << symbol.column << "\n";
    }
    
    if (!context.relatedSymbols.empty()) {
        std::cout << "\nRelated Symbols (" << context.relatedSymbols.size() << "):\n";
        for (const auto& symbol : context.relatedSymbols) {
            std::cout << "- " << symbol.name << " (" << static_cast<int>(symbol.type) << ") in " << symbol.filePath << "\n";
        }
    }
    
    if (!context.relatedFiles.empty()) {
        std::cout << "\nRelated Files (" << context.relatedFiles.size() << "):\n";
        for (const auto& file : context.relatedFiles) {
            std::cout << "- " << file << "\n";
        }
    }
    
    if (!context.codeSnippets.empty()) {
        std::cout << "\nCode Snippets (" << context.codeSnippets.size() << "):\n";
        for (const auto& snippet : context.codeSnippets) {
            printSnippet(snippet);
        }
    }
    
    if (!context.knowledgeEntries.empty()) {
        std::cout << "\nKnowledge Base Entries (" << context.knowledgeEntries.size() << "):\n";
        for (const auto& entry : context.knowledgeEntries) {
            printKnowledgeEntry(entry);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <project_directory>\n";
        return 1;
    }
    
    // Initialize paths
    std::string projectDir = argv[1];
    std::string knowledgeBaseDir = std::filesystem::path(projectDir) / ".kb";
    
    // Ensure the project directory exists
    if (!std::filesystem::exists(projectDir)) {
        std::cerr << "Project directory does not exist: " << projectDir << "\n";
        return 1;
    }
    
    // Create knowledge base directory if it doesn't exist
    if (!std::filesystem::exists(knowledgeBaseDir)) {
        std::filesystem::create_directories(knowledgeBaseDir);
    }
    
    std::cout << "Initializing codebase index...\n";
    
    // Create codebase indexer
    auto indexer = std::make_shared<CodebaseIndexer>();
    bool indexResult = indexer->indexDirectory(projectDir);
    
    if (!indexResult) {
        std::cerr << "Failed to index directory: " << projectDir << "\n";
        return 1;
    }
    
    std::cout << "Indexing complete.\n";
    std::cout << "Files indexed: " << indexer->getIndexedFileCount() << "\n";
    std::cout << "Symbols found: " << indexer->getSymbolCount() << "\n";
    
    // Create project knowledge manager and knowledge base
    auto knowledgeManager = std::make_shared<ProjectKnowledgeManager>(knowledgeBaseDir);
    auto knowledgeBase = knowledgeManager->getKnowledgeBase(projectDir, true);
    
    if (!knowledgeBase) {
        std::cerr << "Failed to create or open knowledge base for: " << projectDir << "\n";
        return 1;
    }
    
    // Create context provider with knowledge base
    auto contextProvider = std::make_shared<CodeContextProvider>(indexer, knowledgeBase);
    
    // Add some sample knowledge entries
    KnowledgeEntry codeConventions;
    codeConventions.title = "Project Code Conventions";
    codeConventions.category = KnowledgeCategory::CodeConvention;
    codeConventions.content = "This project follows the Google C++ Style Guide with the following exceptions:\n"
                             "1. We use 4 spaces for indentation, not tabs\n"
                             "2. Line length limit is 100 characters\n"
                             "3. Class member variables use camelCase with trailing underscore (e.g., camelCase_)\n"
                             "4. We prefer composition over inheritance where possible";
    codeConventions.tags = {"style", "formatting", "guidelines"};
    
    KnowledgeEntry architectureOverview;
    architectureOverview.title = "AI-First Text Editor Architecture";
    architectureOverview.category = KnowledgeCategory::Architecture;
    architectureOverview.content = "The editor is built using a modular architecture with these key components:\n"
                                  "- Core Editor: Handles text editing, file operations, and UI\n"
                                  "- Codebase Indexer: Indexes and analyzes the project structure\n"
                                  "- Context Provider: Gathers relevant context for AI suggestions\n"
                                  "- AI Providers: Interface with different AI models\n"
                                  "- Knowledge Base: Stores project-specific knowledge";
    architectureOverview.tags = {"architecture", "design", "components"};
    
    KnowledgeEntry uiGuidelines;
    uiGuidelines.title = "UI Component Guidelines";
    uiGuidelines.category = KnowledgeCategory::UIDesign;
    uiGuidelines.content = "When creating new UI components:\n"
                          "1. Use the existing theme system for colors and styles\n"
                          "2. Ensure all components are accessible\n"
                          "3. Follow the reactive design pattern\n"
                          "4. Add appropriate keyboard shortcuts";
    uiGuidelines.tags = {"UI", "components", "accessibility"};
    
    // Add entries to knowledge base
    knowledgeBase->addEntry(codeConventions);
    knowledgeBase->addEntry(architectureOverview);
    knowledgeBase->addEntry(uiGuidelines);
    
    // Add a custom project feature entry
    KnowledgeEntry contextSystem;
    contextSystem.title = "Context Gathering System";
    contextSystem.category = KnowledgeCategory::Feature;
    contextSystem.customCategory = "AI Features";
    contextSystem.content = "The context gathering system uses relevance scoring to prioritize code snippets and symbols.\n"
                          "Key features:\n"
                          "- Scores snippets based on proximity to cursor\n"
                          "- Prioritizes symbols related to current code\n"
                          "- Manages token limits for AI models\n"
                          "- Integrates with the project knowledge base";
    contextSystem.tags = {"context", "ai", "relevance"};
    knowledgeBase->addEntry(contextSystem);
    
    std::cout << "Added sample knowledge entries.\n";
    std::cout << "Knowledge base now has " << knowledgeBase->getEntryCount() << " entries.\n";
    
    // Demo the knowledge base integration
    printDivider();
    std::cout << "KNOWLEDGE BASE INTEGRATION DEMO\n";
    printDivider();
    
    // Set up context options
    ContextOptions options;
    options.includeKnowledgeBase = true;
    options.maxKnowledgeEntries = 3;
    options.maxTokens = 8000;
    
    // Find a source file to use for the demo
    std::string demoFile;
    std::vector<std::string> cppFiles;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".cpp" || ext == ".hpp" || ext == ".h") {
                cppFiles.push_back(entry.path().string());
                
                // Prefer CodeContextProvider files for the demo
                if (entry.path().filename().string().find("CodeContextProvider") != std::string::npos) {
                    demoFile = entry.path().string();
                }
            }
        }
    }
    
    // If we didn't find a CodeContextProvider file, use the first C++ file
    if (demoFile.empty() && !cppFiles.empty()) {
        demoFile = cppFiles[0];
    }
    
    if (demoFile.empty()) {
        std::cout << "No C++ files found for demo. Using a dummy file path.\n";
        demoFile = "src/main.cpp";
    }
    
    std::cout << "Using file for demo: " << demoFile << "\n";
    
    // Get context with knowledge base integration
    CodeContext context = contextProvider->getContext(
        demoFile,        // Current file
        10,              // Line
        5,               // Column
        "context",       // Selected text - will help match knowledge entries
        {},              // No visible files
        options          // Context options with knowledge base enabled
    );
    
    // Print the context information
    std::cout << "Context with knowledge base integration:\n";
    printContext(context);
    
    // Generate a prompt with the context
    printDivider();
    std::cout << "GENERATED CONTEXTUAL PROMPT\n";
    printDivider();
    
    std::string prompt = contextProvider->generateContextualPrompt(
        "How does the context gathering system work?",
        context,
        options
    );
    
    std::cout << prompt << "\n";
    
    // Save the knowledge base
    std::string kbFilePath = std::filesystem::path(knowledgeBaseDir) / "knowledge_base.json";
    bool saveResult = knowledgeBase->saveToFile(kbFilePath);
    
    if (saveResult) {
        std::cout << "Knowledge base saved to: " << kbFilePath << "\n";
    } else {
        std::cerr << "Failed to save knowledge base to: " << kbFilePath << "\n";
    }
    
    return 0;
} 