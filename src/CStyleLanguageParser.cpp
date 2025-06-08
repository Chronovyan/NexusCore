#include "CStyleLanguageParser.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace ai_editor {

CStyleLanguageParser::CStyleLanguageParser(const std::string& languageId)
    : BaseLanguageParser(languageId) {
    
    // Initialize regular expressions for parsing
    classRegex_ = std::regex(
        "\\b(class|interface)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*(?::\\s*(?:public|protected|private)\\s+([A-Za-z_][A-Za-z0-9_:]*))?"
    );
    
    structRegex_ = std::regex(
        "\\bstruct\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*(?::\\s*(?:public|protected|private)\\s+([A-Za-z_][A-Za-z0-9_:]*))?"
    );
    
    functionRegex_ = std::regex(
        "\\b([A-Za-z_][A-Za-z0-9_:]*)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(([^)]*)\\)\\s*(?:const)?\\s*(?:[^;{]*)(?:\\{|;)"
    );
    
    methodRegex_ = std::regex(
        "\\b([A-Za-z_][A-Za-z0-9_:]*)\\s+([A-Za-z_][A-Za-z0-9_:]*)::([A-Za-z_][A-Za-z0-9_]*)\\s*\\(([^)]*)\\)\\s*(?:const)?\\s*(?:[^;{]*)(?:\\{|;)"
    );
    
    variableRegex_ = std::regex(
        "\\b([A-Za-z_][A-Za-z0-9_:]*)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*(?:=\\s*[^;]+)?\\s*;"
    );
    
    fieldRegex_ = std::regex(
        "\\s+([A-Za-z_][A-Za-z0-9_:]*)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*(?:=\\s*[^;]+)?\\s*;"
    );
    
    namespaceRegex_ = std::regex(
        "\\bnamespace\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\{"
    );
    
    inheritanceRegex_ = std::regex(
        "\\b(?:class|struct)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*:\\s*(?:public|protected|private)\\s+([A-Za-z_][A-Za-z0-9_:]*)"
    );
    
    methodCallRegex_ = std::regex(
        "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*\\("
    );
    
    variableRefRegex_ = std::regex(
        "\\b([A-Za-z_][A-Za-z0-9_]*)\\b"
    );
}

ParseResult CStyleLanguageParser::parseCodeImpl(
    const std::string& code,
    const std::optional<std::string>& filePath) {
    
    ParseResult result;
    result.success = true;
    
    try {
        // Clear maps of symbols
        classSymbols_.clear();
        functionSymbols_.clear();
        variableSymbols_.clear();
        namespaceSymbols_.clear();
        
        // Preprocess the code to remove comments and string literals
        std::string preprocessedCode = preprocessCode(code);
        
        // Get the actual file path
        std::string actualFilePath = filePath.value_or("unknown_file");
        
        // Extract different types of symbols
        extractNamespaces(preprocessedCode, result, actualFilePath);
        extractClassesAndStructs(preprocessedCode, result, actualFilePath);
        extractFunctions(preprocessedCode, result, actualFilePath);
        extractVariablesAndFields(preprocessedCode, result, actualFilePath);
        
        // Extract relationships and references
        extractInheritanceRelations(preprocessedCode, result);
        extractReferences(preprocessedCode, result, actualFilePath);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Exception during parsing: " + std::string(e.what());
    } catch (...) {
        result.success = false;
        result.errorMessage = "Unknown exception during parsing";
    }
    
    return result;
}

