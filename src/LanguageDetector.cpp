#include "LanguageDetector.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace ai_editor {

LanguageDetector::LanguageDetector() {
    // Initialize with common file extensions to ignore
    ignoreExtensions_ = {
        "exe", "dll", "so", "dylib", "obj", "o", "a", "lib",
        "png", "jpg", "jpeg", "gif", "bmp", "ico", "svg", "webp",
        "mp3", "mp4", "wav", "ogg", "avi", "mov", "mkv", "flac",
        "zip", "tar", "gz", "bz2", "xz", "7z", "rar",
        "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "bin", "dat", "db", "sqlite", "sqlite3"
    };
    
    // Initialize with common ignore patterns
    std::vector<std::string> defaultPatterns = {
        "^\\.git/",              // Git directory
        "^\\.svn/",              // SVN directory
        "^\\.hg/",               // Mercurial directory
        "^node_modules/",        // Node.js modules
        "^vendor/",              // Vendor directories
        "^build/",               // Build directories
        "^dist/",                // Distribution directories
        "^out/",                 // Output directories
        "^bin/",                 // Binary directories
        "^obj/",                 // Object directories
        "^target/",              // Target directories (Maven/Cargo)
        "^coverage/",            // Coverage reports
        "^__pycache__/",         // Python cache
        "^\\.vscode/",           // VS Code settings
        "^\\.idea/",             // IntelliJ IDEA settings
        "^\\.vs/",               // Visual Studio settings
        ".*\\.min\\.js$",        // Minified JavaScript
        ".*\\.min\\.css$",       // Minified CSS
        ".*\\.d\\.ts$",          // TypeScript declaration files
        ".*\\.generated\\.",     // Generated files
        ".*\\.Designer\\.",      // Designer files
        ".*~$",                  // Backup files
        ".*\\.bak$",             // Backup files
        ".*\\.swp$",             // Vim swap files
        ".*\\.swo$",             // Vim swap files
        ".*\\.DS_Store$"         // macOS metadata
    };
    
    addIgnorePatterns(defaultPatterns);
    
    // Initialize with default language definitions
    initializeDefaultLanguages();
}

std::optional<LanguageInfo> LanguageDetector::detectLanguageFromPath(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we should ignore this file
    if (shouldIgnoreFile(filePath)) {
        return std::nullopt;
    }
    
    // Extract the extension and check if it's associated with a language
    fs::path path(filePath);
    std::string filename = path.filename().string();
    std::string extension = path.extension().string();
    
    // Remove the dot from the extension
    if (!extension.empty() && extension[0] == '.') {
        extension = extension.substr(1);
    }
    
    // Convert extension to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check if this is a special filename (like "Makefile")
    auto filenameIt = filenameToLanguage_.find(filename);
    if (filenameIt != filenameToLanguage_.end()) {
        auto languageIt = languages_.find(filenameIt->second);
        if (languageIt != languages_.end()) {
            return languageIt->second;
        }
    }
    
    // Check if the extension is associated with a language
    auto extensionIt = extensionToLanguage_.find(extension);
    if (extensionIt != extensionToLanguage_.end()) {
        auto languageIt = languages_.find(extensionIt->second);
        if (languageIt != languages_.end()) {
            return languageIt->second;
        }
    }
    
    return std::nullopt;
}

std::optional<LanguageInfo> LanguageDetector::detectLanguageFromContent(
    const std::string& fileContent,
    const std::optional<std::string>& filePath) const {
    
    // First try to detect from the path if available
    if (filePath) {
        auto languageFromPath = detectLanguageFromPath(*filePath);
        if (languageFromPath) {
            return languageFromPath;
        }
    }
    
    // If the file is empty, we can't detect the language from content
    if (fileContent.empty()) {
        return std::nullopt;
    }
    
    // Check for shebang in the first line (for script languages)
    std::istringstream iss(fileContent);
    std::string firstLine;
    std::getline(iss, firstLine);
    
    auto languageFromShebang = detectLanguageFromShebang(firstLine);
    if (languageFromShebang) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto languageIt = languages_.find(*languageFromShebang);
        if (languageIt != languages_.end()) {
            return languageIt->second;
        }
    }
    
    // Try to detect based on content heuristics
    auto languageFromHeuristics = detectLanguageFromHeuristics(fileContent);
    if (languageFromHeuristics) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto languageIt = languages_.find(*languageFromHeuristics);
        if (languageIt != languages_.end()) {
            return languageIt->second;
        }
    }
    
    return std::nullopt;
}

