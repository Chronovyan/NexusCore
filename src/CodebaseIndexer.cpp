#include "CodebaseIndexer.hpp"
#include "LanguageDetector.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>
#include <thread>
#include <utility>

namespace fs = std::filesystem;

namespace ai_editor {

// Helper function to create basic FileInfo
FileInfo createBasicFileInfo(const std::string& filePath, const std::string& language = "") {
    FileInfo info;
    info.path = filePath;
    info.language = language;
    // Other fields can be populated later
    return info;
}

// Helper function to convert CodeSymbol::SymbolType to string
std::string symbolTypeToString(CodeSymbol::SymbolType type) {
    switch (type) {
        case CodeSymbol::SymbolType::FUNCTION:
            return "function";
        case CodeSymbol::SymbolType::METHOD:
            return "method";
        case CodeSymbol::SymbolType::CLASS:
            return "class";
        case CodeSymbol::SymbolType::STRUCT:
            return "struct";
        case CodeSymbol::SymbolType::VARIABLE:
            return "variable";
        case CodeSymbol::SymbolType::FIELD:
            return "field";
        case CodeSymbol::SymbolType::ENUM:
            return "enum";
        case CodeSymbol::SymbolType::INTERFACE:
            return "interface";
        case CodeSymbol::SymbolType::NAMESPACE:
            return "namespace";
        case CodeSymbol::SymbolType::MODULE:
            return "module";
        case CodeSymbol::SymbolType::PACKAGE:
            return "package";
        case CodeSymbol::SymbolType::FILE:
            return "file";
        case CodeSymbol::SymbolType::UNKNOWN:
        default:
            return "unknown";
    }
}

CodebaseIndexer::CodebaseIndexer(
    std::shared_ptr<IWorkspaceManager> workspaceManager,
    std::shared_ptr<ILanguageDetector> languageDetector,
    std::shared_ptr<ILanguageParserFactory> parserFactory,
    std::shared_ptr<EditorCoreThreadPool> threadPool)
    : workspaceManager_(workspaceManager),
      languageDetector_(languageDetector),
      parserFactory_(parserFactory),
      threadPool_(threadPool),
      shutdownRequested_(false),
      isIndexing_(false),
      filesIndexed_(0),
      totalFilesToIndex_(0),
      nextCallbackId_(0) {
    
    // Start worker thread
    workerThread_ = std::thread(&CodebaseIndexer::indexWorker, this);
}

CodebaseIndexer::~CodebaseIndexer() {
    shutdown();
}

bool CodebaseIndexer::initialize(const std::vector<std::string>& rootDirectories) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Add root directories
    for (const auto& directory : rootDirectories) {
        if (fs::exists(directory) && fs::is_directory(directory)) {
            rootDirectories_.push_back(directory);
            
            // Queue indexing task for this directory
            addToIndexQueue({
                .type = IndexTask::TaskType::INDEX_DIRECTORY,
                .filePath = directory
            });
        } else {
            // Invalid directory, report error
            return false;
        }
    }
    
    return true;
}

void CodebaseIndexer::shutdown() {
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        if (shutdownRequested_) {
            return; // Already shutting down
        }
        
        shutdownRequested_ = true;
    }
    
    // Notify the worker thread to wake up
    queueCondition_.notify_all();
    
    // Wait for worker thread to finish
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    // Clear all data
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        
        rootDirectories_.clear();
        symbols_.clear();
        symbolsByFile_.clear();
        symbolsByName_.clear();
        symbolsByType_.clear();
        references_.clear();
        outboundRelations_.clear();
        inboundRelations_.clear();
        files_.clear();
        
        parserFactory_.reset();
        languageDetector_.reset();
    }
}

std::vector<std::string> CodebaseIndexer::getRootDirectories() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return rootDirectories_;
}

bool CodebaseIndexer::addRootDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Check if the directory exists
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        return false;
    }
    
    // Check if the directory is already in the list
    auto it = std::find(rootDirectories_.begin(), rootDirectories_.end(), directory);
    if (it != rootDirectories_.end()) {
        return true; // Already added
    }
    
    // Add the directory to the list
    rootDirectories_.push_back(directory);
    
    // Queue indexing task for this directory
    addToIndexQueue({
        .type = IndexTask::TaskType::INDEX_DIRECTORY,
        .filePath = directory
    });
    
    return true;
}

