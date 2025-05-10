#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <regex>
#include <algorithm>
#include <mutex>
#include <iostream>
#include <shared_mutex> // For read-write lock
#include <atomic> // For memory barriers
#include <thread>
#include <sstream>
#include "EditorError.h" // Needed for ErrorReporter and EditorException

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
    virtual std::vector<SyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) const = 0;
    
    // Highlight a full buffer
    virtual std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer& buffer) const = 0;
    
    // Get the file extensions this highlighter supports
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    
    // Get human-readable name of the language
    virtual std::string getLanguageName() const = 0;
};

// Pattern-based syntax highlighter that uses regex patterns for highlighting
class PatternBasedHighlighter : public SyntaxHighlighter {
public:
    PatternBasedHighlighter(const std::string& name) : languageName_(name) {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter Constructor for '" << name << "'" << std::endl;
    }
    
    std::vector<SyntaxStyle> highlightLine(const std::string& line, [[maybe_unused]] size_t lineIndex) const override {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::highlightLine for '" << languageName_ << "' on line: \"" << line.substr(0, 50) << (line.length() > 50 ? "..." : "") << "\"" << std::endl;
        std::vector<SyntaxStyle> styles;
        
        try {
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::highlightLine - Attempting shared_lock" << std::endl;
            std::shared_lock<std::shared_mutex> lock(patterns_mutex_);
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::highlightLine - Acquired shared_lock" << std::endl;
            
            // Apply each pattern to the line
            for (const auto& pattern_pair : patterns_) {
                const auto& regex = pattern_pair.first;
                const SyntaxColor color = pattern_pair.second;
                
                std::sregex_iterator it(line.begin(), line.end(), regex);
                std::sregex_iterator end;
                
                while (it != end) {
                    const std::smatch& match = *it;
                    if (match.size() > 0) {
                        size_t startCol = match.position();
                        size_t endCol = startCol + match.length();
                        styles.push_back(SyntaxStyle(startCol, endCol, color));
                    }
                    ++it;
                }
            }
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(
                std::string("PatternBasedHighlighter::highlightLine: ") + ex.what(), 
                EditorException::Severity::Error));
        } catch (...) {
            ErrorReporter::logUnknownException("PatternBasedHighlighter::highlightLine");
        }
        
        // Sort styles by start position for rendering
        std::sort(styles.begin(), styles.end(), 
                 [](const SyntaxStyle& a, const SyntaxStyle& b) {
                     return a.startCol < b.startCol;
                 });
        
        return styles;
    }
    
    // Declaration only - implementation moved to .cpp file
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer& buffer) const override;
    
    std::vector<std::string> getSupportedExtensions() const override {
        std::shared_lock<std::shared_mutex> lock(patterns_mutex_);
        return supportedExtensions_;
    }
    
    std::string getLanguageName() const override {
        return languageName_;
    }
    
protected:
    // Add a pattern with its associated color
    void addPattern(const std::string& patternStr, SyntaxColor color) {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::addPattern for '" << languageName_ << "' with pattern: \"" << patternStr.substr(0, 50) << (patternStr.length() > 50 ? "..." : "") << "\"" << std::endl;
        try {
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::addPattern - Attempting unique_lock" << std::endl;
            std::unique_lock<std::shared_mutex> lock(patterns_mutex_);
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::addPattern - Acquired unique_lock" << std::endl;
            patterns_.push_back(std::make_pair(std::regex(patternStr), color));
        } catch (const std::regex_error& regex_ex) {
            std::cerr << "[DEBUG THREAD " << std::this_thread::get_id() << "] PatternBasedHighlighter::addPattern - Regex error: " << regex_ex.what() << " for pattern: " << patternStr << std::endl;
            ErrorReporter::logException(SyntaxHighlightingException(
                std::string("PatternBasedHighlighter::addPattern: Invalid regex '") + 
                patternStr + "': " + regex_ex.what(), 
                EditorException::Severity::Error));
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(
                std::string("PatternBasedHighlighter::addPattern: ") + ex.what(), 
                EditorException::Severity::Error));
        } catch (...) {
            ErrorReporter::logUnknownException("PatternBasedHighlighter::addPattern");
        }
    }
    
    void addSupportedExtension(const std::string& ext) {
        std::unique_lock<std::shared_mutex> lock(patterns_mutex_);
        supportedExtensions_.push_back(ext);
    }
    