void CStyleLanguageParser::extractClassesAndStructs(
    const std::string& code,
    ParseResult& result,
    const std::string& filePath) {
    
    // Extract classes
    std::sregex_iterator classIt(code.begin(), code.end(), classRegex_);
    std::sregex_iterator end;
    
    for (; classIt != end; ++classIt) {
        std::smatch match = *classIt;
        std::string kind = match[1].str(); // "class" or "interface"
        std::string className = match[2].str();
        
        // Find the opening brace for the class definition
        size_t pos = match.position() + match.length();
        size_t openBracePos = code.find('{', pos);
        
        if (openBracePos != std::string::npos) {
            // Find the closing brace
            size_t closeBracePos = findMatchingBracket(code, openBracePos, '{', '}');
            
            if (closeBracePos != std::string::npos) {
                // Get line and column information
                auto [startLine, startColumn] = getLineAndColumn(code, match.position());
                auto [endLine, endColumn] = getLineAndColumn(code, closeBracePos);
                
                // Add the class symbol
                std::string symbolId = addSymbol(
                    result,
                    className,
                    kind,
                    filePath,
                    startLine,
                    startColumn,
                    endLine,
                    endColumn
                );
                
                // Store the class symbol ID
                classSymbols_[className] = symbolId;
            }
        }
    }
    
    // Extract structs
    std::sregex_iterator structIt(code.begin(), code.end(), structRegex_);
    
    for (; structIt != end; ++structIt) {
        std::smatch match = *structIt;
        std::string structName = match[1].str();
        
        // Find the opening brace for the struct definition
        size_t pos = match.position() + match.length();
        size_t openBracePos = code.find('{', pos);
        
        if (openBracePos != std::string::npos) {
            // Find the closing brace
            size_t closeBracePos = findMatchingBracket(code, openBracePos, '{', '}');
            
            if (closeBracePos != std::string::npos) {
                // Get line and column information
                auto [startLine, startColumn] = getLineAndColumn(code, match.position());
                auto [endLine, endColumn] = getLineAndColumn(code, closeBracePos);
                
                // Add the struct symbol
                std::string symbolId = addSymbol(
                    result,
                    structName,
                    "struct",
                    filePath,
                    startLine,
                    startColumn,
                    endLine,
                    endColumn
                );
                
                // Store the struct symbol ID
                classSymbols_[structName] = symbolId;
            }
        }
    }
}

void CStyleLanguageParser::extractFunctions(
    const std::string& code,
    ParseResult& result,
    const std::string& filePath) {
    
    // Extract global functions
    std::sregex_iterator funcIt(code.begin(), code.end(), functionRegex_);
    std::sregex_iterator end;
    
    for (; funcIt != end; ++funcIt) {
        std::smatch match = *funcIt;
        std::string returnType = match[1].str();
        std::string functionName = match[2].str();
        std::string parameters = match[3].str();
        
        // Check if this is inside a class or namespace
        // For simplicity, we'll assume it's a global function
        
        // Find the opening brace for the function definition
        size_t pos = match.position() + match.length();
        size_t openBracePos = code.find('{', pos);
        
        if (openBracePos != std::string::npos) {
            // Find the closing brace
            size_t closeBracePos = findMatchingBracket(code, openBracePos, '{', '}');
            
            if (closeBracePos != std::string::npos) {
                // Get line and column information
                auto [startLine, startColumn] = getLineAndColumn(code, match.position());
                auto [endLine, endColumn] = getLineAndColumn(code, closeBracePos);
                
                // Add properties
                std::unordered_map<std::string, std::string> properties;
                properties["returnType"] = returnType;
                properties["parameters"] = parameters;
                
                // Add the function symbol
                std::string symbolId = addSymbol(
                    result,
                    functionName,
                    "function",
                    filePath,
                    startLine,
                    startColumn,
                    endLine,
                    endColumn,
                    std::nullopt, // No container for global functions
                    properties
                );
                
                // Store the function symbol ID
                functionSymbols_[functionName] = symbolId;
            }
        }
    }
    
    // Extract class methods
    std::sregex_iterator methodIt(code.begin(), code.end(), methodRegex_);
    
    for (; methodIt != end; ++methodIt) {
        std::smatch match = *methodIt;
        std::string returnType = match[1].str();
        std::string className = match[2].str();
        std::string methodName = match[3].str();
        std::string parameters = match[4].str();
        
        // Find the class symbol
        auto classIt = classSymbols_.find(className);
        std::optional<std::string> containerSymbolId = std::nullopt;
        
        if (classIt != classSymbols_.end()) {
            containerSymbolId = classIt->second;
        }
        
        // Find the opening brace for the method definition
        size_t pos = match.position() + match.length();
        size_t openBracePos = code.find('{', pos);
        
        if (openBracePos != std::string::npos) {
            // Find the closing brace
            size_t closeBracePos = findMatchingBracket(code, openBracePos, '{', '}');
            
            if (closeBracePos != std::string::npos) {
                // Get line and column information
                auto [startLine, startColumn] = getLineAndColumn(code, match.position());
                auto [endLine, endColumn] = getLineAndColumn(code, closeBracePos);
                
                // Add properties
                std::unordered_map<std::string, std::string> properties;
                properties["returnType"] = returnType;
                properties["parameters"] = parameters;
                properties["className"] = className;
                
                // Add the method symbol
                std::string symbolId = addSymbol(
                    result,
                    methodName,
                    "method",
                    filePath,
                    startLine,
                    startColumn,
                    endLine,
                    endColumn,
                    containerSymbolId,
                    properties
                );
                
                // Store the method symbol ID
                functionSymbols_[className + "::" + methodName] = symbolId;
            }
        }
    }
}

