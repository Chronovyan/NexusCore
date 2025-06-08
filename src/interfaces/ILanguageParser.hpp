#pragma once

#include "ICodebaseIndex.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ai_editor {

/**
 * @struct ParseResult
 * @brief The result of parsing a file
 */
struct ParseResult {
    std::vector<CodeSymbol> symbols;              // Symbols found in the file
    std::vector<SymbolReference> references;      // Symbol references found in the file
    std::vector<SymbolRelation> relations;        // Relations between symbols
    std::unordered_map<std::string, std::string> metadata; // Additional parser-specific metadata
    bool success;                                 // Whether parsing was successful
    std::string errorMessage;                     // Error message if parsing failed
};

/**
 * @interface ILanguageParser
 * @brief Interface for language-specific code parsers
 * 
 * This interface defines the contract for parsers that can extract
 * symbols, references, and relationships from source files in a
 * specific programming language.
 */
class ILanguageParser {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ILanguageParser() = default;
    
    /**
     * @brief Get the language ID this parser handles
     * 
     * @return std::string The language ID
     */
    virtual std::string getLanguageId() const = 0;
    
    /**
     * @brief Check if this parser can handle a specific file
     * 
     * @param filePath The path to the file
     * @param languageId Optional language ID hint
     * @return bool True if the parser can handle this file
     */
    virtual bool canHandleFile(
        const std::string& filePath,
        const std::optional<std::string>& languageId = std::nullopt) const = 0;
    
    /**
     * @brief Parse a file to extract symbols and relationships
     * 
     * @param filePath The path to the file
     * @param fileContent The content of the file
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult The result of parsing
     */
    virtual ParseResult parseFile(
        const std::string& filePath,
        const std::string& fileContent,
        const std::vector<CodeSymbol>& existingSymbols = {}) = 0;
    
    /**
     * @brief Parse a string of code
     * 
     * @param code The code to parse
     * @param contextPath Optional context path for the code
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult The result of parsing
     */
    virtual ParseResult parseCode(
        const std::string& code,
        const std::optional<std::string>& contextPath = std::nullopt,
        const std::vector<CodeSymbol>& existingSymbols = {}) = 0;
    
    /**
     * @brief Get the maximum parse context size
     * 
     * Some parsers may need to analyze the entire file to extract
     * accurate information about symbols. This method returns the
     * maximum number of bytes needed for context.
     * 
     * @return size_t The maximum context size in bytes
     */
    virtual size_t getMaxParseContextSize() const = 0;
    
    /**
     * @brief Check if incremental parsing is supported
     * 
     * @return bool True if incremental parsing is supported
     */
    virtual bool supportsIncrementalParsing() const = 0;
    
    /**
     * @brief Parse a file incrementally
     * 
     * @param filePath The path to the file
     * @param fileContent The content of the file
     * @param previousResult The previous parse result
     * @param startLine The line where changes start
     * @param endLine The line where changes end
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult The result of parsing
     */
    virtual ParseResult parseFileIncrementally(
        const std::string& filePath,
        const std::string& fileContent,
        const ParseResult& previousResult,
        int startLine,
        int endLine,
        const std::vector<CodeSymbol>& existingSymbols = {}) = 0;
};

/**
 * @interface ILanguageParserFactory
 * @brief Factory for creating language parsers
 */
class ILanguageParserFactory {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ILanguageParserFactory() = default;
    
    /**
     * @brief Create a parser for a specific language
     * 
     * @param languageId The language ID
     * @return std::shared_ptr<ILanguageParser> The parser, or nullptr if not supported
     */
    virtual std::shared_ptr<ILanguageParser> createParser(const std::string& languageId) = 0;
    
    /**
     * @brief Get all supported language IDs
     * 
     * @return std::vector<std::string> The supported language IDs
     */
    virtual std::vector<std::string> getSupportedLanguages() const = 0;
    
    /**
     * @brief Register a custom parser factory function
     * 
     * @param languageId The language ID
     * @param factoryFunction Function to create parsers for this language
     * @return bool True if registration was successful
     */
    virtual bool registerParserFactory(
        const std::string& languageId,
        std::function<std::shared_ptr<ILanguageParser>()> factoryFunction) = 0;
};

} // namespace ai_editor 