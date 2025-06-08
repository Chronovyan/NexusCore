#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <string>
#include <vector>
#include <map>
#include <memory> // For std::unique_ptr
#include <regex>
#include <algorithm>
#include <iostream>
#include <atomic> // For memory barriers
#include <sstream>
#include "EditorError.h" // Needed for ErrorReporter and EditorException
#include "ThreadSafetyConfig.h" // Import thread safety configuration

// Forward declaration
class TextBuffer;
class ITextBuffer;

// Define color codes for syntax highlighting
enum class SyntaxColor {
    Default,        // 0
    Keyword,        // 1
    Type,           // 2
    String,         // 3 (High priority for stateful)
    Comment,        // 4 (High priority for stateful)
    Number,         // 5
    Function,       // 6 (Higher priority than Identifier for patterns)
    Identifier,     // 7
    Preprocessor,   // 8
    Operator        // 9
};

// Enum to categorize highlight patterns for more detailed analysis or future features
enum class HighlightCategory {
    UNKNOWN,
    KEYWORD,
    TYPE_PRIMITIVE,
    TYPE_USER_DEFINED,
    LITERAL,
    PREPROCESSOR,
    IDENTIFIER,
    OPERATOR
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
    virtual std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string& line, size_t lineIndex) const = 0;
    
    // Highlight a full buffer
    virtual std::vector<std::vector<SyntaxStyle>> highlightBuffer(const ITextBuffer& buffer) const = 0;
    
    // Get the file extensions this highlighter supports
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    
    // Get human-readable name of the language
    virtual std::string getLanguageName() const = 0;
    
    // Debug logging methods
    static void setDebugLoggingEnabled(bool enabled);
    static bool isDebugLoggingEnabled();
    
protected:
    // Helper method to log debug information
    static void logDebug(const std::string& message);
    
private:
    // Static flag for debug logging
    static bool debugLoggingEnabled_;
};

// Define the NextTokenType enum for CppHighlighter state management
enum class NextTokenType {
    UNKNOWN,
    BLOCK_COMMENT,
    LINE_COMMENT,
    RAW_STRING,
    STRING,
    CHAR
};

// Pattern-based syntax highlighter that uses regex patterns for highlighting
class PatternBasedHighlighter : public SyntaxHighlighter {
public:
    PatternBasedHighlighter(const std::string& name) : languageName_(name) {
        logDebug("PatternBasedHighlighter Constructor for '" + name + "'");
    }
    
    virtual ~PatternBasedHighlighter() = default;
    