void CStyleLanguageParser::extractVariablesAndFields(
    const std::string& code,
    ParseResult& result,
    const std::string& filePath) {
    
    // Extract global variables
    std::sregex_iterator varIt(code.begin(), code.end(), variableRegex_);
    std::sregex_iterator end;
    
    for (; varIt != end; ++varIt) {
        std::smatch match = *varIt;
        std::string type = match[1].str();
        std::string name = match[2].str();
        
        // Get line and column information
        auto [startLine, startColumn] = getLineAndColumn(code, match.position());
        auto [endLine, endColumn] = getLineAndColumn(code, match.position() + match.length());
        
        // Add properties
        std::unordered_map<std::string, std::string> properties;
        properties["type"] = type;
        
        // Add the variable symbol
        std::string symbolId = addSymbol(
            result,
            name,
            "variable",
            filePath,
            startLine,
            startColumn,
            endLine,
            endColumn,
            std::nullopt, // No container for global variables
            properties
        );
        
        // Store the variable symbol ID
        variableSymbols_[name] = symbolId;
    }
    
    // Extract class fields (simple approach, not perfect)
    // We'll look for field-like declarations inside class/struct definitions
    for (const auto& [className, classSymbolId] : classSymbols_) {
        auto classSymbol = result.symbols.end();
        
        // Find the class symbol
        for (auto it = result.symbols.begin(); it != result.symbols.end(); ++it) {
            if (it->id == classSymbolId) {
                classSymbol = it;
                break;
            }
        }
        
        if (classSymbol != result.symbols.end()) {
            // Find the class definition in the code
            size_t classDefStart = code.find("class " + className);
            if (classDefStart == std::string::npos) {
                classDefStart = code.find("struct " + className);
            }
            
            if (classDefStart != std::string::npos) {
                // Find the opening brace
                size_t openBracePos = code.find('{', classDefStart);
                if (openBracePos != std::string::npos) {
                    // Find the closing brace
                    size_t closeBracePos = findMatchingBracket(code, openBracePos, '{', '}');
                    
                    if (closeBracePos != std::string::npos) {
                        // Extract the class body
                        std::string classBody = code.substr(openBracePos + 1, closeBracePos - openBracePos - 1);
                        
                        // Find fields in the class body
                        std::sregex_iterator fieldIt(classBody.begin(), classBody.end(), fieldRegex_);
                        
                        for (; fieldIt != end; ++fieldIt) {
                            std::smatch match = *fieldIt;
                            std::string type = match[1].str();
                            std::string name = match[2].str();
                            
                            // Get line and column information relative to the class body
                            size_t matchPosInBody = match.position();
                            size_t matchPosInCode = openBracePos + 1 + matchPosInBody;
                            
                            auto [startLine, startColumn] = getLineAndColumn(code, matchPosInCode);
                            auto [endLine, endColumn] = getLineAndColumn(code, matchPosInCode + match.length());
                            
                            // Add properties
                            std::unordered_map<std::string, std::string> properties;
                            properties["type"] = type;
                            properties["className"] = className;
                            
                            // Add the field symbol
                            std::string symbolId = addSymbol(
                                result,
                                name,
                                "field",
                                filePath,
                                startLine,
                                startColumn,
                                endLine,
                                endColumn,
                                classSymbolId,
                                properties
                            );
                            
                            // Store the field symbol ID
                            variableSymbols_[className + "::" + name] = symbolId;
                        }
                    }
                }
            }
        }
    }
}

void CStyleLanguageParser::extractNamespaces(
    const std::string& code,
    ParseResult& result,
    const std::string& filePath) {
    
    std::sregex_iterator nsIt(code.begin(), code.end(), namespaceRegex_);
    std::sregex_iterator end;
    
    for (; nsIt != end; ++nsIt) {
        std::smatch match = *nsIt;
        std::string namespaceName = match[1].str();
        
        // Find the opening brace for the namespace definition
        size_t pos = match.position() + match.length();
        size_t openBracePos = code.find('{', pos);
        
        if (openBracePos != std::string::npos) {
            // Find the closing brace
            size_t closeBracePos = findMatchingBracket(code, openBracePos, '{', '}');
            
            if (closeBracePos != std::string::npos) {
                // Get line and column information
                auto [startLine, startColumn] = getLineAndColumn(code, match.position());
                auto [endLine, endColumn] = getLineAndColumn(code, closeBracePos);
                
                // Add the namespace symbol
                std::string symbolId = addSymbol(
                    result,
                    namespaceName,
                    "namespace",
                    filePath,
                    startLine,
                    startColumn,
                    endLine,
                    endColumn
                );
                
                // Store the namespace symbol ID
                namespaceSymbols_[namespaceName] = symbolId;
            }
        }
    }
}