private:
    mutable std::shared_mutex patterns_mutex_; // For thread-safe access to patterns and extensions
    std::vector<std::pair<std::regex, SyntaxColor>> patterns_;
    std::vector<std::string> supportedExtensions_;
    std::string languageName_;
};

// C++ Syntax Highlighter
class CppHighlighter : public PatternBasedHighlighter {
public:
    CppHighlighter() : PatternBasedHighlighter("C++") {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] CppHighlighter Constructor - Start" << std::endl;
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
        
        // Comments (split into two patterns)
        addPattern("//.*$", SyntaxColor::Comment); // Line comments
        addPattern("/\\*[\\s\\S]*?\\*/", SyntaxColor::Comment); // Block comments
        
        // Functions
        addPattern("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()",
                   SyntaxColor::Function);
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] CppHighlighter Constructor - End" << std::endl;
    }
};

// Registry for syntax highlighters
class SyntaxHighlighterRegistry {
public:
    // Thread-safe singleton access with memory ordering guarantees
    static SyntaxHighlighterRegistry& getInstance() {
        // Using Meyers Singleton pattern for thread-safety
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getInstance() - Called" << std::endl;
        static SyntaxHighlighterRegistry instance;
        std::atomic_thread_fence(std::memory_order_acquire); // Ensure visibility of instance
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getInstance() - Returning instance" << std::endl;
        return instance;
    }
    
    void registerHighlighter(std::unique_ptr<SyntaxHighlighter> highlighter) {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::registerHighlighter - Attempting for highlighter: " << (highlighter ? highlighter->getLanguageName() : "nullptr") << std::endl;
        if (!highlighter) {
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::registerHighlighter - Highlighter is null, returning." << std::endl;
            return; // Silently ignore null highlighters
        }
        
        try {
            // Use exclusive lock for writing
            std::scoped_lock lock(registry_mutex_);
            
            // Store the highlighter's extensions
            auto extensions = highlighter->getSupportedExtensions();
            size_t highlighter_index = highlighters_.size();
            
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::registerHighlighter - Registering " << extensions.size() << " extensions for '" << highlighter->getLanguageName() << "' at index " << highlighter_index << std::endl;
            
            // Add the highlighter's supported extensions to our extension map
            for (const auto& ext : extensions) {
                extensionMap_[ext] = highlighter_index;
            }
            
            // Store the highlighter
            highlighters_.push_back(std::move(highlighter));
            
            // Ensure changes are visible to other threads
            std::atomic_thread_fence(std::memory_order_release);
        } catch (const EditorException& ed_ex) { // Catch EditorException first
            ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::registerHighlighter: ") + ex.what(), EditorException::Severity::Error));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::registerHighlighter");
        }
    }
    
    // Get a raw pointer to a highlighter for the given file extension
    SyntaxHighlighter* getHighlighterForExtension(const std::string& extension) const {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - Called for extension: '" << extension << "'" << std::endl;
        try {
            // Use shared lock for reading (allows multiple concurrent readers)
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - Attempting shared_lock" << std::endl;
            std::shared_lock<std::shared_mutex> lock(registry_mutex_);
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - Acquired shared_lock" << std::endl;
            
            if (highlighters_.empty()) {
                std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - No highlighters registered." << std::endl;
                return nullptr;
            }
            
            // Extract extension from path if needed
            std::string ext = extension;
            size_t pos = extension.find_last_of('.');
            if (pos != std::string::npos) {
                ext = extension.substr(pos + 1);
            }
            
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), 
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - Processed extension: '" << ext << "'" << std::endl;
            // Find the highlighter
            auto it = extensionMap_.find(ext);
            if (it != extensionMap_.end() && it->second < highlighters_.size()) {
                std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - Found highlighter '" << highlighters_[it->second]->getLanguageName() << "' for extension '" << ext << "'" << std::endl;
                return highlighters_[it->second].get();
            }
            
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getHighlighterForExtension - No highlighter found for extension '" << ext << "'" << std::endl;
            return nullptr; // No highlighter found
        } catch (const EditorException& ed_ex) { // Catch EditorException first
            ErrorReporter::logException(ed_ex);
            return nullptr;
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::getHighlighterForExtension: ") + ex.what(), EditorException::Severity::Error));
            return nullptr;
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::getHighlighterForExtension");
            return nullptr;
        }
    }
    
    // Get a shared_ptr to a highlighter for the given file extension
    // This method creates a new shared_ptr that shares ownership with the registry
    std::shared_ptr<SyntaxHighlighter> getSharedHighlighterForExtension(const std::string& extension) const {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getSharedHighlighterForExtension - Called for extension: '" << extension << "'" << std::endl;
        try {
            // Use shared lock for reading
            std::shared_lock<std::shared_mutex> lock(registry_mutex_);
            
            if (highlighters_.empty()) {
                std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getSharedHighlighterForExtension - No highlighters registered." << std::endl;
                return nullptr;
            }
            
            // Extract extension from path if needed
            std::string ext = extension;
            size_t pos = extension.find_last_of('.');
            if (pos != std::string::npos) {
                ext = extension.substr(pos + 1);
            }
            
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), 
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            // Find the highlighter
            auto it = extensionMap_.find(ext);
            if (it != extensionMap_.end() && it->second < highlighters_.size()) {
                std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getSharedHighlighterForExtension - Found highlighter for extension '" << ext << "'" << std::endl;
                
                // Create a shared_ptr that shares ownership with the registry's unique_ptr
                // The aliasing constructor maintains a reference to the registry's unique_ptr
                // while providing a pointer to the SyntaxHighlighter
                return std::shared_ptr<SyntaxHighlighter>(
                    std::shared_ptr<void>{}, // Empty shared_ptr as owner
                    highlighters_[it->second].get() // Raw pointer to the highlighter
                );
            }
            
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry::getSharedHighlighterForExtension - No highlighter found for extension '" << ext << "'" << std::endl;
            return nullptr; // No highlighter found
        } catch (const EditorException& ed_ex) {
            ErrorReporter::logException(ed_ex);
            return nullptr;
        } catch (const std::exception& ex) {
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::getSharedHighlighterForExtension: ") + ex.what(), EditorException::Severity::Error));
            return nullptr;
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::getSharedHighlighterForExtension");
            return nullptr;
        }
    }
    
