#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>

#include "../CodeContextProvider.hpp"
#include "../interfaces/ICodebaseIndex.hpp"
#include "../EditorErrorReporter.h"

// Mock codebase index for testing
class MockCodebaseIndex : public ICodebaseIndex {
public:
    void addSymbol(Symbol symbol) {
        m_symbols.push_back(symbol);
    }
    
    void addRelationship(const SymbolRelationship& relationship) {
        m_relationships.push_back(relationship);
    }
    
    void addFile(const std::string& filePath) {
        m_files.push_back(filePath);
    }
    
    // ICodebaseIndex implementation
    virtual void indexFile(const std::string& filePath) override {
        // Mock implementation
        m_files.push_back(filePath);
    }
    
    virtual void indexDirectory(const std::string& directoryPath) override {
        // Mock implementation
        std::cout << "Indexing directory: " << directoryPath << std::endl;
    }
    
    virtual Symbol findSymbolAtLocation(const std::string& filePath, int line, int column) override {
        // Find a symbol near the specified location
        for (const auto& symbol : m_symbols) {
            if (symbol.filePath == filePath && 
                line >= symbol.lineStart && line <= symbol.lineEnd) {
                return symbol;
            }
        }
        return Symbol();
    }
    
    virtual std::vector<Symbol> findSymbolsByName(const std::string& name) override {
        std::vector<Symbol> result;
        for (const auto& symbol : m_symbols) {
            if (symbol.name == name) {
                result.push_back(symbol);
            }
        }
        return result;
    }
    
    virtual std::vector<Symbol> findSymbolsInFile(const std::string& filePath) override {
        std::vector<Symbol> result;
        for (const auto& symbol : m_symbols) {
            if (symbol.filePath == filePath) {
                result.push_back(symbol);
            }
        }
        return result;
    }
    
    virtual std::vector<SymbolRelationship> findRelationships(const std::string& symbolId) override {
        std::vector<SymbolRelationship> result;
        for (const auto& rel : m_relationships) {
            if (rel.sourceSymbolId == symbolId || rel.targetSymbolId == symbolId) {
                result.push_back(rel);
            }
        }
        return result;
    }
    
    virtual std::vector<std::string> getIndexedFiles() override {
        return m_files;
    }
    
    virtual std::vector<Symbol> getAllSymbols() override {
        return m_symbols;
    }
    
    virtual bool isIndexed(const std::string& filePath) override {
        return std::find(m_files.begin(), m_files.end(), filePath) != m_files.end();
    }
    
    virtual void clear() override {
        m_symbols.clear();
        m_relationships.clear();
        m_files.clear();
    }
    
private:
    std::vector<Symbol> m_symbols;
    std::vector<SymbolRelationship> m_relationships;
    std::vector<std::string> m_files;
};

// Helper function to create test symbols
Symbol createTestSymbol(const std::string& name, const std::string& filePath, 
                      int lineStart, int lineEnd, SymbolType type) {
    Symbol symbol;
    symbol.id = "sym_" + name;
    symbol.name = name;
    symbol.filePath = filePath;
    symbol.lineStart = lineStart;
    symbol.lineEnd = lineEnd;
    symbol.columnStart = 0;
    symbol.columnEnd = 0;
    symbol.type = type;
    symbol.signature = name + "()";
    symbol.documentation = "Documentation for " + name;
    return symbol;
}