void CStyleLanguageParser::extractInheritanceRelations(
    const std::string& code,
    ParseResult& result) {
    
    std::sregex_iterator inheritIt(code.begin(), code.end(), inheritanceRegex_);
    std::sregex_iterator end;
    
    for (; inheritIt != end; ++inheritIt) {
        std::smatch match = *inheritIt;
        std::string derivedClassName = match[1].str();
        std::string baseClassName = match[2].str();
        
        // Find the derived class symbol
        auto derivedClassIt = classSymbols_.find(derivedClassName);
        
        // Find the base class symbol
        auto baseClassIt = classSymbols_.find(baseClassName);
        
        // If both symbols exist, add an inheritance relation
        if (derivedClassIt != classSymbols_.end() && baseClassIt != classSymbols_.end()) {
            addRelation(
                result,
                derivedClassIt->second,
                baseClassIt->second,
                "inherits"
            );
        }
    }
}

void CStyleLanguageParser::extractReferences(
    const std::string& code,
    ParseResult& result,
    const std::string& filePath) {
    
    // This is a simplified approach to find references.
    // A more robust implementation would need to track scopes and visibility.
    
    // Extract method calls
    std::sregex_iterator methodCallIt(code.begin(), code.end(), methodCallRegex_);
    std::sregex_iterator end;
    
    for (; methodCallIt != end; ++methodCallIt) {
        std::smatch match = *methodCallIt;
        std::string methodName = match[1].str();
        
        // Find the function symbol
        auto funcIt = functionSymbols_.find(methodName);
        
        if (funcIt != functionSymbols_.end()) {
            // Get line and column information
            auto [startLine, startColumn] = getLineAndColumn(code, match.position());
            auto [endLine, endColumn] = getLineAndColumn(code, match.position() + match.length());
            
            // Add the reference
            addReference(
                result,
                funcIt->second,
                filePath,
                startLine,
                startColumn,
                endLine,
                endColumn,
                false // Not a definition
            );
        }
    }
    
    // Extract variable references
    std::sregex_iterator varRefIt(code.begin(), code.end(), variableRefRegex_);
    
    for (; varRefIt != end; ++varRefIt) {
        std::smatch match = *varRefIt;
        std::string varName = match[1].str();
        
        // Find the variable symbol
        auto varIt = variableSymbols_.find(varName);
        
        if (varIt != variableSymbols_.end()) {
            // Get line and column information
            auto [startLine, startColumn] = getLineAndColumn(code, match.position());
            auto [endLine, endColumn] = getLineAndColumn(code, match.position() + match.length());
            
            // Add the reference
            addReference(
                result,
                varIt->second,
                filePath,
                startLine,
                startColumn,
                endLine,
                endColumn,
                false // Not a definition
            );
        }
    }
}

size_t CStyleLanguageParser::getLineNumber(const std::string& code, size_t pos) const {
    if (pos >= code.length()) {
        return 1;
    }
    
    size_t lineNumber = 1;
    for (size_t i = 0; i < pos; ++i) {
        if (code[i] == '\n') {
            ++lineNumber;
        }
    }
    
    return lineNumber;
}

size_t CStyleLanguageParser::getColumnNumber(const std::string& code, size_t pos, size_t lineNumber) const {
    if (pos >= code.length()) {
        return 1;
    }
    
    // Find the start of the line
    size_t lineStart = 0;
    size_t currentLine = 1;
    
    for (size_t i = 0; i < pos; ++i) {
        if (code[i] == '\n') {
            ++currentLine;
            if (currentLine == lineNumber) {
                lineStart = i + 1;
            }
        }
    }
    
    return pos - lineStart + 1;
}

std::pair<size_t, size_t> CStyleLanguageParser::getLineAndColumn(const std::string& code, size_t pos) const {
    size_t lineNumber = getLineNumber(code, pos);
    size_t columnNumber = getColumnNumber(code, pos, lineNumber);
    
    return {lineNumber, columnNumber};
}

