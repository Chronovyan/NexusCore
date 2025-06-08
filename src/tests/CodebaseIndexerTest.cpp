#include "../CodebaseIndexer.hpp"
#include "../CStyleLanguageParser.hpp"
#include "../LanguageDetector.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

using namespace ai_editor;

// Mock implementations for required dependencies
class MockWorkspaceManager : public IWorkspaceManager {
public:
    bool writeFile(const std::string& filename, const std::string& content) override {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        file << content;
        return true;
    }
    
    bool fileExists(const std::string& filename) const override { 
        return fs::exists(filename); 
    }
    
    std::vector<std::string> listFiles() const override {
        std::vector<std::string> files;
        for (const auto& dir : rootDirectories) {
            if (!fs::exists(dir) || !fs::is_directory(dir)) continue;
            
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().string());
                }
            }
        }
        return files;
    }
    
    std::string readFile(const std::string& filename) const override {
        if (!fileExists(filename)) return "";
        
        std::ifstream file(filename);
        if (!file.is_open()) return "";
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::string getWorkspacePath() const override {
        if (rootDirectories.empty()) return ".";
        return rootDirectories[0];
    }
    
    bool createDirectory(const std::string& dirname) override {
        try {
            return fs::create_directories(dirname);
        } catch (...) {
            return false;
        }
    }
    
    bool deleteFile(const std::string& filename) override {
        try {
            return fs::remove(filename);
        } catch (...) {
            return false;
        }
    }
    
    bool renameFile(const std::string& oldFilename, const std::string& newFilename) override {
        try {
            fs::rename(oldFilename, newFilename);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // Helper methods for the mock
    void addRootDirectory(const std::string& dir) {
        rootDirectories.push_back(dir);
    }
    
private:
    std::vector<std::string> rootDirectories;
};

class MockEditorCoreThreadPool : public EditorCoreThreadPool {
public:
    MockEditorCoreThreadPool() : EditorCoreThreadPool(1) {} // Create with 1 thread

    void start() override {
        // No-op for mock
    }
    
    void shutdown() override {
        // No-op for mock
    }
    
    std::thread::id assignTextBufferOwnership(std::shared_ptr<TextBuffer> buffer) override {
        return std::this_thread::get_id(); // Just return current thread id
    }
    
    bool isPoolThread() const override {
        return true; // Always consider current thread as part of pool
    }
    
    bool isTextBufferOwnerThread() const override {
        return true; // Always consider current thread as owner
    }
    
    void submitTask(std::function<void()> task) override {
        // Just run the task in the current thread for simplicity
        task();
    }
    
    size_t threadCount() const override {
        return 1; // Mock has 1 thread (current thread)
    }
    
    void notifyTextBufferOperationsAvailable() override {
        // No-op for mock
    }
};

// Helper functions for printing
void printSymbol(const CodeSymbol& symbol, int indentLevel = 0) {
    std::string indent(indentLevel * 2, ' ');
    std::cout << indent << "Symbol: " << symbol.name << " (";
    
    switch (symbol.type) {
        case CodeSymbol::SymbolType::FUNCTION: std::cout << "function"; break;
        case CodeSymbol::SymbolType::METHOD: std::cout << "method"; break;
        case CodeSymbol::SymbolType::CLASS: std::cout << "class"; break;
        case CodeSymbol::SymbolType::STRUCT: std::cout << "struct"; break;
        case CodeSymbol::SymbolType::VARIABLE: std::cout << "variable"; break;
        case CodeSymbol::SymbolType::FIELD: std::cout << "field"; break;
        case CodeSymbol::SymbolType::ENUM: std::cout << "enum"; break;
        case CodeSymbol::SymbolType::INTERFACE: std::cout << "interface"; break;
        case CodeSymbol::SymbolType::NAMESPACE: std::cout << "namespace"; break;
        case CodeSymbol::SymbolType::MODULE: std::cout << "module"; break;
        case CodeSymbol::SymbolType::PACKAGE: std::cout << "package"; break;
        case CodeSymbol::SymbolType::FILE: std::cout << "file"; break;
        default: std::cout << "unknown"; break;
    }
    
    std::cout << ")" << std::endl;
    std::cout << indent << "  Location: " << symbol.filePath << ":" << symbol.lineNumber << ":" << symbol.columnNumber << std::endl;
    
    for (const auto& [key, value] : symbol.metadata) {
        std::cout << indent << "  " << key << ": " << value << std::endl;
    }
}

void printReference(const SymbolReference& ref) {
    std::cout << "Reference: " << ref.symbolId << std::endl;
    std::cout << "  Location: " << ref.filePath << ":" << ref.lineNumber << ":" << ref.columnNumber << std::endl;
    std::cout << "  Is Definition: " << (ref.isDefinition ? "Yes" : "No") << std::endl;
}

void printRelation(const SymbolRelation& rel, const std::unordered_map<std::string, CodeSymbol>& symbols) {
    auto sourceIt = symbols.find(rel.sourceSymbolId);
    auto targetIt = symbols.find(rel.targetSymbolId);
    
    std::string sourceName = sourceIt != symbols.end() ? sourceIt->second.name : rel.sourceSymbolId;
    std::string targetName = targetIt != symbols.end() ? targetIt->second.name : rel.targetSymbolId;
    
    std::string relTypeStr;
    switch (rel.type) {
        case SymbolRelation::RelationType::CALLS: relTypeStr = "calls"; break;
        case SymbolRelation::RelationType::INHERITS_FROM: relTypeStr = "inherits from"; break;
        case SymbolRelation::RelationType::CONTAINS: relTypeStr = "contains"; break;
        case SymbolRelation::RelationType::IMPLEMENTS: relTypeStr = "implements"; break;
        case SymbolRelation::RelationType::USES: relTypeStr = "uses"; break;
        case SymbolRelation::RelationType::OVERRIDES: relTypeStr = "overrides"; break;
        case SymbolRelation::RelationType::DEPENDS_ON: relTypeStr = "depends on"; break;
        default: relTypeStr = "unknown relation"; break;
    }
    
    std::cout << "Relation: " << sourceName << " " << relTypeStr << " " << targetName << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <directory_to_index>" << std::endl;
        return 1;
    }
    
    std::string directoryPath = argv[1];
    
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cerr << "Error: " << directoryPath << " is not a valid directory." << std::endl;
        return 1;
    }
    
    std::cout << "Indexing directory: " << directoryPath << std::endl;
    
    // Create dependencies
    auto workspaceManager = std::make_shared<MockWorkspaceManager>();
    workspaceManager->addRootDirectory(directoryPath); // Add the directory to index as a root directory
    
    auto languageDetector = std::make_shared<LanguageDetector>();
    auto parserFactory = std::make_shared<CStyleLanguageParserFactory>();
    auto threadPool = std::make_shared<MockEditorCoreThreadPool>();
    
    // Create the codebase indexer with dependencies
    auto indexer = std::make_unique<CodebaseIndexer>(
        workspaceManager,
        languageDetector,
        parserFactory,
        threadPool
    );
    
    // Initialize the indexer
    indexer->initialize({directoryPath});
    
    // Wait for indexing to complete, showing progress
    std::cout << "Indexing in progress..." << std::endl;
    
    while (indexer->isIndexing()) {
        float progress = indexer->getIndexingProgress();
        int barWidth = 50;
        int pos = static_cast<int>(barWidth * progress);
        
        std::cout << "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << " %\r";
        std::cout.flush();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << std::endl << "Indexing complete!" << std::endl;
    
    // Get all files indexed
    auto files = indexer->getAllFiles();
    std::cout << "Total files indexed: " << files.size() << std::endl;
    
    // Collect statistics
    std::unordered_map<std::string, int> symbolTypeCounts;
    std::unordered_map<std::string, int> fileLanguageCounts;
    
    // Print indexed files by language
    for (const auto& fileInfo : files) {
        fileLanguageCounts[fileInfo.language]++;
    }
    
    std::cout << "\nFiles by language:" << std::endl;
    for (const auto& [language, count] : fileLanguageCounts) {
        std::cout << "  " << language << ": " << count << " files" << std::endl;
    }
    
    // Test symbol search
    std::string searchQuery;
    std::cout << "\nEnter a symbol name to search (or press Enter to skip): ";
    std::getline(std::cin, searchQuery);
    
    if (!searchQuery.empty()) {
        std::cout << "Searching for symbol: " << searchQuery << std::endl;
        auto symbols = indexer->findSymbolsByName(searchQuery, false);
        
        std::cout << "Found " << symbols.size() << " symbols matching \"" << searchQuery << "\":" << std::endl;
        for (const auto& symbol : symbols) {
            printSymbol(symbol);
            
            // For class symbols, show their methods and fields
            if (symbol.type == CodeSymbol::SymbolType::CLASS || symbol.type == CodeSymbol::SymbolType::STRUCT) {
                // Get all symbols in the same file
                auto fileSymbols = indexer->findSymbolsInFile(symbol.filePath);
                
                // Filter for symbols contained in this class
                for (const auto& fileSymbol : fileSymbols) {
                    if (fileSymbol.parentId && *fileSymbol.parentId == symbol.id) {
                        printSymbol(fileSymbol, 1);
                    }
                }
            }
            
            // Show references to this symbol
            auto references = indexer->getSymbolReferences(symbol.id);
            if (!references.empty()) {
                std::cout << "  References (" << references.size() << "):" << std::endl;
                for (size_t i = 0; i < std::min(references.size(), size_t(5)); ++i) {
                    std::cout << "    " << references[i].filePath << ":" << references[i].lineNumber << ":" << references[i].columnNumber << std::endl;
                }
                if (references.size() > 5) {
                    std::cout << "    ... and " << (references.size() - 5) << " more" << std::endl;
                }
            }
            
            // Show relations
            auto relations = indexer->getSymbolRelations(symbol.id, std::nullopt, false);
            if (!relations.empty()) {
                std::cout << "  Relations:" << std::endl;
                std::unordered_map<std::string, CodeSymbol> relatedSymbols;
                for (const auto& relation : relations) {
                    auto sourceSymbol = indexer->getSymbol(relation.sourceSymbolId);
                    if (sourceSymbol) {
                        relatedSymbols[relation.sourceSymbolId] = *sourceSymbol;
                    }
                    
                    auto targetSymbol = indexer->getSymbol(relation.targetSymbolId);
                    if (targetSymbol) {
                        relatedSymbols[relation.targetSymbolId] = *targetSymbol;
                    }
                }
                
                for (const auto& relation : relations) {
                    printRelation(relation, relatedSymbols);
                }
            }
            
            std::cout << std::endl;
        }
    }
    
    // Test file search
    std::cout << "\nEnter a file path or extension to search (or press Enter to skip): ";
    std::string fileSearchQuery;
    std::getline(std::cin, fileSearchQuery);
    
    if (!fileSearchQuery.empty()) {
        std::vector<FileInfo> matchingFiles;
        
        // Check if it's a language search (starts with dot or short string)
        bool isExtension = !fileSearchQuery.empty() && fileSearchQuery[0] == '.';
        if (isExtension || fileSearchQuery.size() <= 5) {
            matchingFiles = indexer->findFilesByLanguage(fileSearchQuery);
        } else {
            // It's a file path search - just filter the results
            for (const auto& fileInfo : files) {
                if (fileInfo.path.find(fileSearchQuery) != std::string::npos) {
                    matchingFiles.push_back(fileInfo);
                }
            }
        }
        
        std::cout << "Found " << matchingFiles.size() << " files matching \"" << fileSearchQuery << "\":" << std::endl;
        for (const auto& fileInfo : matchingFiles) {
            std::cout << "  " << fileInfo.path << " (" << fileInfo.language << ")" << std::endl;
        }
    }
    
    // Shutdown the indexer
    indexer->shutdown();
    
    return 0;
} 