private:
    SyntaxHighlighterRegistry() {
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry Constructor - Start" << std::endl;
        try {
            // Add memory barrier before initialization
            std::atomic_thread_fence(std::memory_order_acquire);
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry Constructor - Registering built-in CppHighlighter" << std::endl;
            // Register built-in highlighters
            registerHighlighter(std::make_unique<CppHighlighter>());
            std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry Constructor - CppHighlighter registered." << std::endl;
            
            // Add memory barrier after initialization
            std::atomic_thread_fence(std::memory_order_release);
        } catch (const EditorException& ed_ex) { // Catch EditorException first
             ErrorReporter::logException(ed_ex);
        } catch (const std::exception& ex) {
            // Constructor errors are generally critical
            ErrorReporter::logException(SyntaxHighlightingException(std::string("SyntaxHighlighterRegistry::constructor: ") + ex.what(), EditorException::Severity::Critical));
        } catch (...) {
            ErrorReporter::logUnknownException("SyntaxHighlighterRegistry::constructor");
            std::cerr << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry Constructor - Unknown exception!" << std::endl;
        }
        std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] SyntaxHighlighterRegistry Constructor - End" << std::endl;
    }
    
    ~SyntaxHighlighterRegistry() = default;
    
    // Delete copy/move constructors and operators
    SyntaxHighlighterRegistry(const SyntaxHighlighterRegistry&) = delete;
    SyntaxHighlighterRegistry& operator=(const SyntaxHighlighterRegistry&) = delete;
    SyntaxHighlighterRegistry(SyntaxHighlighterRegistry&&) = delete;
    SyntaxHighlighterRegistry& operator=(SyntaxHighlighterRegistry&&) = delete;
    
    // Thread-safe registry access with read-write lock for better concurrency
    mutable std::shared_mutex registry_mutex_;
    std::vector<std::unique_ptr<SyntaxHighlighter>> highlighters_;
    std::map<std::string, size_t> extensionMap_;
};

#endif // SYNTAX_HIGHLIGHTER_H 