std::optional<LanguageInfo> LanguageDetector::getLanguageInfo(const std::string& languageId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = languages_.find(languageId);
    if (it != languages_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<LanguageInfo> LanguageDetector::getAllLanguages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LanguageInfo> result;
    result.reserve(languages_.size());
    
    for (const auto& [_, info] : languages_) {
        result.push_back(info);
    }
    
    return result;
}

bool LanguageDetector::registerLanguage(const LanguageInfo& languageInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a language with this ID already exists
    if (languages_.find(languageInfo.id) != languages_.end()) {
        EditorErrorReporter::reportError(
            "LanguageDetector",
            "Language with ID '" + languageInfo.id + "' already exists",
            "Use a different ID for the new language"
        );
        return false;
    }
    
    // Add the language
    languages_[languageInfo.id] = languageInfo;
    
    // Add file extension associations
    for (const auto& ext : languageInfo.extensions) {
        std::string lowerExt = ext;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        extensionToLanguage_[lowerExt] = languageInfo.id;
    }
    
    // Add special filename associations
    for (const auto& filename : languageInfo.filenames) {
        filenameToLanguage_[filename] = languageInfo.id;
    }
    
    return true;
}

bool LanguageDetector::shouldIgnoreFile(const std::string& filePath) const {
    // Check if the file should be ignored based on its extension
    fs::path path(filePath);
    std::string extension = path.extension().string();
    
    // Remove the dot from the extension
    if (!extension.empty() && extension[0] == '.') {
        extension = extension.substr(1);
    }
    
    // Convert extension to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check if the extension is in the ignore list
    if (ignoreExtensions_.find(extension) != ignoreExtensions_.end()) {
        return true;
    }
    
    // Check if the file matches any ignore patterns
    for (const auto& pattern : ignorePatterns_) {
        if (std::regex_search(filePath, pattern)) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> LanguageDetector::getFileExtensions(const std::string& languageId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = languages_.find(languageId);
    if (it != languages_.end()) {
        return it->second.extensions;
    }
    
    return {};
}

std::optional<std::string> LanguageDetector::getLanguageIdForExtension(const std::string& extension) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    auto it = extensionToLanguage_.find(lowerExt);
    if (it != extensionToLanguage_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void LanguageDetector::addIgnorePatterns(const std::vector<std::string>& patterns) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& pattern : patterns) {
        try {
            ignorePatterns_.push_back(std::regex(pattern, std::regex::ECMAScript));
        } catch (const std::regex_error& e) {
            EditorErrorReporter::reportError(
                "LanguageDetector",
                "Invalid regex pattern: " + pattern,
                "Error: " + std::string(e.what())
            );
        }
    }
}

std::optional<std::string> LanguageDetector::detectLanguageFromShebang(const std::string& firstLine) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the line starts with "#!"
    if (firstLine.size() >= 2 && firstLine[0] == '#' && firstLine[1] == '!') {
        // Extract the interpreter path
        size_t start = 2;
        while (start < firstLine.size() && std::isspace(firstLine[start])) {
            start++;
        }
        
        size_t end = firstLine.find(' ', start);
        if (end == std::string::npos) {
            end = firstLine.size();
        }
        
        std::string interpreter = firstLine.substr(start, end - start);
        
        // Extract the base name of the interpreter
        size_t lastSlash = interpreter.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            interpreter = interpreter.substr(lastSlash + 1);
        }
        
        // Check if this interpreter is associated with a language
        for (const auto& [pattern, languageId] : shebangPatterns_) {
            if (std::regex_match(interpreter, std::regex(pattern))) {
                return languageId;
            }
        }
    }
    
    return std::nullopt;
}

std::optional<std::string> LanguageDetector::detectLanguageFromHeuristics(const std::string& fileContent) const {
    // This is a simple implementation that could be expanded with more sophisticated heuristics
    
    // Check for C++ patterns
    static const std::regex cppPatterns[] = {
        std::regex("#include\\s+<[^>]+>"),
        std::regex("using\\s+namespace\\s+\\w+;"),
        std::regex("class\\s+\\w+\\s*:\\s*public"),
        std::regex("std::\\w+")
    };
    
    for (const auto& pattern : cppPatterns) {
        if (std::regex_search(fileContent, pattern)) {
            return "cpp";
        }
    }
    
    // Check for Java patterns
    static const std::regex javaPatterns[] = {
        std::regex("public\\s+class\\s+\\w+"),
        std::regex("import\\s+java\\.\\w+"),
        std::regex("public\\s+static\\s+void\\s+main"),
        std::regex("@Override")
    };
    
    for (const auto& pattern : javaPatterns) {
        if (std::regex_search(fileContent, pattern)) {
            return "java";
        }
    }
    
    // Check for Python patterns
    static const std::regex pythonPatterns[] = {
        std::regex("import\\s+\\w+"),
        std::regex("from\\s+\\w+\\s+import"),
        std::regex("def\\s+\\w+\\(.*\\)\\s*:"),
        std::regex("class\\s+\\w+\\s*:")
    };
    
    for (const auto& pattern : pythonPatterns) {
        if (std::regex_search(fileContent, pattern)) {
            return "python";
        }
    }
    
    // Check for JavaScript patterns
    static const std::regex jsPatterns[] = {
        std::regex("function\\s+\\w+\\s*\\("),
        std::regex("const\\s+\\w+\\s*="),
        std::regex("let\\s+\\w+\\s*="),
        std::regex("var\\s+\\w+\\s*="),
        std::regex("document\\.getElementById")
    };
    
    for (const auto& pattern : jsPatterns) {
        if (std::regex_search(fileContent, pattern)) {
            return "javascript";
        }
    }
    
    // No clear pattern detected
    return std::nullopt;
}