bool CodebaseIndexer::removeRootDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Find the directory in the list
    auto it = std::find(rootDirectories_.begin(), rootDirectories_.end(), directory);
    if (it == rootDirectories_.end()) {
        return false; // Not found
    }
    
    // Remove the directory from the list
    rootDirectories_.erase(it);
    
    // Remove all files and symbols from this directory
    removeDirectoryFromIndex(directory);
    
    return true;
}

std::optional<CodeSymbol> CodebaseIndexer::getSymbol(const std::string& symbolId) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = symbols_.find(symbolId);
    if (it != symbols_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<CodeSymbol> CodebaseIndexer::findSymbolsByName(const std::string& name, bool exactMatch) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<CodeSymbol> result;
    
    if (exactMatch) {
        auto range = symbolsByName_.equal_range(name);
        for (auto it = range.first; it != range.second; ++it) {
            auto symbolIt = symbols_.find(it->second);
            if (symbolIt != symbols_.end()) {
                result.push_back(symbolIt->second);
            }
        }
    } else {
        // Substring search
        for (const auto& [symbolName, symbolId] : symbolsByName_) {
            if (symbolName.find(name) != std::string::npos) {
                auto symbolIt = symbols_.find(symbolId);
                if (symbolIt != symbols_.end()) {
                    result.push_back(symbolIt->second);
                }
            }
        }
    }
    
    return result;
}

std::vector<CodeSymbol> CodebaseIndexer::findSymbolsByType(CodeSymbol::SymbolType type, bool includeChildren) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<CodeSymbol> result;
    
    auto range = symbolsByType_.equal_range(type);
    for (auto it = range.first; it != range.second; ++it) {
        auto symbolIt = symbols_.find(it->second);
        if (symbolIt != symbols_.end()) {
            result.push_back(symbolIt->second);
        }
    }
    
    // If includeChildren is true, we should also include symbols of derived types
    // This would require a mapping of which types are children of others
    // For now, we'll just return the exact type matches
    
    return result;
}

std::vector<CodeSymbol> CodebaseIndexer::findSymbolsInFile(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<CodeSymbol> result;
    
    auto range = symbolsByFile_.equal_range(filePath);
    for (auto it = range.first; it != range.second; ++it) {
        auto symbolIt = symbols_.find(it->second);
        if (symbolIt != symbols_.end()) {
            result.push_back(symbolIt->second);
        }
    }
    
    return result;
}

std::vector<SymbolReference> CodebaseIndexer::getSymbolReferences(const std::string& symbolId) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<SymbolReference> result;
    
    auto range = symbolReferences_.equal_range(symbolId);
    for (auto it = range.first; it != range.second; ++it) {
        auto refIt = references_.find(it->second);
        if (refIt != references_.end()) {
            result.push_back(refIt->second);
        }
    }
    
    return result;
}

std::vector<SymbolRelation> CodebaseIndexer::getSymbolRelations(
    const std::string& symbolId,
    std::optional<SymbolRelation::RelationType> relationType,
    bool inbound) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<SymbolRelation> result;
    
    // Get relations where this symbol is the source
    if (!inbound) {
        auto sourceRange = outboundRelations_.find(symbolId);
        if (sourceRange != outboundRelations_.end()) {
            for (const auto& relation : sourceRange->second) {
                // If a relation type filter is specified, only include matching relations
                if (!relationType || relation.type == *relationType) {
                    result.push_back(relation);
                }
            }
        }
    } else {
        // Get relations where this symbol is the target
        auto targetRange = inboundRelations_.find(symbolId);
        if (targetRange != inboundRelations_.end()) {
            for (const auto& relation : targetRange->second) {
                // If a relation type filter is specified, only include matching relations
                if (!relationType || relation.type == *relationType) {
                    result.push_back(relation);
                }
            }
        }
    }
    
    return result;
}

std::vector<FileInfo> CodebaseIndexer::getAllFiles() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<FileInfo> result;
    result.reserve(files_.size());
    
    for (const auto& [filePath, fileInfo] : files_) {
        result.push_back(fileInfo);
    }
    
    return result;
}