std::string CStyleLanguageParser::preprocessCode(const std::string& code) const {
    std::string result = code;
    
    // Replace contents of string literals with spaces
    size_t pos = 0;
    while ((pos = result.find('"', pos)) != std::string::npos) {
        // Skip escaped quotes
        if (pos > 0 && result[pos - 1] == '\\') {
            pos++;
            continue;
        }
        
        // Find the closing quote
        size_t endPos = pos + 1;
        while (endPos < result.length()) {
            if (result[endPos] == '"' && result[endPos - 1] != '\\') {
                break;
            }
            endPos++;
        }
        
        if (endPos < result.length()) {
            // Replace the string content with spaces
            for (size_t i = pos + 1; i < endPos; ++i) {
                result[i] = ' ';
            }
            pos = endPos + 1;
        } else {
            // No closing quote found
            break;
        }
    }
    
    // Replace contents of character literals with spaces
    pos = 0;
    while ((pos = result.find('\'', pos)) != std::string::npos) {
        // Skip escaped quotes
        if (pos > 0 && result[pos - 1] == '\\') {
            pos++;
            continue;
        }
        
        // Find the closing quote
        size_t endPos = pos + 1;
        while (endPos < result.length()) {
            if (result[endPos] == '\'' && result[endPos - 1] != '\\') {
                break;
            }
            endPos++;
        }
        
        if (endPos < result.length()) {
            // Replace the character content with spaces
            for (size_t i = pos + 1; i < endPos; ++i) {
                result[i] = ' ';
            }
            pos = endPos + 1;
        } else {
            // No closing quote found
            break;
        }
    }
    
    // Replace C-style comments with spaces
    pos = 0;
    while ((pos = result.find("/*", pos)) != std::string::npos) {
        size_t endPos = result.find("*/", pos + 2);
        
        if (endPos != std::string::npos) {
            // Replace the comment content with spaces
            for (size_t i = pos; i < endPos + 2; ++i) {
                result[i] = ' ';
            }
            pos = endPos + 2;
        } else {
            // No closing comment found
            break;
        }
    }
    
    // Replace C++-style comments with spaces
    pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos) {
        size_t endPos = result.find('\n', pos + 2);
        
        if (endPos != std::string::npos) {
            // Replace the comment content with spaces
            for (size_t i = pos; i < endPos; ++i) {
                result[i] = ' ';
            }
            pos = endPos + 1;
        } else {
            // Comment goes to the end of the file
            for (size_t i = pos; i < result.length(); ++i) {
                result[i] = ' ';
            }
            break;
        }
    }
    
    return result;
}

size_t CStyleLanguageParser::findMatchingBracket(
    const std::string& code,
    size_t openPos,
    char openBracket,
    char closeBracket) const {
    
    if (openPos >= code.length() || code[openPos] != openBracket) {
        return std::string::npos;
    }
    
    int depth = 1;
    size_t pos = openPos + 1;
    
    while (pos < code.length() && depth > 0) {
        if (code[pos] == openBracket) {
            depth++;
        } else if (code[pos] == closeBracket) {
            depth--;
        }
        
        if (depth == 0) {
            return pos;
        }
        
        pos++;
    }
    
    return std::string::npos;
}

// CStyleLanguageParserFactory implementation

CStyleLanguageParserFactory::CStyleLanguageParserFactory() {
    // Register factory functions for supported languages
    factories_["c"] = []() {
        return std::make_shared<CStyleLanguageParser>("c");
    };
    
    factories_["cpp"] = []() {
        return std::make_shared<CStyleLanguageParser>("cpp");
    };
    
    factories_["java"] = []() {
        return std::make_shared<CStyleLanguageParser>("java");
    };
}

std::shared_ptr<ILanguageParser> CStyleLanguageParserFactory::createParserForLanguage(
    const std::string& languageId) {
    
    auto it = factories_.find(languageId);
    if (it != factories_.end()) {
        return it->second();
    }
    
    return nullptr;
}

std::vector<std::string> CStyleLanguageParserFactory::getSupportedLanguages() const {
    std::vector<std::string> languages;
    languages.reserve(factories_.size());
    
    for (const auto& [languageId, _] : factories_) {
        languages.push_back(languageId);
    }
    
    return languages;
}

bool CStyleLanguageParserFactory::registerParserFactory(
    const std::string& languageId,
    std::function<std::shared_ptr<ILanguageParser>()> factory) {
    
    // Check if a factory for this language already exists
    if (factories_.find(languageId) != factories_.end()) {
        return false;
    }
    
    factories_[languageId] = factory;
    return true;
}

} // namespace ai_editor 