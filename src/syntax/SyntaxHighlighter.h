#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <imgui.h>

namespace ai_editor {

/**
 * @class SyntaxHighlightingRule
 * @brief Defines a single syntax highlighting rule
 */
class SyntaxHighlightingRule {
public:
    std::string pattern;
    ImVec4 color;
    bool isRegex = false;
    bool caseSensitive = false;
    
    /**
     * @brief Check if this rule matches the given token
     */
    bool matches(const std::string& token) const;
};

/**
 * @class SyntaxLanguage
 * @brief Defines a programming language with syntax highlighting rules
 */
class SyntaxLanguage {
public:
    std::string name;
    std::vector<std::string> fileExtensions;
    std::vector<std::string> fileNames;  // For special files like .gitignore, Makefile, etc.
    
    // Syntax highlighting rules
    std::vector<SyntaxHighlightingRule> rules;
    
    // Language-specific settings
    std::string lineComment;  // e.g., "//" for C++
    std::string blockCommentStart;  // e.g., "/*"
    std::string blockCommentEnd;    // e.g., "*/"
    
    // Common keywords
    std::vector<std::string> keywords;
    std::vector<std::string> types;
    std::vector<std::string> builtins;
    
    // Colors
    ImVec4 defaultColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    ImVec4 keywordColor = ImVec4(0.8f, 0.6f, 0.8f, 1.0f);
    ImVec4 typeColor = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
    ImVec4 stringColor = ImVec4(0.8f, 0.8f, 0.4f, 1.0f);
    ImVec4 numberColor = ImVec4(0.8f, 0.7f, 0.6f, 1.0f);
    ImVec4 commentColor = ImVec4(0.5f, 0.8f, 0.5f, 1.0f);
    ImVec4 preprocessorColor = ImVec4(0.8f, 0.5f, 0.5f, 1.0f);
    ImVec4 functionColor = ImVec4(0.4f, 0.8f, 0.8f, 1.0f);
    ImVec4 memberColor = ImVec4(0.8f, 0.8f, 0.4f, 1.0f);
    
    /**
     * @brief Initialize the language with default rules
     */
    void initialize();
    
    /**
     * @brief Add a keyword to the language
     */
    void addKeyword(const std::string& keyword) {
        keywords.push_back(keyword);
    }
    
    /**
     * @brief Add multiple keywords to the language
     */
    void addKeywords(const std::initializer_list<std::string>& words) {
        keywords.insert(keywords.end(), words);
    }
    
    /**
     * @brief Add a type to the language
     */
    void addType(const std::string& type) {
        types.push_back(type);
    }
    
    /**
     * @brief Add multiple types to the language
     */
    void addTypes(const std::initializer_list<std::string>& typeList) {
        types.insert(types.end(), typeList);
    }
    
    /**
     * @brief Add a built-in function/constant to the language
     */
    void addBuiltin(const std::string& builtin) {
        builtins.push_back(builtin);
    }
    
    /**
     * @brief Add multiple built-ins to the language
     */
    void addBuiltins(const std::initializer_list<std::string>& builtinList) {
        builtins.insert(builtins.end(), builtinList);
    }
    
    /**
     * @brief Add a file extension for this language
     */
    void addExtension(const std::string& ext) {
        fileExtensions.push_back(ext);
    }
    
    /**
     * @brief Check if this language supports the given file extension
     */
    bool supportsExtension(const std::string& ext) const {
        std::string lowerExt = ext;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
        
        for (const auto& e : fileExtensions) {
            if (e == lowerExt) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Check if this language supports the given file name
     */
    bool supportsFileName(const std::string& fileName) const {
        for (const auto& name : fileNames) {
            if (fileName == name) {
                return true;
            }
        }
        return false;
    }
};

/**
 * @class SyntaxHighlighter
 * @brief Manages syntax highlighting for different programming languages
 */
class SyntaxHighlighter {
public:
    /**
     * @brief Constructor
     */
    SyntaxHighlighter();
    
    /**
     * @brief Get the syntax highlighter instance (singleton)
     */
    static SyntaxHighlighter& getInstance();
    
    /**
     * @brief Initialize the syntax highlighter with built-in languages
     */
    void initialize();
    
    /**
     * @brief Get the language for the given file extension
     * @param ext File extension (with or without leading dot)
     * @return The language, or nullptr if not found
     */
    const SyntaxLanguage* getLanguageByExtension(const std::string& ext) const;
    
    /**
     * @brief Get the language for the given file name
     * @param fileName File name (for special files like Makefile)
     * @return The language, or nullptr if not found
     */
    const SyntaxLanguage* getLanguageByFileName(const std::string& fileName) const;
    
    /**
     * @brief Get the language by name
     */
    const SyntaxLanguage* getLanguageByName(const std::string& name) const;
    
    /**
     * @brief Get all available languages
     */
    const std::vector<SyntaxLanguage>& getLanguages() const { return languages_; }
    
    /**
     * @brief Add a custom language
     */
    void addLanguage(const SyntaxLanguage& language);
    
    /**
     * @brief Tokenize and apply syntax highlighting to a line of text
     * @param line The line of text to highlight
     * @param language The language to use for highlighting
     * @param outTokens Output vector of tokens (text and color pairs)
     */
    void highlightLine(const std::string& line, 
                      const SyntaxLanguage* language,
                      std::vector<std::pair<std::string, ImVec4>>& outTokens) const;
    
    /**
     * @brief Check if the given position in the line is inside a string literal
     */
    bool isInStringLiteral(const std::string& line, size_t pos, 
                          const SyntaxLanguage* language) const;
    
    /**
     * @brief Check if the given position in the line is inside a comment
     */
    bool isInComment(const std::string& line, size_t pos,
                    const SyntaxLanguage* language) const;
    
    /**
     * @brief Get the color for a token in the given language
     */
    ImVec4 getTokenColor(const std::string& token, const SyntaxLanguage* language) const;
    
private:
    std::vector<SyntaxLanguage> languages_;
    
    // Built-in language definitions
    void initCPP();
    void initPython();
    void initJavaScript();
    void initJava();
    void initCSharp();
    void initGo();
    void initRust();
    void initRuby();
    void initPHP();
    void initSwift();
    void initKotlin();
    void initShell();
    void initJSON();
    void initXML();
    void initCSS();
    void initMarkdown();
    void initYAML();
    void initTOML();
    void initINI();
    void initSQL();
    
    // Helper functions
    bool isWhitespace(char c) const {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }
    
    bool isWordChar(char c) const {
        return (c >= 'a' && c <= 'z') || 
               (c >= 'A' && c <= 'Z') || 
               (c >= '0' && c <= '9') || 
               c == '_';
    }
    
    bool isDigit(char c) const {
        return c >= '0' && c <= '9';
    }
};

} // namespace ai_editor