std::optional<FileInfo> CodebaseIndexer::getFileInfo(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = files_.find(filePath);
    if (it != files_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<FileInfo> CodebaseIndexer::findFilesByLanguage(const std::string& language) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<FileInfo> result;
    
    for (const auto& [filePath, fileInfo] : files_) {
        if (fileInfo.language == language) {
            result.push_back(fileInfo);
        }
    }
    
    return result;
}

std::vector<SearchResult> CodebaseIndexer::search(const std::string& query, size_t maxResults) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<SearchResult> result;
    
    // First search for symbols by name
    auto symbolsByName = findSymbolsByName(query, false);
    for (const auto& symbol : symbolsByName) {
        SearchResult searchResult;
        searchResult.type = SearchResultType::SYMBOL;
        searchResult.symbolId = symbol.id;
        searchResult.filePath = symbol.filePath;
        searchResult.lineNumber = symbol.lineNumber;
        searchResult.columnNumber = symbol.columnNumber;
        searchResult.name = symbol.name;
        searchResult.kind = symbolTypeToString(symbol.type);
        searchResult.snippet = getCodeSnippet(symbol.filePath, symbol.lineNumber, 3);
        
        result.push_back(std::move(searchResult));
        
        if (result.size() >= maxResults) {
            return result;
        }
    }
    
    // Then search for file paths containing the query
    for (const auto& [filePath, fileInfo] : files_) {
        if (filePath.find(query) != std::string::npos) {
            SearchResult searchResult;
            searchResult.type = SearchResultType::FILE;
            searchResult.filePath = filePath;
            searchResult.name = fs::path(filePath).filename().string();
            searchResult.kind = "file";
            
            result.push_back(std::move(searchResult));
            
            if (result.size() >= maxResults) {
                return result;
            }
        }
    }
    
    return result;
}

bool CodebaseIndexer::isIndexing() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return isIndexing_;
}

float CodebaseIndexer::getIndexingProgress() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return static_cast<float>(filesIndexed_) / totalFilesToIndex_;
}

bool CodebaseIndexer::reindex(bool incremental) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // If we're already indexing, don't start a new indexing process
    if (isIndexing_) {
        return false;
    }
    
    if (!incremental) {
        // Clear existing index for full reindex
        symbols_.clear();
        symbolsByFile_.clear();
        symbolsByName_.clear();
        symbolsByType_.clear();
        references_.clear();
        outboundRelations_.clear();
        inboundRelations_.clear();
        files_.clear();
    }
    
    // Reset progress tracking
    filesIndexed_ = 0;
    totalFilesToIndex_ = 0;
    
    // Queue indexing tasks for all root directories
    for (const auto& directory : rootDirectories_) {
        // Use C++17 compatible initialization instead of designated initializers
        IndexTask task;
        task.type = IndexTask::TaskType::INDEX_DIRECTORY;
        task.filePath = directory;
        addToIndexQueue(task);
    }
    
    // Set indexing flag
    isIndexing_ = true;
    
    return true;
}

void CodebaseIndexer::handleFileChange(const std::string& filePath, bool isCreate, bool isDelete) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Check if this file is within one of our root directories
    bool isInRootDirectory = false;
    for (const auto& directory : rootDirectories_) {
        if (filePath.find(directory) == 0) {
            isInRootDirectory = true;
            break;
        }
    }
    
    if (!isInRootDirectory) {
        return; // Ignore changes to files outside our root directories
    }
    
    if (isDelete) {
        // File was deleted, remove it from the index
        IndexTask task;
        task.type = IndexTask::TaskType::REMOVE_FILE;
        task.filePath = filePath;
        addToIndexQueue(task);
    } else {
        // File was created or modified
        IndexTask task;
        
        if (isCreate || files_.find(filePath) == files_.end()) {
            // New file
            task.type = IndexTask::TaskType::INDEX_FILE;
        } else {
            // Modified file
            task.type = IndexTask::TaskType::UPDATE_FILE;
        }
        
        task.filePath = filePath;
        
        addToIndexQueue(task);
    }
}

void CodebaseIndexer::indexWorker() {
    while (true) {
        IndexTask task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Wait for a task or shutdown request
            queueCondition_.wait(lock, [this] {
                return !indexQueue_.empty() || shutdownRequested_;
            });
            
            if (shutdownRequested_ && indexQueue_.empty()) {
                // Shutdown requested and no more tasks
                break;
            }
            
            // Get the next task
            if (!indexQueue_.empty()) {
                task = indexQueue_.front();
                indexQueue_.pop();
            } else {
                continue;
            }
        }
        
        // Process the task
        processTask(task);
    }
}

