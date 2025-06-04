#pragma once

#ifndef ICODEBASEINDEX_HPP
#define ICODEBASEINDEX_HPP

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace ai_editor {

/**
 * @struct CodeSymbol
 * @brief Represents a symbol in the codebase (function, class, variable, etc.)
 */
struct CodeSymbol {
    enum class SymbolType {
        UNKNOWN,
        FUNCTION,
        METHOD,
        CLASS,
        STRUCT,
        VARIABLE,
        FIELD,
        ENUM,
        INTERFACE,
        NAMESPACE,
        MODULE,
        PACKAGE,
        FILE
    };
    
    std::string id;                 // Unique identifier for the symbol
    std::string name;               // Name of the symbol
    std::string displayName;        // Display name (may include namespace, class, etc.)
    SymbolType type;                // Type of the symbol
    std::string filePath;           // File where the symbol is defined
    int lineNumber;                 // Line number where the symbol is defined
    int columnNumber;               // Column number where the symbol is defined
    std::string signature;          // Signature (for functions/methods)
    std::string documentation;      // Documentation or comments
    std::string namespace_;         // Namespace or package
    std::optional<std::string> parentId; // Parent symbol ID (e.g., class for a method)
    std::vector<std::string> childIds;   // Child symbol IDs
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
    
    // Equality comparison
    bool operator==(const CodeSymbol& other) const {
        return id == other.id;
    }
};

/**
 * @struct SymbolReference
 * @brief Represents a reference to a symbol in the code
 */
struct SymbolReference {
    std::string symbolId;        // ID of the referenced symbol
    std::string filePath;        // File containing the reference
    int lineNumber;              // Line number of the reference
    int columnNumber;            // Column number of the reference
    bool isDefinition;           // Whether this is the symbol's definition
    
    // Equality comparison
    bool operator==(const SymbolReference& other) const {
        return symbolId == other.symbolId &&
               filePath == other.filePath &&
               lineNumber == other.lineNumber &&
               columnNumber == other.columnNumber;
    }
};

/**
 * @struct SymbolRelation
 * @brief Represents a relationship between two symbols
 */
struct SymbolRelation {
    enum class RelationType {
        UNKNOWN,
        CALLS,            // Function calls another function
        INHERITS_FROM,    // Class inherits from another class
        CONTAINS,         // Class contains field
        IMPLEMENTS,       // Class implements interface
        USES,             // Uses a type
        OVERRIDES,        // Method overrides another method
        DEPENDS_ON        // General dependency
    };
    
    std::string sourceSymbolId;  // ID of the source symbol
    std::string targetSymbolId;  // ID of the target symbol
    RelationType type;           // Type of the relationship
    std::string description;     // Description of the relationship
    
    // Equality comparison
    bool operator==(const SymbolRelation& other) const {
        return sourceSymbolId == other.sourceSymbolId &&
               targetSymbolId == other.targetSymbolId &&
               type == other.type;
    }
};

/**
 * @struct FileInfo
 * @brief Information about a file in the codebase
 */
struct FileInfo {
    std::string path;              // Path to the file
    std::string language;          // Programming language
    std::string encoding;          // File encoding
    size_t sizeBytes;              // File size in bytes
    std::string hash;              // File hash for detecting changes
    std::unordered_set<std::string> symbols; // Symbols defined in this file
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
    
    // Equality comparison
    bool operator==(const FileInfo& other) const {
        return path == other.path;
    }
};

/**
 * @enum SearchResultType
 * @brief Type of search result
 */
enum class SearchResultType {
    SYMBOL,  // Result is a code symbol
    FILE     // Result is a file
};

/**
 * @struct SearchResult
 * @brief Represents a search result from the codebase
 */
struct SearchResult {
    SearchResultType type;      // Type of search result
    std::string symbolId;       // ID of the symbol (if type is SYMBOL)
    std::string filePath;       // Path to the file
    int lineNumber;             // Line number (if type is SYMBOL)
    int columnNumber;           // Column number (if type is SYMBOL)
    std::string name;           // Name of the symbol or file
    std::string kind;           // Kind of symbol or file type
    std::string snippet;        // Code snippet around the result
};

/**
 * @interface ICodebaseIndex
 * @brief Interface for accessing and querying the codebase index
 * 
 * This interface provides methods to query and navigate the indexed code
 * structure, including symbols, references, and relationships.
 */
class ICodebaseIndex {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ICodebaseIndex() = default;
    
    /**
     * @brief Get the root directories being indexed
     * 
     * @return std::vector<std::string> The root directories
     */
    virtual std::vector<std::string> getRootDirectories() const = 0;
    
    /**
     * @brief Add a root directory to the index
     * 
     * @param directory The directory to add
     * @return bool True if successful
     */
    virtual bool addRootDirectory(const std::string& directory) = 0;
    
    /**
     * @brief Remove a root directory from the index
     * 
     * @param directory The directory to remove
     * @return bool True if successful
     */
    virtual bool removeRootDirectory(const std::string& directory) = 0;
    
    /**
     * @brief Get a symbol by ID
     * 
     * @param symbolId The symbol ID
     * @return std::optional<CodeSymbol> The symbol, if found
     */
    virtual std::optional<CodeSymbol> getSymbol(const std::string& symbolId) const = 0;
    
    /**
     * @brief Find symbols by name
     * 
     * @param name The symbol name to search for
     * @param exactMatch Whether to require an exact match
     * @return std::vector<CodeSymbol> Matching symbols
     */
    virtual std::vector<CodeSymbol> findSymbolsByName(
        const std::string& name, 
        bool exactMatch = true) const = 0;
    
    /**
     * @brief Find symbols by type
     * 
     * @param type The symbol type to search for
     * @param includeChildren Whether to include child symbol types
     * @return std::vector<CodeSymbol> Matching symbols
     */
    virtual std::vector<CodeSymbol> findSymbolsByType(
        CodeSymbol::SymbolType type,
        bool includeChildren = false) const = 0;
    
    /**
     * @brief Find symbols in a file
     * 
     * @param filePath The file path
     * @return std::vector<CodeSymbol> Symbols in the file
     */
    virtual std::vector<CodeSymbol> findSymbolsInFile(const std::string& filePath) const = 0;
    
    /**
     * @brief Get symbol references
     * 
     * @param symbolId The symbol ID
     * @return std::vector<SymbolReference> References to the symbol
     */
    virtual std::vector<SymbolReference> getSymbolReferences(const std::string& symbolId) const = 0;
    
    /**
     * @brief Get symbol relations
     * 
     * @param symbolId The symbol ID
     * @param relationType Optional relation type filter
     * @param inbound Whether to get relations where this symbol is the target
     * @return std::vector<SymbolRelation> Relations involving the symbol
     */
    virtual std::vector<SymbolRelation> getSymbolRelations(
        const std::string& symbolId,
        std::optional<SymbolRelation::RelationType> relationType = std::nullopt,
        bool inbound = false) const = 0;
    
    /**
     * @brief Get all files in the index
     * 
     * @return std::vector<FileInfo> Information about all indexed files
     */
    virtual std::vector<FileInfo> getAllFiles() const = 0;
    
    /**
     * @brief Get file information
     * 
     * @param filePath The file path
     * @return std::optional<FileInfo> File information, if found
     */
    virtual std::optional<FileInfo> getFileInfo(const std::string& filePath) const = 0;
    
    /**
     * @brief Find files by language
     * 
     * @param language The programming language
     * @return std::vector<FileInfo> Files in the specified language
     */
    virtual std::vector<FileInfo> findFilesByLanguage(const std::string& language) const = 0;
    
    /**
     * @brief Search the codebase with a query
     * 
     * @param query The search query
     * @param maxResults Maximum number of results to return
     * @return std::vector<SearchResult> Matching results
     */
    virtual std::vector<SearchResult> search(
        const std::string& query, 
        size_t maxResults = 100) const = 0;
    
    /**
     * @brief Check if the index is currently being built or updated
     * 
     * @return bool True if the index is being built or updated
     */
    virtual bool isIndexing() const = 0;
    
    /**
     * @brief Get the current indexing progress
     * 
     * @return float Progress as a value between 0.0 and 1.0
     */
    virtual float getIndexingProgress() const = 0;
    
    /**
     * @brief Force a reindex of the codebase
     * 
     * @param incremental Whether to do an incremental update
     * @return bool True if reindexing started successfully
     */
    virtual bool reindex(bool incremental = true) = 0;
    
    /**
     * @brief Register a callback for index update notifications
     * 
     * @param callback Function to call when the index is updated
     * @return int Callback ID for later unregistration
     */
    virtual int registerUpdateCallback(std::function<void()> callback) = 0;
    
    /**
     * @brief Unregister an update callback
     * 
     * @param callbackId Callback ID from registerUpdateCallback
     */
    virtual void unregisterUpdateCallback(int callbackId) = 0;
};

} // namespace ai_editor

#endif // ICODEBASEINDEX_HPP 