#pragma once

#include "interfaces/ICodebaseIndex.hpp"
#include "interfaces/IProjectKnowledgeBase.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>

namespace ai_editor {

/**
 * @struct CodeContext
 * @brief Contains relevant code context information for AI assistance
 */
struct CodeContext {
    std::string currentFile;              // Current file being edited
    size_t cursorLine;                    // Current cursor line position
    size_t cursorColumn;                  // Current cursor column position
    std::string selectedText;             // Currently selected text, if any
    std::vector<std::string> visibleFiles; // Currently visible/open files
    
    // Current function/class/symbol information
    std::optional<CodeSymbol> currentSymbol;
    
    // Related symbols (e.g., parent class, methods of current class)
    std::vector<CodeSymbol> relatedSymbols;
    
    // Related files (e.g., header for implementation file)
    std::vector<std::string> relatedFiles;
    
    // Code snippets from relevant parts of the codebase
    struct ContextSnippet {
        std::string filePath;
        std::string symbolName;
        std::string content;
        size_t startLine;
        size_t endLine;
        float relevanceScore;  // Higher score = more relevant
    };
    std::vector<ContextSnippet> codeSnippets;

    // Project structure information
    std::vector<std::string> importantProjectFiles;  // Key files in the project
    std::string projectLanguage;                    // Primary language used
    std::vector<std::string> dependencies;          // Project dependencies
    
    // Project knowledge base entries
    std::vector<KnowledgeEntry> knowledgeEntries;   // Relevant knowledge base entries
};

/**
 * @struct ContextOptions
 * @brief Options for context gathering customization
 */
struct ContextOptions {
    // General options
    bool includeDefinitions = true;       // Include symbol definitions
    bool includeReferences = true;        // Include symbol references
    bool includeRelationships = true;     // Include symbol relationships
    bool includeKnowledgeBase = true;     // Include knowledge base entries
    
    // Token management
    size_t maxTokens = 4000;              // Maximum tokens for all context
    
    // Relevance thresholds
    float minRelevanceScore = 0.2f;       // Minimum relevance score (0-1)
    
    // Quantity limits
    size_t maxSymbols = 10;               // Maximum related symbols to include
    size_t maxSnippets = 5;               // Maximum code snippets to include
    size_t maxRelatedFiles = 5;           // Maximum related files to include
    size_t maxKnowledgeEntries = 3;       // Maximum knowledge base entries to include
    
    // Scope depth
    size_t symbolScopeDepth = 2;          // How deep to traverse symbol hierarchy
    
    // Knowledge base options
    std::optional<KnowledgeCategory> knowledgeCategory = std::nullopt; // Optional category filter
};

// Function type for scoring symbol relevance
using SymbolRelevanceScorer = std::function<float(
    const CodeSymbol&, 
    const std::string&, 
    size_t, 
    size_t
)>;

// Function type for scoring file relevance
using FileRelevanceScorer = std::function<float(
    const std::string&,
    const std::string&
)>;

/**
 * @class CodeContextProvider
 * @brief Provides code context information for AI assistance
 * 
 * This class serves as a bridge between the codebase indexing system and the AI
 * system, gathering relevant context information about the code the user is
 * working with to improve the relevance of AI suggestions.
 */
class CodeContextProvider {
public:
    /**
     * @brief Constructor
     * 
     * @param codebaseIndex The codebase index to use for context gathering
     * @param knowledgeBase Optional knowledge base for project-specific information
     */
    explicit CodeContextProvider(
        std::shared_ptr<ICodebaseIndex> codebaseIndex,
        std::shared_ptr<IProjectKnowledgeBase> knowledgeBase = nullptr);
    
    /**
     * @brief Set the project knowledge base
     * 
     * @param knowledgeBase The project knowledge base to use
     */
    void setProjectKnowledgeBase(std::shared_ptr<IProjectKnowledgeBase> knowledgeBase);
    
    /**
     * @brief Get the project knowledge base
     * 
     * @return std::shared_ptr<IProjectKnowledgeBase> The project knowledge base
     */
    std::shared_ptr<IProjectKnowledgeBase> getProjectKnowledgeBase() const;
    
    /**
     * @brief Get code context for the current editing position
     * 
     * @param filePath Current file path
     * @param line Current line position
     * @param column Current column position
     * @param selectedText Currently selected text, if any
     * @param visibleFiles Currently visible/open files
     * @param options Context gathering options
     * @return CodeContext Context information for AI assistance
     */
    CodeContext getContext(
        const std::string& filePath,
        size_t line,
        size_t column,
        const std::string& selectedText = "",
        const std::vector<std::string>& visibleFiles = {},
        const ContextOptions& options = ContextOptions());
    
    /**
     * @brief Generate a prompt with code context information
     * 
     * @param userPrompt The user's original prompt
     * @param context The code context information
     * @param options The context options
     * @return std::string A prompt enriched with code context information
     */
    std::string generateContextualPrompt(
        const std::string& userPrompt,
        const CodeContext& context,
        const ContextOptions& options = ContextOptions());
    
    /**
     * @brief Get an AI-friendly summary of a symbol
     * 
     * @param symbol The symbol to summarize
     * @return std::string A summary of the symbol suitable for inclusion in AI prompts
     */
    std::string getSymbolSummary(const CodeSymbol& symbol);
    
    /**
     * @brief Set context gathering options
     * 
     * @param options The options to set
     */
    void setContextOptions(const ContextOptions& options) { contextOptions_ = options; }
    