void CodebaseIndexer::processTask(const IndexTask& task) {
    switch (task.type) {
        case IndexTask::TaskType::INDEX_FILE:
            indexFile(task.filePath, task.content);
            break;
        
        case IndexTask::TaskType::REMOVE_FILE:
            removeFileFromIndex(task.filePath);
            break;
        
        case IndexTask::TaskType::UPDATE_FILE:
            updateFile(task.filePath, task.content);
            break;
        
        case IndexTask::TaskType::INDEX_DIRECTORY:
            indexDirectory(task.filePath);
            break;
    }
}

void CodebaseIndexer::indexFile(const std::string& filePath, const std::optional<std::string>& content) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Check if we should ignore this file
    if (languageDetector_->shouldIgnoreFile(filePath)) {
        return;
    }
    
    // Detect the language of the file
    auto languageInfo = languageDetector_->detectLanguageFromPath(filePath);
    if (!languageInfo) {
        // Try to detect from content
        std::string fileContent;
        if (content) {
            fileContent = *content;
        } else {
            std::ifstream file(filePath, std::ios::binary);
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                fileContent = buffer.str();
            }
        }
        
        if (!fileContent.empty()) {
            languageInfo = languageDetector_->detectLanguageFromContent(fileContent, filePath);
        }
    }
    
    if (!languageInfo) {
        // Unknown language, skip this file
        return;
    }
    
    // Get a parser for this language
    auto parser = parserFactory_->createParserForLanguage(languageInfo->id);
    if (!parser) {
        // No parser available for this language
        return;
    }
    
    // Parse the file
    auto parseResult = parser->parseFile(filePath, content);
    if (!parseResult.success) {
        // Parse failed
        // TODO: Log error
        std::cerr << "Failed to parse file: " << filePath << " - " << parseResult.errorMessage << std::endl;
        return;
    }
    
    // Process all extracted symbols
    for (const auto& symbol : parseResult.symbols) {
        // Add to symbols map
        symbols_[symbol.id] = symbol;
        
        // Add to index maps
        symbolsByFile_.emplace(filePath, symbol.id);
        symbolsByName_.emplace(symbol.name, symbol.id);
        symbolsByType_.emplace(symbol.type, symbol.id);
    }
    
    // Process all extracted references
    for (const auto& reference : parseResult.references) {
        // Add to references map
        references_[reference.symbolId + "|" + reference.filePath + "|" + 
                  std::to_string(reference.lineNumber) + "|" + 
                  std::to_string(reference.columnNumber)] = reference;
    }
    
    // Process all extracted relations
    for (const auto& relation : parseResult.relations) {
        // Generate a unique key for the relation
        std::string relationKey = relation.sourceSymbolId + "|" + 
                               relation.targetSymbolId + "|" + 
                               std::to_string(static_cast<int>(relation.type));
        
        // Add to relations map
        relations_[relationKey] = relation;
        
        // Add to outbound and inbound relation maps
        outboundRelations_[relation.sourceSymbolId].push_back(relation);
        inboundRelations_[relation.targetSymbolId].push_back(relation);
    }
    
    // Update file info
    auto fileInfo = createBasicFileInfo(filePath, parseResult.language);
    fileInfo.symbols = parseResult.symbolIds;
    fileInfo.metadata["lastIndexed"] = std::to_string(std::time(nullptr));
    fileInfo.metadata["language"] = parseResult.language;
    files_[filePath] = std::move(fileInfo);
    
    // Add to language index
    filesByLanguage_.emplace(parseResult.language, filePath);
    
    // Update progress
    filesIndexed_++;
    
    // Notify callbacks
    notifyUpdateCallbacks();
}

