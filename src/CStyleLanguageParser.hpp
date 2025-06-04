#pragma once

#include "BaseLanguageParser.hpp"
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ai_editor {

/**
 * @brief Parser for C-style languages (C, C++, Java)
 * 
 * This parser handles basic parsing of C-style languages using regex-based
 * pattern matching to extract symbols and their relationships.
 */
class CStyleLanguageParser : public BaseLanguageParser {
public:
    /**
     * @brief Constructor for the CStyleLanguageParser
     * @param languageId The ID of the language this parser handles (e.g., "cpp", "java")
     */
    explicit CStyleLanguageParser(const std::string& languageId);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~CStyleLanguageParser() = default;
    
protected:
    /**
     * @brief Implementation of code parsing for C-style languages
     * @param code The code to parse
     * @param filePath Optional file path for context
     * @param existingSymbols Existing symbols that may be referenced
     * @return ParseResult containing found symbols, references, and relations
     */
    ParseResult parseCodeImpl(
        const std::string& code,
        const std::optional<std::string>& filePath = std::nullopt,
        const std::vector<CodeSymbol>& existingSymbols = {}) override;
    
private:
    /**
     * @brief Extract classes and structs from the code
     * @param code The code to parse
     * @param result ParseResult to add found symbols to
     * @param filePath File path for context
     */
    void extractClassesAndStructs(const std::string& code, ParseResult& result, const std::string& filePath);
    
    /**
     * @brief Extract functions from the code
     * @param code The code to parse
     * @param result ParseResult to add found symbols to
     * @param filePath File path for context
     */
    void extractFunctions(const std::string& code, ParseResult& result, const std::string& filePath);
    
    /**
     * @brief Extract variables and fields from the code
     * @param code The code to parse
     * @param result ParseResult to add found symbols to
     * @param filePath File path for context
     */
    void extractVariablesAndFields(const std::string& code, ParseResult& result, const std::string& filePath);
    
    /**
     * @brief Extract namespace definitions from the code
     * @param code The code to parse
     * @param result ParseResult to add found symbols to
     * @param filePath File path for context
     */
    void extractNamespaces(const std::string& code, ParseResult& result, const std::string& filePath);
    
    /**
     * @brief Extract inheritance and implementation relationships
     * @param code The code to parse
     * @param result ParseResult to add found relations to
     */
    void extractInheritanceRelations(const std::string& code, ParseResult& result);
    
    /**
     * @brief Extract method calls and symbol references
     * @param code The code to parse
     * @param result ParseResult to add found references to
     * @param filePath File path for context
     */
    void extractReferences(const std::string& code, ParseResult& result, const std::string& filePath);
    
    /**
     * @brief Get line number from character position
     * @param code The code to parse
     * @param pos Character position in the code
     * @return Line number (1-based)
     */
    size_t getLineNumber(const std::string& code, size_t pos) const;
    
    /**
     * @brief Get column number from character position and line number
     * @param code The code to parse
     * @param pos Character position in the code
     * @param lineNumber Line number (1-based)
     * @return Column number (1-based)
     */
    size_t getColumnNumber(const std::string& code, size_t pos, size_t lineNumber) const;
    
    /**
     * @brief Get line and column numbers from character position
     * @param code The code to parse
     * @param pos Character position in the code
     * @return Pair of line and column numbers (both 1-based)
     */
    std::pair<size_t, size_t> getLineAndColumn(const std::string& code, size_t pos) const;
    
    /**
     * @brief Remove comments and string literals from code for easier parsing
     * @param code The original code
     * @return Code with comments and string literals replaced by spaces
     */
    std::string preprocessCode(const std::string& code) const;
    
    /**
     * @brief Find matching closing bracket for an opening bracket
     * @param code The code to parse
     * @param openPos Position of the opening bracket
     * @param openBracket Opening bracket character
     * @param closeBracket Closing bracket character
     * @return Position of the matching closing bracket, or string::npos if not found
     */
    size_t findMatchingBracket(const std::string& code, size_t openPos, 
                              char openBracket, char closeBracket) const;
    
    /** Regular expressions for parsing different constructs */
    std::regex classRegex_;
    std::regex structRegex_;
    std::regex functionRegex_;
    std::regex methodRegex_;
    std::regex variableRegex_;
    std::regex fieldRegex_;
    std::regex namespaceRegex_;
    std::regex inheritanceRegex_;
    std::regex methodCallRegex_;
    std::regex variableRefRegex_;
    
    /** Maps to track parsed symbols by name */
    std::unordered_map<std::string, std::string> classSymbols_;
    std::unordered_map<std::string, std::string> functionSymbols_;
    std::unordered_map<std::string, std::string> variableSymbols_;
    std::unordered_map<std::string, std::string> namespaceSymbols_;
};

/**
 * @brief Factory for creating C-style language parsers
 */
class CStyleLanguageParserFactory : public ILanguageParserFactory {
public:
    /**
     * @brief Constructor
     */
    CStyleLanguageParserFactory();
    
    /**
     * @brief Create a parser for the specified language
     * @param languageId The language ID to create a parser for
     * @return A shared pointer to the created parser, or nullptr if not supported
     */
    std::shared_ptr<ILanguageParser> createParser(const std::string& languageId) override;
    
    /**
     * @brief Get a list of supported language IDs
     * @return Vector of supported language IDs
     */
    std::vector<std::string> getSupportedLanguages() const override;
    
    /**
     * @brief Register a custom parser factory function
     * @param languageId The language ID to register the factory for
     * @param factory The factory function to call to create parsers for this language
     * @return True if registration was successful, false otherwise
     */
    bool registerParserFactory(
        const std::string& languageId,
        std::function<std::shared_ptr<ILanguageParser>()> factory) override;
    
private:
    /** Map of language IDs to factory functions */
    std::unordered_map<std::string, std::function<std::shared_ptr<ILanguageParser>()>> factories_;
};

} // namespace ai_editor 