// Test cases for the enhanced context gathering
void testRelevanceScoring() {
    std::cout << "=== Testing Relevance Scoring ===" << std::endl;
    
    auto mockIndex = std::make_shared<MockCodebaseIndex>();
    auto contextProvider = std::make_shared<CodeContextProvider>(mockIndex);
    
    // Setup test data
    Symbol mainFunc = createTestSymbol("main", "/project/main.cpp", 10, 20, SymbolType::Function);
    Symbol helperFunc = createTestSymbol("helper", "/project/utils.cpp", 5, 15, SymbolType::Function);
    Symbol unrelatedFunc = createTestSymbol("unrelated", "/project/other.cpp", 30, 40, SymbolType::Function);
    
    mockIndex->addSymbol(mainFunc);
    mockIndex->addSymbol(helperFunc);
    mockIndex->addSymbol(unrelatedFunc);
    
    // Add relationship from main to helper
    SymbolRelationship rel;
    rel.sourceSymbolId = mainFunc.id;
    rel.targetSymbolId = helperFunc.id;
    rel.type = RelationshipType::Calls;
    mockIndex->addRelationship(rel);
    
    // Add files
    mockIndex->addFile("/project/main.cpp");
    mockIndex->addFile("/project/utils.cpp");
    mockIndex->addFile("/project/other.cpp");
    
    // Register custom symbol scorer
    contextProvider->registerSymbolRelevanceScorer([](const Symbol& symbol, const Symbol& currentSymbol) {
        if (symbol.name == "helper") {
            return 0.9f;  // Boost helper function
        }
        return 0.5f;
    });
    
    // Register custom file scorer
    contextProvider->registerFileRelevanceScorer([](const std::string& filePath, const std::string& currentFilePath) {
        if (filePath == "/project/utils.cpp") {
            return 0.8f;  // Boost utils.cpp
        }
        return 0.4f;
    });
    
    // Get context
    CodeContextProvider::ContextOptions options;
    options.maxRelatedSymbols = 10;
    options.maxRelatedFiles = 5;
    options.minRelevanceScore = 0.1f;  // Low threshold to include everything
    
    CodeContext context = contextProvider->getContext("/project/main.cpp", 15, 0, "", {"/project/main.cpp"}, options);
    
    // Verify that the helper function has higher relevance
    bool helperFound = false;
    bool unrelatedFound = false;
    
    for (const auto& symbol : context.relatedSymbols) {
        if (symbol.name == "helper") {
            helperFound = true;
            std::cout << "Helper function relevance: " << symbol.relevanceScore << std::endl;
            assert(symbol.relevanceScore > 0.7f);  // Should be high due to custom scorer and relationship
        }
        if (symbol.name == "unrelated") {
            unrelatedFound = true;
            std::cout << "Unrelated function relevance: " << symbol.relevanceScore << std::endl;
            assert(symbol.relevanceScore < 0.6f);  // Should be lower
        }
    }
    
    assert(helperFound);  // Helper should be included
    
    // Check file relevance
    bool utilsFound = false;
    bool otherFound = false;
    
    for (const auto& file : context.relatedFiles) {
        if (file.path == "/project/utils.cpp") {
            utilsFound = true;
            std::cout << "Utils file relevance: " << file.relevanceScore << std::endl;
            assert(file.relevanceScore > 0.7f);  // Should be high due to custom scorer
        }
        if (file.path == "/project/other.cpp") {
            otherFound = true;
            std::cout << "Other file relevance: " << file.relevanceScore << std::endl;
            assert(file.relevanceScore < 0.6f);  // Should be lower
        }
    }
    
    assert(utilsFound);  // Utils should be included
    
    std::cout << "Relevance scoring test passed!" << std::endl;
}

void testTokenManagement() {
    std::cout << "=== Testing Token Management ===" << std::endl;
    
    auto mockIndex = std::make_shared<MockCodebaseIndex>();
    auto contextProvider = std::make_shared<CodeContextProvider>(mockIndex);
    
    // Setup test data with many symbols to trigger token limit
    for (int i = 0; i < 50; i++) {
        Symbol sym = createTestSymbol("func" + std::to_string(i), 
                                     "/project/file" + std::to_string(i % 10) + ".cpp", 
                                     i * 10, i * 10 + 5, 
                                     SymbolType::Function);
        // Add a long documentation to increase token count
        sym.documentation = std::string(100, 'x') + " Documentation for function " + std::to_string(i);
        mockIndex->addSymbol(sym);
        mockIndex->addFile(sym.filePath);
    }
    
    // Get context with a very low token limit
    CodeContextProvider::ContextOptions options;
    options.maxRelatedSymbols = 50;  // Try to include all symbols
    options.maxRelatedFiles = 20;    // Try to include all files
    options.maxTokens = 500;         // But limit tokens severely
    options.minRelevanceScore = 0.0f; // Include everything
    
    CodeContext context = contextProvider->getContext("/project/file0.cpp", 15, 0, "", {"/project/file0.cpp"}, options);
    
    // Check if the context was trimmed to respect token limit
    int totalSymbols = context.relatedSymbols.size();
    int totalFiles = context.relatedFiles.size();
    int totalSnippets = context.codeSnippets.size();
    
    std::cout << "Total symbols after trimming: " << totalSymbols << std::endl;
    std::cout << "Total files after trimming: " << totalFiles << std::endl;
    std::cout << "Total snippets after trimming: " << totalSnippets << std::endl;
    
    // Estimate the token count (rough approximation)
    int estimatedTokens = 0;
    
    // Symbols
    for (const auto& symbol : context.relatedSymbols) {
        estimatedTokens += contextProvider->estimateTokenCount(symbol.name + symbol.signature + symbol.documentation);
    }
    
    // Files
    for (const auto& file : context.relatedFiles) {
        estimatedTokens += contextProvider->estimateTokenCount(file.path);
    }
    
    // Snippets
    for (const auto& snippet : context.codeSnippets) {
        estimatedTokens += contextProvider->estimateTokenCount(snippet.content);
    }
    
    std::cout << "Estimated token count: " << estimatedTokens << std::endl;
    
    // The estimated token count should be reasonably close to the limit
    assert(estimatedTokens <= options.maxTokens * 1.2);  // Allow some margin due to estimation inaccuracy
    
    // There should be fewer symbols than we added (due to trimming)
    assert(totalSymbols < 50);
    
    std::cout << "Token management test passed!" << std::endl;
}