void LanguageDetector::initializeDefaultLanguages() {
    // C++
    LanguageInfo cpp;
    cpp.id = "cpp";
    cpp.name = "C++";
    cpp.extensions = {"cpp", "cc", "cxx", "c++", "hpp", "hh", "hxx", "h", "h++", "ipp"};
    cpp.filenames = {};
    cpp.lineCommentPrefix = "//";
    cpp.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(cpp);
    
    // C
    LanguageInfo c;
    c.id = "c";
    c.name = "C";
    c.extensions = {"c", "h"};
    c.filenames = {};
    c.lineCommentPrefix = "//";
    c.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(c);
    
    // Java
    LanguageInfo java;
    java.id = "java";
    java.name = "Java";
    java.extensions = {"java"};
    java.filenames = {};
    java.lineCommentPrefix = "//";
    java.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(java);
    
    // Python
    LanguageInfo python;
    python.id = "python";
    python.name = "Python";
    python.extensions = {"py", "pyw", "pyi", "pyx"};
    python.filenames = {};
    python.lineCommentPrefix = "#";
    python.blockCommentDelimiters = {"'''", "'''", "\"\"\"", "\"\"\""};
    registerLanguage(python);
    
    // JavaScript
    LanguageInfo javascript;
    javascript.id = "javascript";
    javascript.name = "JavaScript";
    javascript.extensions = {"js", "mjs", "cjs"};
    javascript.filenames = {};
    javascript.lineCommentPrefix = "//";
    javascript.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(javascript);
    
    // TypeScript
    LanguageInfo typescript;
    typescript.id = "typescript";
    typescript.name = "TypeScript";
    typescript.extensions = {"ts", "tsx"};
    typescript.filenames = {};
    typescript.lineCommentPrefix = "//";
    typescript.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(typescript);
    
    // HTML
    LanguageInfo html;
    html.id = "html";
    html.name = "HTML";
    html.extensions = {"html", "htm", "xhtml"};
    html.filenames = {};
    html.lineCommentPrefix = "";
    html.blockCommentDelimiters = {"<!--", "-->"};
    registerLanguage(html);
    
    // CSS
    LanguageInfo css;
    css.id = "css";
    css.name = "CSS";
    css.extensions = {"css"};
    css.filenames = {};
    css.lineCommentPrefix = "";
    css.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(css);
    
    // JSON
    LanguageInfo json;
    json.id = "json";
    json.name = "JSON";
    json.extensions = {"json"};
    json.filenames = {};
    json.lineCommentPrefix = "";
    json.blockCommentDelimiters = {};
    registerLanguage(json);
    
    // XML
    LanguageInfo xml;
    xml.id = "xml";
    xml.name = "XML";
    xml.extensions = {"xml", "xsd", "xsl", "xslt", "svg"};
    xml.filenames = {};
    xml.lineCommentPrefix = "";
    xml.blockCommentDelimiters = {"<!--", "-->"};
    registerLanguage(xml);
    
    // Markdown
    LanguageInfo markdown;
    markdown.id = "markdown";
    markdown.name = "Markdown";
    markdown.extensions = {"md", "markdown"};
    markdown.filenames = {"README", "README.md"};
    markdown.lineCommentPrefix = "";
    markdown.blockCommentDelimiters = {};
    registerLanguage(markdown);
    
    // Bash
    LanguageInfo bash;
    bash.id = "bash";
    bash.name = "Bash";
    bash.extensions = {"sh", "bash"};
    bash.filenames = {".bashrc", ".bash_profile", ".profile"};
    bash.lineCommentPrefix = "#";
    bash.blockCommentDelimiters = {};
    registerLanguage(bash);
    
    // Ruby
    LanguageInfo ruby;
    ruby.id = "ruby";
    ruby.name = "Ruby";
    ruby.extensions = {"rb"};
    ruby.filenames = {"Rakefile", "Gemfile"};
    ruby.lineCommentPrefix = "#";
    ruby.blockCommentDelimiters = {"=begin", "=end"};
    registerLanguage(ruby);
    
    // PHP
    LanguageInfo php;
    php.id = "php";
    php.name = "PHP";
    php.extensions = {"php", "php3", "php4", "php5", "phtml"};
    php.filenames = {};
    php.lineCommentPrefix = "//";
    php.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(php);
    
    // SQL
    LanguageInfo sql;
    sql.id = "sql";
    sql.name = "SQL";
    sql.extensions = {"sql"};
    sql.filenames = {};
    sql.lineCommentPrefix = "--";
    sql.blockCommentDelimiters = {"/*", "*/"};
    registerLanguage(sql);
    
    // Set up shebang patterns
    shebangPatterns_["python[23]?"] = "python";
    shebangPatterns_["ruby"] = "ruby";
    shebangPatterns_["bash"] = "bash";
    shebangPatterns_["sh"] = "bash";
    shebangPatterns_["zsh"] = "bash";
    shebangPatterns_["node"] = "javascript";
    shebangPatterns_["nodejs"] = "javascript";
    shebangPatterns_["php"] = "php";
}

} // namespace ai_editor 