    // Main method to highlight a single line based on patterns
    virtual std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string& line, [[maybe_unused]] size_t lineIndex) const override {
        logDebug("PatternBasedHighlighter::highlightLine for '" + languageName_ + "' on line: \"" + 
                line.substr(0, std::min(size_t(50), line.length())) + 
                (line.length() > 50 ? "..." : "") + "\"");
        auto styles = std::make_unique<std::vector<SyntaxStyle>>();
        
        try {
            logDebug("PatternBasedHighlighter::highlightLine - Attempting read lock");
            READ_LOCK(patterns_mutex_);
            logDebug("PatternBasedHighlighter::highlightLine - Acquired read lock");
            
            // Keep track of which positions have already been styled
            std::vector<bool> positionStyled(line.length(), false);
            
            // Apply each pattern to the line
            for (const auto& pattern_pair : patterns_) {
                const auto& regex = pattern_pair.first;
                const SyntaxColor color = pattern_pair.second;
                
                std::sregex_iterator it(line.begin(), line.end(), regex);
                std::sregex_iterator end;
                
                while (it != end) {
                    const std::smatch& match = *it;
                    if (match.size() > 0) {
                        if (match.length(0) == 0) { // Prevent issues with zero-length matches
                            ++it;
                            continue;
                        }
                        size_t startCol = match.position();
                        size_t endCol = startCol + match.length();
                        
                        // Check if this range is already styled - only add if not
                        bool alreadyStyled = false;
                        for (size_t i = startCol; i < endCol && i < positionStyled.size(); ++i) {
                            if (positionStyled[i]) {
                                alreadyStyled = true;
                                break;
                            }
                        }
                        
                        if (!alreadyStyled) {
                            styles->push_back(SyntaxStyle(startCol, endCol, color));
                            logDebug("PBHL match '" + match.str(0) + "' -> (" + 
                                    std::to_string(startCol) + "," + std::to_string(endCol) + 
                                    ") C:" + std::to_string(static_cast<int>(color)));
                            
                            // Mark these positions as styled
                            for (size_t i = startCol; i < endCol && i < positionStyled.size(); ++i) {
                                positionStyled[i] = true;
                            }
                        }
                    }
                    ++it;
                }
            }
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(
                std::string("PatternBasedHighlighter::highlightLine: ") + ex.what(), 
                EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("PatternBasedHighlighter::highlightLine");
        }
        
        // Sort styles by start position for rendering 
        // (this shouldn't affect the precedence since we already filtered out overlaps)
        std::sort(styles->begin(), styles->end(), 
                  [](const SyntaxStyle& a, const SyntaxStyle& b) {
                      return a.startCol < b.startCol;
                  });
        
        return styles;
    }
    
    // Highlight a full buffer
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(const ITextBuffer& buffer) const override;
    
    std::vector<std::string> getSupportedExtensions() const override {
        READ_LOCK(patterns_mutex_);
        return supportedExtensions_;
    }
    
    std::string getLanguageName() const override {
        return languageName_;
    }
    
protected:
    // Add a pattern with its associated color and category
    void addPattern(const std::string& patternStr, SyntaxColor color, [[maybe_unused]] HighlightCategory category = HighlightCategory::UNKNOWN) {
        logDebug("PatternBasedHighlighter::addPattern for '" + languageName_ + "' with pattern: \"" + 
                patternStr.substr(0, std::min(size_t(50), patternStr.length())) + 
                (patternStr.length() > 50 ? "..." : "") + "\"");
        try {
            WRITE_LOCK(patterns_mutex_);
            patterns_.push_back(std::make_pair(std::regex(patternStr), color));
        } catch (const std::regex_error& regex_ex) {
            ErrorReporter::logError("PatternBasedHighlighter::addPattern - Regex error: " + 
                                    std::string(regex_ex.what()) + " for pattern: " + patternStr);
            ErrorReporter::logException(SyntaxHighlightingException(
                std::string("PatternBasedHighlighter::addPattern: Invalid regex '") + 
                patternStr + "': " + regex_ex.what(), 
                EditorException::Severity::EDITOR_ERROR));
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(
                std::string("PatternBasedHighlighter::addPattern: ") + ex.what(), 
                EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("PatternBasedHighlighter::addPattern");
        }
    }
    
    void addSupportedExtension(const std::string& ext) {
        WRITE_LOCK(patterns_mutex_);
        supportedExtensions_.push_back(ext);
    }
    
protected:
    mutable READER_WRITER_MUTEX patterns_mutex_; // For thread-safe access to patterns and extensions
    std::vector<std::pair<std::regex, SyntaxColor>> patterns_;
    std::vector<std::string> supportedExtensions_;

private:
    std::string languageName_;
};

// C++ Syntax Highlighter
class CppHighlighter : public PatternBasedHighlighter {
public:
    CppHighlighter() : PatternBasedHighlighter("C++") {
        logDebug("CppHighlighter Constructor - Start");
        addSupportedExtension("cpp");
        addSupportedExtension("h");
        addSupportedExtension("hpp");
        addSupportedExtension("cc");
        
        addPattern("\\b(if|else|for|while|do|switch|case|default|break|continue|return|goto|try|catch|throw|new|delete|operator|template|typename|this|friend|explicit|inline|virtual|static|const|constexpr|volatile|mutable|extern|auto|decltype|namespace|using|asm|typedef|sizeof|alignas|alignof|noexcept|static_assert|thread_local)\\b", SyntaxColor::Keyword, HighlightCategory::KEYWORD);
        addPattern("\\b(void|bool|char|char16_t|char32_t|wchar_t|short|int|long|float|double|signed|unsigned)\\b", SyntaxColor::Type, HighlightCategory::TYPE_PRIMITIVE);
        addPattern("\\b(class|struct|enum|union)\\b", SyntaxColor::Type, HighlightCategory::TYPE_USER_DEFINED);
        addPattern("\\b([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([uUlLfF]|[eE][-+]?[0-9]+)?\\b", SyntaxColor::Number, HighlightCategory::LITERAL);
        addPattern(R"(\'(?:[^\'\\]|\\.)*\')", SyntaxColor::String, HighlightCategory::LITERAL);
        addPattern(R"(\"(?:[^\"\\]|\\.)*\")", SyntaxColor::String, HighlightCategory::LITERAL);
        addPattern("\\b(true|false|nullptr)\\b", SyntaxColor::Keyword, HighlightCategory::LITERAL);
        addPattern("^\\s*#\\s*(define|include|if|ifdef|ifndef|else|elif|endif|pragma|line|error|warning)(?:\\b|\\s|$)", SyntaxColor::Preprocessor, HighlightCategory::PREPROCESSOR);
        addPattern("\\b([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()", SyntaxColor::Function, HighlightCategory::IDENTIFIER);
        addPattern("[a-zA-Z_][a-zA-Z0-9_]*", SyntaxColor::Identifier, HighlightCategory::IDENTIFIER);
        logDebug("CppHighlighter Constructor - End - Patterns Added: Count Details...");
    }

    std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string& line, size_t lineIndex) const override;
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(const ITextBuffer& buffer) const override;
    
    // Helper method to reset mutable state variables
    void mutable_reset() const;

    mutable bool isInBlockComment_ = false;
    mutable bool isInRawString_ = false;
    mutable std::string rawStringDelimiter_;
    mutable bool isInString_ = false;
    mutable bool isInChar_ = false;
    mutable size_t lastProcessedLineIndex_ = static_cast<size_t>(-1);
    mutable bool isInMacroContinuation_ = false;

private:
    void findNextStatefulToken(const std::string& segment, size_t& nextTokenPos, NextTokenType& tokenType) const;
    void appendBaseStyles(std::vector<SyntaxStyle>& existingStyles, const std::string& subLine, size_t offset) const;
};

