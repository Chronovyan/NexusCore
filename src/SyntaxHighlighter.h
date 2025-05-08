#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <regex>
#include <algorithm>

// Forward declaration
class TextBuffer;

// Define color codes for syntax highlighting
enum class SyntaxColor {
    Default,
    Keyword,
    Type,
    String,
    Comment,
    Number,
    Identifier,
    Preprocessor,
    Operator,
    Function
};

// Structure to hold styling information for a range of text
struct SyntaxStyle {
    size_t startCol;
    size_t endCol;
    SyntaxColor color;
    
    SyntaxStyle(size_t start, size_t end, SyntaxColor c)
        : startCol(start), endCol(end), color(c) {}
};

// Base class for all syntax highlighters
class SyntaxHighlighter {
public:
    virtual ~SyntaxHighlighter() = default;
    
    // Highlight a line of text
    virtual std::vector<SyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) = 0;
    
    // Highlight a full buffer
    virtual std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer& buffer) = 0;
    
    // Get the file extensions this highlighter supports
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    
    // Get human-readable name of the language
    virtual std::string getLanguageName() const = 0;
};

// Pattern-based syntax highlighter that uses regex patterns for highlighting
class PatternBasedHighlighter : public SyntaxHighlighter {
public:
    PatternBasedHighlighter(const std::string& name) : languageName_(name) {}
    
    std::vector<SyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) override {
        std::vector<SyntaxStyle> styles;
        
        // Apply each pattern to the line
        for (const auto& pattern : patterns_) {
            const auto& regex = pattern.first;
            const SyntaxColor color = pattern.second;
            
            std::sregex_iterator it(line.begin(), line.end(), regex);
            std::sregex_iterator end;
            
            while (it != end) {
                const std::smatch& match = *it;
                if (match.size() > 0) {
                    // Add a style for the match
                    size_t startCol = match.position();
                    size_t endCol = startCol + match.length();
                    
                    // Only add if this match adds value - could be more sophisticated
                    styles.push_back(SyntaxStyle(startCol, endCol, color));
                }
                ++it;
            }
        }
        
        // Sort styles by start position for rendering
        std::sort(styles.begin(), styles.end(), 
                 [](const SyntaxStyle& a, const SyntaxStyle& b) {
                     return a.startCol < b.startCol;
                 });
        
        return styles;
    }
    
    // Declaration only - implementation moved to .cpp file
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer& buffer) override;
    
    std::vector<std::string> getSupportedExtensions() const override {
        return supportedExtensions_;
    }
    
    std::string getLanguageName() const override {
        return languageName_;
    }
    
protected:
    // Add a pattern with its associated color
    void addPattern(const std::string& patternStr, SyntaxColor color) {
        patterns_.push_back(std::make_pair(std::regex(patternStr), color));
    }
    
    void addSupportedExtension(const std::string& ext) {
        supportedExtensions_.push_back(ext);
    }
    
private:
    std::vector<std::pair<std::regex, SyntaxColor>> patterns_;
    std::vector<std::string> supportedExtensions_;
    std::string languageName_;
};

// C++ Syntax Highlighter
class CppHighlighter : public PatternBasedHighlighter {
public:
    CppHighlighter() : PatternBasedHighlighter("C++") {
        // Add supported file extensions
        addSupportedExtension("cpp");
        addSupportedExtension("h");
        addSupportedExtension("hpp");
        addSupportedExtension("cc");
        
        // Keywords
        addPattern("\\b(auto|break|case|catch|class|const|constexpr|continue|default|"
                   "delete|do|else|enum|explicit|export|extern|for|friend|goto|if|"
                   "inline|mutable|namespace|new|noexcept|operator|private|protected|"
                   "public|register|return|sizeof|static|struct|switch|template|this|"
                   "throw|try|typedef|typeid|typename|union|using|virtual|volatile|while)\\b",
                   SyntaxColor::Keyword);
        
        // Types
        addPattern("\\b(bool|char|double|float|int|long|short|signed|unsigned|void|wchar_t|std::string|string)\\b",
                   SyntaxColor::Type);
        
        // Preprocessor directives
        addPattern("#(include|define|ifdef|ifndef|endif|else|error|pragma|if|line|undef)",
                   SyntaxColor::Preprocessor);
        
        // Numbers
        addPattern("\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?[fFlL]?\\b|"
                   "\\b0x[0-9a-fA-F]+[uUlL]*\\b|"
                   "\\b0[0-7]+[uUlL]*\\b",
                   SyntaxColor::Number);
        
        // Strings
        addPattern("\"([^\"\\\\]|\\\\.)*\"|'([^'\\\\]|\\\\.)*'",
                   SyntaxColor::String);
        
        // Comments
        addPattern("//.*$|/\\*[\\s\\S]*?\\*/",
                   SyntaxColor::Comment);
        
        // Functions
        addPattern("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()",
                   SyntaxColor::Function);
    }
};

// Registry for syntax highlighters
class SyntaxHighlighterRegistry {
public:
    static SyntaxHighlighterRegistry& getInstance() {
        static SyntaxHighlighterRegistry instance;
        return instance;
    }
    
    void registerHighlighter(std::unique_ptr<SyntaxHighlighter> highlighter) {
        // Add the highlighter's supported extensions to our extension map
        for (const auto& ext : highlighter->getSupportedExtensions()) {
            extensionMap_[ext] = highlighters_.size();
        }
        
        // Store the highlighter
        highlighters_.push_back(std::move(highlighter));
    }
    
    SyntaxHighlighter* getHighlighterForExtension(const std::string& extension) {
        // Extract extension from path if needed
        std::string ext = extension;
        size_t pos = extension.find_last_of('.');
        if (pos != std::string::npos) {
            ext = extension.substr(pos + 1);
        }
        
        // Convert to lowercase for comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        
        // Find the highlighter
        auto it = extensionMap_.find(ext);
        if (it != extensionMap_.end() && it->second < highlighters_.size()) {
            return highlighters_[it->second].get();
        }
        
        return nullptr; // No highlighter found
    }
    
private:
    SyntaxHighlighterRegistry() {
        // Register built-in highlighters
        registerHighlighter(std::make_unique<CppHighlighter>());
    }
    
    ~SyntaxHighlighterRegistry() = default;
    
    // Delete copy/move constructors and operators
    SyntaxHighlighterRegistry(const SyntaxHighlighterRegistry&) = delete;
    SyntaxHighlighterRegistry& operator=(const SyntaxHighlighterRegistry&) = delete;
    SyntaxHighlighterRegistry(SyntaxHighlighterRegistry&&) = delete;
    SyntaxHighlighterRegistry& operator=(SyntaxHighlighterRegistry&&) = delete;
    
    std::vector<std::unique_ptr<SyntaxHighlighter>> highlighters_;
    std::map<std::string, size_t> extensionMap_;
};

#endif // SYNTAX_HIGHLIGHTER_H 