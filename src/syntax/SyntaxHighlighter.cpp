#include "SyntaxHighlighter.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace ai_editor {

bool SyntaxHighlightingRule::matches(const std::string& token) const {
    if (isRegex) {
        try {
            std::regex_constants::syntax_option_type flags = 
                std::regex_constants::ECMAScript;
            if (!caseSensitive) {
                flags |= std::regex_constants::icase;
            }
            std::regex re(pattern, flags);
            return std::regex_match(token, re);
        } catch (const std::regex_error&) {
            return false;
        }
    } else {
        if (caseSensitive) {
            return token == pattern;
        } else {
            if (token.length() != pattern.length()) {
                return false;
            }
            return std::equal(token.begin(), token.end(), pattern.begin(),
                           [](char a, char b) { 
                               return std::tolower(a) == std::tolower(b); 
                           });
        }
    }
}

void SyntaxLanguage::initialize() {
    // Default initialization for a language
    rules.clear();
    
    // Add rules for common syntax elements
    // Note: The order of rules matters - first matching rule is used
    
    // Line comments
    if (!lineComment.empty()) {
        SyntaxHighlightingRule commentRule;
        commentRule.pattern = lineComment + ".*$";
        commentRule.color = commentColor;
        commentRule.isRegex = true;
        rules.push_back(commentRule);
    }
    
    // String literals (simplified)
    SyntaxHighlightingRule stringRule;
    stringRule.pattern = "\"(\\.|[^\\\"])*\"";
    stringRule.color = stringColor;
    stringRule.isRegex = true;
    rules.push_back(stringRule);
    
    // Character literals
    SyntaxHighlightingRule charRule;
    charRule.pattern = "'(\\.|[^\\'])'";
    charRule.color = stringColor;
    charRule.isRegex = true;
    rules.push_back(charRule);
    
    // Numbers
    SyntaxHighlightingRule numberRule;
    numberRule.pattern = "\\b[0-9]+(\\.[0-9]*)?([eE][+-]?[0-9]+)?\\b";
    numberRule.color = numberColor;
    numberRule.isRegex = true;
    rules.push_back(numberRule);
    
    // Hex numbers
    SyntaxHighlightingRule hexRule;
    hexRule.pattern = "\\b0x[0-9a-fA-F]+\\b";
    hexRule.color = numberColor;
    hexRule.isRegex = true;
    rules.push_back(hexRule);
    
    // Keywords
    if (!keywords.empty()) {
        std::ostringstream keywordPattern;
        keywordPattern << "\\b(";
        bool first = true;
        for (const auto& kw : keywords) {
            if (!first) {
                keywordPattern << "|";
            }
            keywordPattern << kw;
            first = false;
        }
        keywordPattern << ")\\b";
        
        SyntaxHighlightingRule keywordRule;
        keywordRule.pattern = keywordPattern.str();
        keywordRule.color = keywordColor;
        keywordRule.isRegex = true;
        keywordRule.caseSensitive = false;
        rules.push_back(keywordRule);
    }
    
    // Types
    if (!types.empty()) {
        std::ostringstream typePattern;
        typePattern << "\\b(";
        bool first = true;
        for (const auto& t : types) {
            if (!first) {
                typePattern << "|";
            }
            typePattern << t;
            first = false;
        }
        typePattern << ")\\b";
        
        SyntaxHighlightingRule typeRule;
        typeRule.pattern = typePattern.str();
        typeRule.color = typeColor;
        typeRule.isRegex = true;
        typeRule.caseSensitive = false;
        rules.push_back(typeRule);
    }
    
    // Built-in functions/constants
    if (!builtins.empty()) {
        std::ostringstream builtinPattern;
        builtinPattern << "\\b(";
        bool first = true;
        for (const auto& b : builtins) {
            if (!first) {
                builtinPattern << "|";
            }
            builtinPattern << b << "\\s*\\(";
            first = false;
        }
        builtinPattern << ")";
        
        SyntaxHighlightingRule builtinRule;
        builtinRule.pattern = builtinPattern.str();
        builtinRule.color = functionColor;
        builtinRule.isRegex = true;
        builtinRule.caseSensitive = false;
        rules.push_back(builtinRule);
    }
}

// Singleton instance
static SyntaxHighlighter* g_instance = nullptr;

SyntaxHighlighter::SyntaxHighlighter() {
    initialize();
}

SyntaxHighlighter& SyntaxHighlighter::getInstance() {
    if (!g_instance) {
        g_instance = new SyntaxHighlighter();
    }
    return *g_instance;
}