void CodebaseIndexer::removeFileFromIndex(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Remove file info
    files_.erase(filePath);
    
    // Find all symbols in this file
    auto range = symbolsByFile_.equal_range(filePath);
    std::vector<std::string> symbolIds;
    
    for (auto it = range.first; it != range.second; ++it) {
        symbolIds.push_back(it->second);
    }
    
    // Remove symbols from the index
    for (const auto& symbolId : symbolIds) {
        auto symbolIt = symbols_.find(symbolId);
        if (symbolIt != symbols_.end()) {
            // Remove from symbolsByName and symbolsByType
            auto nameRange = symbolsByName_.equal_range(symbolIt->second.name);
            for (auto nameIt = nameRange.first; nameIt != nameRange.second; ++nameIt) {
                if (nameIt->second == symbolId) {
                    symbolsByName_.erase(nameIt);
                    break;
                }
            }
            
            auto typeIt = symbolsByType_.find(symbolIt->second.type);
            if (typeIt != symbolsByType_.end()) {
                symbolsByType_.erase(typeIt);
            }
            
            // Remove the symbol
            symbols_.erase(symbolIt);
        }
    }
    
    // Remove from symbolsByFile
    symbolsByFile_.erase(filePath);
    
    // Remove references in this file
    std::vector<std::string> referenceIdsToRemove;
    for (const auto& [id, reference] : references_) {
        if (reference.filePath == filePath) {
            referenceIdsToRemove.push_back(id);
        }
    }
    
    for (const auto& refId : referenceIdsToRemove) {
        auto refIt = references_.find(refId);
        if (refIt != references_.end()) {
            // Remove from references
            references_.erase(refIt);
        }
    }
}

void CodebaseIndexer::updateFile(const std::string& filePath, 
                               const std::optional<std::string>& content) {
    // For now, just remove the file and re-index it
    // In the future, we could implement incremental parsing
    removeFileFromIndex(filePath);
    indexFile(filePath, content);
}

void CodebaseIndexer::indexDirectory(const std::string& directory) {
    std::vector<std::string> filesToIndex;
    
    // Collect all files in the directory
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                
                // Skip files that should be ignored
                if (languageDetector_->shouldIgnoreFile(filePath)) {
                    continue;
                }
                
                filesToIndex.push_back(filePath);
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Handle filesystem errors
        // TODO: Add proper error logging
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    // Update total files count
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        totalFilesToIndex_ += filesToIndex.size();
    }
    
    // Queue tasks for each file
    for (const auto& filePath : filesToIndex) {
        IndexTask task;
        task.type = IndexTask::TaskType::INDEX_FILE;
        task.filePath = filePath;
        
        addToIndexQueue(task);
    }
}

void CodebaseIndexer::removeDirectoryFromIndex(const std::string& directory) {
    std::vector<std::string> filesToRemove;
    
    // Find all files in the directory
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        
        for (const auto& [filePath, _] : files_) {
            if (filePath.find(directory) == 0) {
                filesToRemove.push_back(filePath);
            }
        }
    }
    
    // Remove each file
    for (const auto& filePath : filesToRemove) {
        removeFileFromIndex(filePath);
    }
}

void CodebaseIndexer::addToIndexQueue(const IndexTask& task) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    indexQueue_.push(task);
    queueCondition_.notify_one();
}

std::string CodebaseIndexer::getCodeSnippet(const std::string& filePath, size_t lineNumber, size_t contextLines) const {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return "";
        }
        
        std::string line;
        size_t currentLine = 1;
        
        // Skip to the first line we're interested in
        size_t firstLine = lineNumber > contextLines ? lineNumber - contextLines : 1;
        while (currentLine < firstLine && std::getline(file, line)) {
            currentLine++;
        }
        
        // Read the lines we're interested in
        std::stringstream snippet;
        size_t lastLine = lineNumber + contextLines;
        
        while (currentLine <= lastLine && std::getline(file, line)) {
            if (currentLine == lineNumber) {
                snippet << "-> ";
            } else {
                snippet << "   ";
            }
            
            snippet << currentLine << ": " << line << "\n";
            currentLine++;
        }
        
        return snippet.str();
    } catch (...) {
        return "";
    }
}

int CodebaseIndexer::registerUpdateCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    int callbackId = nextCallbackId_++;
    updateCallbacks_[callbackId] = callback;
    
    return callbackId;
}

void CodebaseIndexer::unregisterUpdateCallback(int callbackId) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    updateCallbacks_.erase(callbackId);
}

void CodebaseIndexer::notifyUpdateCallbacks() {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    for (const auto& [_, callback] : updateCallbacks_) {
        callback();
    }
}

} // namespace ai_editor 