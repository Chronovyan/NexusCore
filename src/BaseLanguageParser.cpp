#include "BaseLanguageParser.hpp"
#include "LanguageDetector.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <utility>
#include <uuid.h> // For generating unique IDs

namespace fs = std::filesystem;

namespace ai_editor {

BaseLanguageParser::BaseLanguageParser(const std::string& languageId)
    : languageId_(languageId) {
}

std::string BaseLanguageParser::getLanguageId() const {
    return languageId_;
}

bool BaseLanguageParser::canHandleFile(
    const std::string& filePath,
    const std::optional<std::string>& languageId) const {
    
    // If language ID is provided, check if it matches
    if (languageId.has_value()) {
        return languageId.value() == languageId_;
    }
    
    // Otherwise, try to detect language from file path
    LanguageDetector detector;
    auto detectedLanguage = detector.detectLanguageFromPath(filePath);
    
    if (detectedLanguage.has_value()) {
        return detectedLanguage->id == languageId_;
    }
    
    return false;
}

ParseResult BaseLanguageParser::parseFile(
    const std::string& filePath,
    const std::optional<std::string>& content) {
    
    // Check if we can handle this file
    if (!canHandleFile(filePath)) {
        ParseResult result;
        result.success = false;
        result.errorMessage = "File type not supported by this parser";
        return result;
    }
    
    // Get file content
    std::string fileContent;
    if (content.has_value()) {
        fileContent = content.value();
    } else {
        fileContent = readFileContent(filePath);
        if (fileContent.empty()) {
            ParseResult result;
            result.success = false;
            result.errorMessage = "Failed to read file: " + filePath;
            return result;
        }
    }
    
    // Parse the content
    return parseCode(fileContent, filePath);
}

ParseResult BaseLanguageParser::parseCode(
    const std::string& code,
    const std::optional<std::string>& filePath) {
    
    try {
        // Call implementation in derived class
        return parseCodeImpl(code, filePath);
    } catch (const std::exception& e) {
        ParseResult result;
        result.success = false;
        result.errorMessage = "Exception during parsing: " + std::string(e.what());
        return result;
    } catch (...) {
        ParseResult result;
        result.success = false;
        result.errorMessage = "Unknown exception during parsing";
        return result;
    }
}

size_t BaseLanguageParser::getMaxParseContextSize() const {
    return maxParseContextSize_;
}

bool BaseLanguageParser::supportsIncrementalParsing() const {
    return supportsIncrementalParsing_;
}

ParseResult BaseLanguageParser::parseFileIncrementally(
    const std::string& filePath,
    const ParseResult& previousResult,
    const std::vector<std::pair<size_t, size_t>>& changedLineRanges,
    const std::optional<std::string>& content) {
    
    // Default implementation just does a full parse
    // Derived classes should override this if they support incremental parsing
    return parseFile(filePath, content);
}

std::string BaseLanguageParser::readFileContent(const std::string& filePath) const {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (...) {
        return "";
    }
}

std::string BaseLanguageParser::addSymbol(
    ParseResult& result,
    const std::string& name,
    const std::string& kind,
    const std::string& filePath,
    size_t startLine,
    size_t startColumn,
    size_t endLine,
    size_t endColumn,
    const std::optional<std::string>& containerSymbolId,
    const std::unordered_map<std::string, std::string>& properties) {
    
    // Generate a unique ID for the symbol
    uuids::uuid id = uuids::uuid_system_generator{}();
    std::string symbolId = uuids::to_string(id);
    
    // Create the symbol
    CodeSymbol symbol;
    symbol.id = symbolId;
    symbol.name = name;
    symbol.kind = kind;
    symbol.filePath = filePath;
    symbol.startLine = startLine;
    symbol.startColumn = startColumn;
    symbol.endLine = endLine;
    symbol.endColumn = endColumn;
    symbol.containerSymbolId = containerSymbolId;
    symbol.properties = properties;
    
    // Add to result
    result.symbols.push_back(std::move(symbol));
    
    return symbolId;
}

std::string BaseLanguageParser::addReference(
    ParseResult& result,
    const std::string& symbolId,
    const std::string& filePath,
    size_t startLine,
    size_t startColumn,
    size_t endLine,
    size_t endColumn,
    bool isDefinition,
    const std::optional<std::string>& containerSymbolId) {
    
    // Generate a unique ID for the reference
    uuids::uuid id = uuids::uuid_system_generator{}();
    std::string referenceId = uuids::to_string(id);
    
    // Create the reference
    SymbolReference reference;
    reference.id = referenceId;
    reference.symbolId = symbolId;
    reference.filePath = filePath;
    reference.startLine = startLine;
    reference.startColumn = startColumn;
    reference.endLine = endLine;
    reference.endColumn = endColumn;
    reference.isDefinition = isDefinition;
    reference.containerSymbolId = containerSymbolId;
    
    // Add to result
    result.references.push_back(std::move(reference));
    
    return referenceId;
}

void BaseLanguageParser::addRelation(
    ParseResult& result,
    const std::string& sourceSymbolId,
    const std::string& targetSymbolId,
    const std::string& relationType,
    const std::unordered_map<std::string, std::string>& properties) {
    
    // Generate a unique ID for the relation
    uuids::uuid id = uuids::uuid_system_generator{}();
    std::string relationId = uuids::to_string(id);
    
    // Create the relation
    SymbolRelation relation;
    relation.id = relationId;
    relation.sourceSymbolId = sourceSymbolId;
    relation.targetSymbolId = targetSymbolId;
    relation.type = relationType;
    relation.properties = properties;
    
    // Add to result
    result.relations.push_back(std::move(relation));
}

} // namespace ai_editor 