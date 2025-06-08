#pragma once

#include "interfaces/ILanguageParser.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ai_editor {

/**
 * @brief Base class for language-specific parsers
 * 
 * This class provides common functionality for all language parsers,
 * implementing parts of the ILanguageParser interface with behavior
 * that's likely to be shared across multiple parser implementations.
 */
class BaseLanguageParser : public ILanguageParser {
public:
    /**
     * @brief Constructor for the BaseLanguageParser
     * @param languageId The ID of the language this parser handles
     */
    explicit BaseLanguageParser(const std::string& languageId);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~BaseLanguageParser() = default;
    
    /**
     * @brief Get the language ID handled by this parser
     * @return The language ID string
     */
    std::string getLanguageId() const override;
    
    /**
     * @brief Check if this parser can handle the specified file
     * @param filePath Path to the file
     * @param languageId Optional language ID if already known
     * @return True if this parser can handle the file, false otherwise
     */
    bool canHandleFile(const std::string& filePath, 
                      const std::optional<std::string>& languageId = std::nullopt) const override;
    
    /**
     * @brief Parse a file to extract symbols and relationships
     * @param filePath Path to the file to parse
     * @param fileContent Content of the file
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult containing found symbols, references, and relations
     */
    ParseResult parseFile(
        const std::string& filePath,
        const std::string& fileContent,
        const std::vector<CodeSymbol>& existingSymbols = {}) override;
    
    /**
     * @brief Parse a string of code
     * @param code The code to parse
     * @param contextPath Optional file path for context
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult containing found symbols, references, and relations
     */
    ParseResult parseCode(
        const std::string& code,
        const std::optional<std::string>& contextPath = std::nullopt,
        const std::vector<CodeSymbol>& existingSymbols = {}) override;
    
    /**
     * @brief Get the maximum number of bytes needed for context when parsing
     * @return The maximum context size in bytes, or 0 if unlimited
     */
    size_t getMaxParseContextSize() const override;
    
    /**
     * @brief Check if incremental parsing is supported
     * @return True if incremental parsing is supported, false otherwise
     */
    bool supportsIncrementalParsing() const override;
    
    /**
     * @brief Parse a file incrementally based on previous results and changed lines
     * @param filePath Path to the file to parse
     * @param fileContent Content of the file
     * @param previousResult Previous parse result for incremental update
     * @param startLine Start line of the change
     * @param endLine End line of the change
     * @param existingSymbols Existing symbols that may be referenced
     * @return Updated ParseResult
     */
    ParseResult parseFileIncrementally(
        const std::string& filePath,
        const std::string& fileContent,
        const ParseResult& previousResult,
        int startLine,
        int endLine,
        const std::vector<CodeSymbol>& existingSymbols = {}) override;

protected:
    /**
     * @brief The actual implementation of code parsing
     * 
     * This method must be implemented by derived classes to provide
     * language-specific parsing logic.
     * 
     * @param code The code to parse
     * @param filePath Optional file path for context
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult containing found symbols, references, and relations
     */
    virtual ParseResult parseCodeImpl(
        const std::string& code,
        const std::optional<std::string>& filePath = std::nullopt,
        const std::vector<CodeSymbol>& existingSymbols = {}) = 0;
    
    /**
     * @brief Read file content from disk
     * @param filePath Path to the file to read
     * @return The file content as a string, or empty string if file couldn't be read
     */
    std::string readFileContent(const std::string& filePath) const;
    
    /**
     * @brief Add a symbol to the parse result
     * 
     * This helper method creates a symbol and adds it to the parse result.
     * 
     * @param result The parse result to add the symbol to
     * @param name Symbol name
     * @param kind Symbol kind
     * @param filePath File where the symbol is defined
     * @param startLine Starting line of the symbol definition
     * @param startColumn Starting column of the symbol definition
     * @param endLine Ending line of the symbol definition
     * @param endColumn Ending column of the symbol definition
     * @param containerSymbolId ID of the containing symbol (if any)
     * @param properties Additional properties for the symbol
     * @return The ID of the created symbol
     */
    std::string addSymbol(
        ParseResult& result,
        const std::string& name,
        const std::string& kind,
        const std::string& filePath,
        size_t startLine,
        size_t startColumn,
        size_t endLine,
        size_t endColumn,
        const std::optional<std::string>& containerSymbolId = std::nullopt,
        const std::unordered_map<std::string, std::string>& properties = {});
    
    /**
     * @brief Add a reference to the parse result
     * 
     * This helper method creates a reference and adds it to the parse result.
     * 
     * @param result The parse result to add the reference to
     * @param symbolId ID of the referenced symbol
     * @param filePath File where the reference occurs
     * @param startLine Starting line of the reference
     * @param startColumn Starting column of the reference
     * @param endLine Ending line of the reference
     * @param endColumn Ending column of the reference
     * @param isDefinition Whether this reference is also a definition
     * @param containerSymbolId ID of the containing symbol (if any)
     * @return The ID of the created reference
     */
    std::string addReference(
        ParseResult& result,
        const std::string& symbolId,
        const std::string& filePath,
        size_t startLine,
        size_t startColumn,
        size_t endLine,
        size_t endColumn,
        bool isDefinition = false,
        const std::optional<std::string>& containerSymbolId = std::nullopt);
    
    /**
     * @brief Add a relation to the parse result
     * 
     * This helper method creates a relation between two symbols and adds it to the parse result.
     * 
     * @param result The parse result to add the relation to
     * @param sourceSymbolId ID of the source symbol
     * @param targetSymbolId ID of the target symbol
     * @param relationType Type of relation
     * @param properties Additional properties for the relation
     */
    void addRelation(
        ParseResult& result,
        const std::string& sourceSymbolId,
        const std::string& targetSymbolId,
        const std::string& relationType,
        const std::unordered_map<std::string, std::string>& properties = {});
    
    /** The language ID this parser handles */
    std::string languageId_;
    
    /** The maximum parse context size in bytes (0 = unlimited) */
    size_t maxParseContextSize_ = 0;
    
    /** Whether incremental parsing is supported */
    bool supportsIncrementalParsing_ = false;
};

} // namespace ai_editor 