void SyntaxHighlighter::initialize() {
    // Initialize built-in languages
    initCPP();
    initPython();
    initJavaScript();
    initJava();
    initCSharp();
    initGo();
    initRust();
    initRuby();
    initPHP();
    initSwift();
    initKotlin();
    initShell();
    initJSON();
    initXML();
    initCSS();
    initMarkdown();
    initYAML();
    initTOML();
    initINI();
    initSQL();
}

const SyntaxLanguage* SyntaxHighlighter::getLanguageByExtension(const std::string& ext) const {
    std::string extLower = ext;
    if (!extLower.empty() && extLower[0] == '.') {
        extLower = extLower.substr(1);
    }
    
    for (const auto& lang : languages_) {
        if (lang.supportsExtension(extLower)) {
            return &lang;
        }
    }
    
    return nullptr;
}

const SyntaxLanguage* SyntaxHighlighter::getLanguageByFileName(const std::string& fileName) const {
    for (const auto& lang : languages_) {
        if (lang.supportsFileName(fileName)) {
            return &lang;
        }
    }
    return nullptr;
}

const SyntaxLanguage* SyntaxHighlighter::getLanguageByName(const std::string& name) const {
    for (const auto& lang : languages_) {
        if (lang.name == name) {
            return &lang;
        }
    }
    return nullptr;
}

void SyntaxHighlighter::addLanguage(const SyntaxLanguage& language) {
    // Check if language with this name already exists
    for (auto& lang : languages_) {
        if (lang.name == language.name) {
            lang = language; // Replace existing
            return;
        }
    }
    
    // Add new language
    languages_.push_back(language);
}