void testContextPruning() {
    std::cout << "=== Testing Context Pruning ===" << std::endl;
    
    auto mockIndex = std::make_shared<MockCodebaseIndex>();
    auto contextProvider = std::make_shared<CodeContextProvider>(mockIndex);
    
    // Setup test data with varying relevance
    for (int i = 0; i < 20; i++) {
        Symbol sym = createTestSymbol("func" + std::to_string(i), 
                                     "/project/file" + std::to_string(i % 5) + ".cpp", 
                                     i * 10, i * 10 + 5, 
                                     SymbolType::Function);
        mockIndex->addSymbol(sym);
        mockIndex->addFile(sym.filePath);
    }
    
    // Register a custom scorer that gives higher relevance to even-numbered functions
    contextProvider->registerSymbolRelevanceScorer([](const Symbol& symbol, const Symbol& currentSymbol) {
        if (symbol.name.find("func") == 0) {
            int num = std::stoi(symbol.name.substr(4));
            if (num % 2 == 0) {
                return 0.9f;  // High relevance for even numbers
            } else {
                return 0.2f;  // Low relevance for odd numbers
            }
        }
        return 0.5f;
    });
    
    // Get context with a high minimum relevance score
    CodeContextProvider::ContextOptions options;
    options.maxRelatedSymbols = 20;         // Try to include all symbols
    options.minRelevanceScore = 0.5f;       // But require high relevance
    
    CodeContext context = contextProvider->getContext("/project/file0.cpp", 15, 0, "", {"/project/file0.cpp"}, options);
    
    // Check that only high-relevance symbols were included
    int evenCount = 0;
    int oddCount = 0;
    
    for (const auto& symbol : context.relatedSymbols) {
        if (symbol.name.find("func") == 0) {
            int num = std::stoi(symbol.name.substr(4));
            if (num % 2 == 0) {
                evenCount++;
            } else {
                oddCount++;
            }
        }
    }
    
    std::cout << "Even-numbered functions (high relevance): " << evenCount << std::endl;
    std::cout << "Odd-numbered functions (low relevance): " << oddCount << std::endl;
    
    // We should have more even-numbered (high relevance) functions than odd-numbered ones
    assert(evenCount > 0);
    assert(oddCount == 0);  // All odd-numbered functions should be pruned due to low relevance
    
    std::cout << "Context pruning test passed!" << std::endl;
}

void testContextualPrompt() {
    std::cout << "=== Testing Contextual Prompt Generation ===" << std::endl;
    
    auto mockIndex = std::make_shared<MockCodebaseIndex>();
    auto contextProvider = std::make_shared<CodeContextProvider>(mockIndex);
    
    // Setup test data
    Symbol mainFunc = createTestSymbol("main", "/project/main.cpp", 10, 20, SymbolType::Function);
    Symbol helperFunc = createTestSymbol("helper", "/project/utils.cpp", 5, 15, SymbolType::Function);
    
    mockIndex->addSymbol(mainFunc);
    mockIndex->addSymbol(helperFunc);
    
    // Add relationship
    SymbolRelationship rel;
    rel.sourceSymbolId = mainFunc.id;
    rel.targetSymbolId = helperFunc.id;
    rel.type = RelationshipType::Calls;
    mockIndex->addRelationship(rel);
    
    // Add files
    mockIndex->addFile("/project/main.cpp");
    mockIndex->addFile("/project/utils.cpp");
    
    // Get context
    CodeContextProvider::ContextOptions options;
    CodeContext context = contextProvider->getContext("/project/main.cpp", 15, 0, "", {"/project/main.cpp"}, options);
    
    // Generate a contextual prompt
    std::string userInput = "How do I improve this code?";
    std::string prompt = contextProvider->generateContextualPrompt(userInput, context, options);
    
    // Verify the prompt contains important elements
    assert(prompt.find(userInput) != std::string::npos);
    assert(prompt.find("main") != std::string::npos);
    assert(prompt.find("helper") != std::string::npos);
    assert(prompt.find("/project/main.cpp") != std::string::npos);
    
    std::cout << "Contextual prompt test passed!" << std::endl;
    std::cout << "Sample prompt:\n---\n" << prompt << "\n---" << std::endl;
}

int main() {
    try {
        std::cout << "Running enhanced context gathering tests..." << std::endl;
        
        testRelevanceScoring();
        testTokenManagement();
        testContextPruning();
        testContextualPrompt();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
} 