    /**
     * @brief Get the current context options
     * 
     * @return const ContextOptions& The current options
     */
    const ContextOptions& getContextOptions() const { return contextOptions_; }
    
    /**
     * @brief Register a custom symbol relevance scorer
     * 
     * @param name Name of the scorer
     * @param scorer Function that scores a symbol's relevance (0-1)
     */
    void registerSymbolRelevanceScorer(
        const std::string& name,
        SymbolRelevanceScorer scorer);
    
    /**
     * @brief Register a custom file relevance scorer
     * 
     * @param name Name of the scorer
     * @param scorer Function that scores a file's relevance (0-1)
     */
    void registerFileRelevanceScorer(
        const std::string& name, 
        FileRelevanceScorer scorer);
    
    /**
     * @brief Estimate token count for a string
     * 
     * @param text The text to estimate tokens for
     * @return size_t Estimated number of tokens
     */
    static size_t estimateTokenCount(const std::string& text);
    
private:
    /**
     * @brief Find the symbol at the given position
     * 
     * @param filePath File path
     * @param line Line position
     * @param column Column position
     * @return std::optional<CodeSymbol> The symbol at the position, if any
     */
    std::optional<CodeSymbol> findSymbolAtPosition(
        const std::string& filePath,
        size_t line,
        size_t column);
    
    /**
     * @brief Find related symbols for the given symbol
     * 
     * @param symbol The symbol to find related symbols for
     * @param maxRelated Maximum number of related symbols to include
     * @param options Context options affecting symbol gathering
     * @return std::vector<CodeSymbol> Related symbols
     */
    std::vector<CodeSymbol> findRelatedSymbols(
        const CodeSymbol& symbol,
        size_t maxRelated = 10,
        const ContextOptions& options = ContextOptions());
    
    /**
     * @brief Find related files for the given file
     * 
     * @param filePath The file to find related files for
     * @param options Context options affecting file gathering
     * @return std::vector<std::string> Related files
     */
    std::vector<std::string> findRelatedFiles(
        const std::string& filePath,
        const ContextOptions& options = ContextOptions());
    
    /**
     * @brief Generate code snippets for the given context
     * 
     * @param context The context information
     * @param maxSnippets Maximum number of snippets to generate
     * @param options Context options affecting snippet generation
     * @return std::vector<CodeContext::ContextSnippet> Code snippets
     */
    std::vector<CodeContext::ContextSnippet> generateCodeSnippets(
        const CodeContext& context,
        size_t maxSnippets,
        const ContextOptions& options = ContextOptions());
    
    /**
     * @brief Calculate relevance score for a symbol
     * 
     * @param symbol The symbol to score
     * @param currentFile Current file path
     * @param line Current line position
     * @param column Current column position
     * @return float Relevance score (0-1)
     */
    float calculateSymbolRelevance(
        const CodeSymbol& symbol,
        const std::string& currentFile,
        size_t line,
        size_t column);
    
    /**
     * @brief Calculate relevance score for a file
     * 
     * @param filePath The file to score
     * @param currentFile Current file path
     * @return float Relevance score (0-1)
     */
    float calculateFileRelevance(
        const std::string& filePath,
        const std::string& currentFile);
    
    /**
     * @brief Get important project files
     * 
     * @return std::vector<std::string> List of important project files
     */
    std::vector<std::string> getImportantProjectFiles();
    
    /**
     * @brief Detect primary project language
     * 
     * @return std::string The primary language used in the project
     */
    std::string detectProjectLanguage();
    
    /**
     * @brief Get project dependencies
     * 
     * @return std::vector<std::string> List of project dependencies
     */
    std::vector<std::string> getProjectDependencies();
    
    /**
     * @brief Trim context to fit within token limit
     * 
     * @param context The context to trim
     * @param maxTokens Maximum tokens allowed
     */
    void trimContextToTokenLimit(CodeContext& context, size_t maxTokens);
    
    /**
     * @brief Prune snippets based on relevance score
     * 
     * @param snippets Vector of snippets to prune
     * @param minScore Minimum relevance score to keep
     */
    void pruneSnippetsByRelevance(
        std::vector<CodeContext::ContextSnippet>& snippets,
        float minScore);
    
    /**
     * @brief Extract key terms from the current context
     * 
     * @param context The context to extract terms from
     * @return std::vector<std::string> List of key terms
     */
    std::vector<std::string> extractKeyTerms(const CodeContext& context);
    
    /**
     * @brief Find relevant knowledge base entries for the current context
     * 
     * @param context The current context
     * @param options The context options
     * @return std::vector<KnowledgeEntry> Relevant knowledge base entries
     */
    std::vector<KnowledgeEntry> findRelevantKnowledgeEntries(
        const CodeContext& context,
        const ContextOptions& options);
    
    // Helper methods
    std::string getSymbolTypeString(CodeSymbol::SymbolType type);
    int getSymbolDepth(const CodeSymbol& symbol);
    bool isSymbolMultiLine(const CodeSymbol& symbol);
    size_t getSymbolEndLine(const CodeSymbol& symbol);
    std::string getFileSnippet(const std::string& filePath, size_t startLine, size_t endLine);
    
    // Member variables
    std::shared_ptr<ICodebaseIndex> codebaseIndex_;
    std::shared_ptr<IProjectKnowledgeBase> knowledgeBase_;
    ContextOptions contextOptions_;
    
    // Custom relevance scorers
    std::unordered_map<std::string, SymbolRelevanceScorer> symbolScorers_;
    std::unordered_map<std::string, FileRelevanceScorer> fileScorers_;
};

} // namespace ai_editor 