void SyntaxHighlighter::highlightLine(const std::string& line, 
                                     const SyntaxLanguage* language,
                                     std::vector<std::pair<std::string, ImVec4>>& outTokens) const {
    if (!language) {
        // No language specified, use default color
        outTokens.emplace_back(line, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        return;
    }
    
    // Simple tokenization - split on whitespace and punctuation
    std::string currentToken;
    ImVec4 currentColor = language->defaultColor;
    
    auto addToken = [&]() {
        if (!currentToken.empty()) {
            outTokens.emplace_back(currentToken, currentColor);
            currentToken.clear();
        }
    };
    
    for (size_t i = 0; i < line.length(); ) {
        char c = line[i];
        
        // Handle string literals
        if (c == '\"') {
            addToken();
            size_t end = i + 1;
            bool escaped = false;
            
            while (end < line.length()) {
                if (line[end] == '\\' && !escaped) {
                    escaped = true;
                } else if (line[end] == '\"' && !escaped) {
                    end++;
                    break;
                } else {
                    escaped = false;
                }
                end++;
            }
            
            outTokens.emplace_back(line.substr(i, end - i), language->stringColor);
            i = end;
            continue;
        }
        
        // Handle character literals
        if (c == '\'') {
            addToken();
            size_t end = i + 1;
            bool escaped = false;
            
            while (end < line.length()) {
                if (line[end] == '\\' && !escaped) {
                    escaped = true;
                } else if (line[end] == '\'' && !escaped) {
                    end++;
                    break;
                } else {
                    escaped = false;
                }
                end++;
            }
            
            outTokens.emplace_back(line.substr(i, end - i), language->stringColor);
            i = end;
            continue;
        }
        
        // Handle comments
        if (!language->lineComment.empty() && 
            line.compare(i, language->lineComment.length(), language->lineComment) == 0) {
            addToken();
            outTokens.emplace_back(line.substr(i), language->commentColor);
            return; // Rest of the line is a comment
        }
        
        // Simple tokenization by whitespace and punctuation
        if (isWhitespace(c)) {
            addToken();
            outTokens.emplace_back(std::string(1, c), language->defaultColor);
            i++;
        } else if (ispunct(static_cast<unsigned char>(c))) {
            addToken();
            outTokens.emplace_back(std::string(1, c), language->defaultColor);
            i++;
        } else {
            currentToken += c;
            currentColor = getTokenColor(currentToken, language);
            i++;
        }
    }
    
    // Add any remaining token
    addToken();
}

bool SyntaxHighlighter::isInStringLiteral(const std::string& line, size_t pos, 
                                         const SyntaxLanguage* language) const {
    if (!language) return false;
    
    bool inString = false;
    bool escaped = false;
    bool inChar = false;
    
    for (size_t i = 0; i < line.length() && i <= pos; i++) {
        char c = line[i];
        
        if (inString) {
            if (c == '\\' && !escaped) {
                escaped = true;
            } else if (c == '\"' && !escaped) {
                inString = false;
            } else {
                escaped = false;
            }
        } else if (inChar) {
            if (c == '\\' && !escaped) {
                escaped = true;
            } else if (c == '\'' && !escaped) {
                inChar = false;
            } else {
                escaped = false;
            }
        } else {
            if (c == '\"') {
                inString = true;
            } else if (c == '\'') {
                inChar = true;
            }
        }
    }
    
    return inString || inChar;
}

bool SyntaxHighlighter::isInComment(const std::string& line, size_t pos,
                                  const SyntaxLanguage* language) const {
    if (!language || language->lineComment.empty()) {
        return false;
    }
    
    size_t commentPos = line.find(language->lineComment);
    if (commentPos == std::string::npos) {
        return false;
    }
    
    return pos >= commentPos;
}

ImVec4 SyntaxHighlighter::getTokenColor(const std::string& token, const SyntaxLanguage* language) const {
    if (!language) {
        return ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    }
    
    // Check against language rules
    for (const auto& rule : language->rules) {
        if (rule.matches(token)) {
            return rule.color;
        }
    }
    
    // Default color
    return language->defaultColor;
}

// Language definitions

void SyntaxHighlighter::initCPP() {
    SyntaxLanguage lang;
    lang.name = "C++";
    lang.addExtension("cpp");
    lang.addExtension("cc");
    lang.addExtension("cxx");
    lang.addExtension("hpp");
    lang.addExtension("h");
    lang.addExtension("hxx");
    lang.addExtension("h++");
    lang.addExtension("inl");
    lang.addExtension("ipp");
    
    lang.lineComment = "//";
    lang.blockCommentStart = "/*";
    lang.blockCommentEnd = "*/";
    
    // C++ keywords
    lang.addKeywords({
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
        "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
        "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
        "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
        "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
        "public", "register", "reinterpret_cast", "requires", "return", "short",
        "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
        "switch", "template", "this", "thread_local", "throw", "true", "try",
        "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
        "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
    });
    
    // C++ types
    lang.addTypes({
        "bool", "char", "char8_t", "char16_t", "char32_t", "double", "float",
        "int", "long", "short", "signed", "unsigned", "void", "wchar_t",
        "size_t", "ssize_t", "ptrdiff_t", "int8_t", "int16_t", "int32_t",
        "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t",
        "uintptr_t", "intmax_t", "uintmax_t", "int_fast8_t", "int_fast16_t",
        "int_fast32_t", "int_fast64_t", "uint_fast8_t", "uint_fast16_t",
        "uint_fast32_t", "uint_fast64_t", "int_least8_t", "int_least16_t",
        "int_least32_t", "int_least64_t", "uint_least8_t", "uint_least16_t",
        "uint_least32_t", "uint_least64_t"
    });
    
    // C++ standard library types and functions
    lang.addBuiltins({
        "std::", "std::string", "std::vector", "std::map", "std::unordered_map",
        "std::set", "std::unordered_set", "std::list", "std::deque", "std::array",
        "std::pair", "std::tuple", "std::make_pair", "std::make_tuple", "std::get",
        "std::move", "std::forward", "std::unique_ptr", "std::shared_ptr",
        "std::weak_ptr", "std::make_unique", "std::make_shared", "std::function",
        "std::bind", "std::ref", "std::cref", "std::thread", "std::mutex",
        "std::lock_guard", "std::unique_lock", "std::condition_variable",
        "std::future", "std::promise", "std::async", "std::launch",
        "std::chrono::", "std::this_thread::"
    });
    
    lang.initialize();
    addLanguage(lang);
}

// Implement other language initializations (Python, JavaScript, etc.) in a similar way
void SyntaxHighlighter::initPython() { /* ... */ }
void SyntaxHighlighter::initJavaScript() { /* ... */ }
void SyntaxHighlighter::initJava() { /* ... */ }
void SyntaxHighlighter::initCSharp() { /* ... */ }
void SyntaxHighlighter::initGo() { /* ... */ }
void SyntaxHighlighter::initRust() { /* ... */ }
void SyntaxHighlighter::initRuby() { /* ... */ }
void SyntaxHighlighter::initPHP() { /* ... */ }
void SyntaxHighlighter::initSwift() { /* ... */ }
void SyntaxHighlighter::initKotlin() { /* ... */ }
void SyntaxHighlighter::initShell() { /* ... */ }
void SyntaxHighlighter::initJSON() { /* ... */ }
void SyntaxHighlighter::initXML() { /* ... */ }
void SyntaxHighlighter::initCSS() { /* ... */ }
void SyntaxHighlighter::initMarkdown() { /* ... */ }
void SyntaxHighlighter::initYAML() { /* ... */ }
void SyntaxHighlighter::initTOML() { /* ... */ }
void SyntaxHighlighter::initINI() { /* ... */ }
void SyntaxHighlighter::initSQL() { /* ... */ }

} // namespace ai_editor