// Registry for syntax highlighters
class SyntaxHighlighterRegistry {
public:
    static SyntaxHighlighterRegistry& getInstance() {
        static SyntaxHighlighterRegistry instance;
        std::atomic_thread_fence(std::memory_order_acquire);
        return instance;
    }
    
    void clearRegistry() {
        try {
            SCOPED_LOCK(registry_mutex_);
            highlighters_.clear();
            extensionMap_.clear();
            std::atomic_thread_fence(std::memory_order_release);
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::clearRegistry: ") + ex.what(), EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::clearRegistry");
        }
    }
    
    void registerHighlighter(std::unique_ptr<SyntaxHighlighter> highlighter) {
        if (!highlighter) return;
        try {
            SCOPED_LOCK(registry_mutex_);
            size_t highlighter_index = highlighters_.size();
            const auto extensions = highlighter->getSupportedExtensions();
            for (const auto& ext : extensions) {
                extensionMap_[ext] = highlighter_index;
            }
            highlighters_.push_back(std::move(highlighter));
            std::atomic_thread_fence(std::memory_order_release);
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::registerHighlighter: ") + ex.what(), EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::registerHighlighter");
        }
    }
    
    SyntaxHighlighter* getHighlighterForExtension(const std::string& extension) const {
        try {
            READ_LOCK(registry_mutex_);
            if (highlighters_.empty()) return nullptr;
            std::string ext = extension;
            size_t dot_pos = extension.find_last_of('.');
            if (dot_pos != std::string::npos) ext = extension.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); });
            auto it = extensionMap_.find(ext);
            if (it != extensionMap_.end() && it->second < highlighters_.size()) return highlighters_[it->second].get();
            return nullptr;
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::getHighlighterForExtension: ") + ex.what(), EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::getHighlighterForExtension");
        }
        return nullptr;
    }
    
    std::shared_ptr<SyntaxHighlighter> getSharedHighlighterForExtension(const std::string& extension) const {
        try {
            READ_LOCK(registry_mutex_);
            if (highlighters_.empty()) return nullptr;
            std::string ext = extension;
            size_t dot_pos = extension.find_last_of('.');
            if (dot_pos != std::string::npos) ext = extension.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); });
            auto it = extensionMap_.find(ext);
            if (it != extensionMap_.end() && it->second < highlighters_.size()) {
                return std::shared_ptr<SyntaxHighlighter>(highlighters_[it->second].get(), [](SyntaxHighlighter*) {});
            }
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::getSharedHighlighterForExtension: ") + ex.what(), EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::getSharedHighlighterForExtension");
        }
        return nullptr;
    }
    
private:
    SyntaxHighlighterRegistry() {
        try {
            registerHighlighter(std::make_unique<CppHighlighter>());
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry Constructor: ") + ex.what(), EditorException::Severity::EDITOR_ERROR));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry Constructor");
        }
    }
    
    ~SyntaxHighlighterRegistry() = default;
    SyntaxHighlighterRegistry(const SyntaxHighlighterRegistry&) = delete;
    SyntaxHighlighterRegistry& operator=(const SyntaxHighlighterRegistry&) = delete;
    SyntaxHighlighterRegistry(SyntaxHighlighterRegistry&&) = delete;
    SyntaxHighlighterRegistry& operator=(SyntaxHighlighterRegistry&&) = delete;
    
    mutable READER_WRITER_MUTEX registry_mutex_;
    std::vector<std::unique_ptr<SyntaxHighlighter>> highlighters_;
    std::map<std::string, size_t> extensionMap_;
};

#endif // SYNTAX_HIGHLIGHTER_H 