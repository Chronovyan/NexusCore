#pragma once

#include "interfaces/ICodebaseIndex.hpp"
#include "interfaces/ILanguageDetector.hpp"
#include "interfaces/ILanguageParser.hpp"
#include "interfaces/IWorkspaceManager.hpp"
#include "EditorErrorReporter.h"
#include "EditorCoreThreadPool.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>
#include <chrono>
#include <future>
#include <filesystem>

namespace ai_editor {

/**
 * @struct IndexTask
 * @brief Represents a task for indexing a file
 */
struct IndexTask {
    enum class TaskType {
        INDEX_FILE,
        REMOVE_FILE,
        UPDATE_FILE,
        INDEX_DIRECTORY
    };
    
    TaskType type;
    std::string filePath;
    std::optional<std::string> content;
};

/**
 * @class CodebaseIndexer
 * @brief Implements the codebase indexing functionality
 * 
 * This class is responsible for building and maintaining an index of
 * the codebase, including symbols, references, and relationships.
 */
class CodebaseIndexer : public ICodebaseIndex {
public:
    /**
     * @brief Constructor
     * 
     * @param workspaceManager The workspace manager
     * @param languageDetector The language detector
     * @param parserFactory The language parser factory
     * @param threadPool The thread pool for async operations
     */
    CodebaseIndexer(
        std::shared_ptr<IWorkspaceManager> workspaceManager,
        std::shared_ptr<ILanguageDetector> languageDetector,
        std::shared_ptr<ILanguageParserFactory> parserFactory,
        std::shared_ptr<EditorCoreThreadPool> threadPool);
    
    /**
     * @brief Destructor
     */
    ~CodebaseIndexer();
    
    // ICodebaseIndex implementation
    std::vector<std::string> getRootDirectories() const override;
    bool addRootDirectory(const std::string& directory) override;
    bool removeRootDirectory(const std::string& directory) override;
    std::optional<CodeSymbol> getSymbol(const std::string& symbolId) const override;
    std::vector<CodeSymbol> findSymbolsByName(const std::string& name, bool exactMatch) const override;
    std::vector<CodeSymbol> findSymbolsByType(CodeSymbol::SymbolType type, bool includeChildren) const override;
    std::vector<CodeSymbol> findSymbolsInFile(const std::string& filePath) const override;
    std::vector<SymbolReference> getSymbolReferences(const std::string& symbolId) const override;
    std::vector<SymbolRelation> getSymbolRelations(
        const std::string& symbolId,
        std::optional<SymbolRelation::RelationType> relationType,
        bool inbound) const override;
    std::vector<FileInfo> getAllFiles() const override;
    std::optional<FileInfo> getFileInfo(const std::string& filePath) const override;
    std::vector<FileInfo> findFilesByLanguage(const std::string& language) const override;
    std::vector<SearchResult> search(const std::string& query, size_t maxResults) const override;
    bool isIndexing() const override;
    float getIndexingProgress() const override;
    bool reindex(bool incremental) override;
    int registerUpdateCallback(std::function<void()> callback) override;
    void unregisterUpdateCallback(int callbackId) override;
    
    /**
     * @brief Initialize the indexer
     * 
     * @param rootDirectories The root directories to index
     * @return bool True if initialization was successful
     */
    bool initialize(const std::vector<std::string>& rootDirectories = {});
    
    /**
     * @brief Shut down the indexer
     */
    void shutdown();
    
    /**
     * @brief Handle a file change event
     * 
     * @param filePath The path to the changed file
     * @param isCreate Whether this is a new file
     * @param isDelete Whether the file was deleted
     */
    void handleFileChange(const std::string& filePath, bool isCreate, bool isDelete);

private:
    // Helper methods
    void indexWorker();
    bool indexFile(const std::string& filePath, const std::string& content);
    bool indexFileWithParser(
        const std::string& filePath,
        const std::string& content,
        const std::string& languageId,
        std::shared_ptr<ILanguageParser> parser);
    void processTask(const IndexTask& task);
    void indexFile(const std::string& filePath, const std::optional<std::string>& content);
    void updateFile(const std::string& filePath, const std::optional<std::string>& content);
    void indexDirectory(const std::string& directory);
    void removeFileFromIndex(const std::string& filePath);
    void removeDirectoryFromIndex(const std::string& directory);
    bool shouldIndexFile(const std::string& filePath) const;
    void scanDirectory(const std::string& directory);
    void addToIndexQueue(const IndexTask& task);
    void notifyUpdateCallbacks();
    std::string calculateFileHash(const std::string& content) const;
    std::string getCodeSnippet(const std::string& filePath, size_t lineNumber, size_t contextLines) const;
    
    // Data structures
    std::unordered_map<std::string, CodeSymbol> symbols_;
    std::unordered_map<std::string, FileInfo> files_;
    std::unordered_map<std::string, SymbolReference> references_;
    std::unordered_map<std::string, SymbolRelation> relations_;
    std::unordered_map<std::string, std::vector<SymbolRelation>> outboundRelations_;
    std::unordered_map<std::string, std::vector<SymbolRelation>> inboundRelations_;
    
    // Index by name (for faster lookups)
    std::unordered_multimap<std::string, std::string> symbolsByName_;
    std::unordered_map<CodeSymbol::SymbolType, std::string> symbolsByType_;
    std::unordered_multimap<std::string, std::string> symbolsByFile_;
    std::unordered_multimap<std::string, std::string> filesByLanguage_;
    
    // Root directories
    std::vector<std::string> rootDirectories_;
    
    // Dependencies
    std::shared_ptr<IWorkspaceManager> workspaceManager_;
    std::shared_ptr<ILanguageDetector> languageDetector_;
    std::shared_ptr<ILanguageParserFactory> parserFactory_;
    std::shared_ptr<EditorCoreThreadPool> threadPool_;
    
    // Worker thread
    std::thread workerThread_;
    std::atomic<bool> shutdownRequested_;
    std::queue<IndexTask> indexQueue_;  // Queue for indexing tasks
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    
    // Indexing state
    std::atomic<bool> isIndexing_;
    std::atomic<size_t> filesIndexed_;
    std::atomic<size_t> totalFilesToIndex_;
    
    // Update callbacks
    std::unordered_map<int, std::function<void()>> updateCallbacks_;
    std::atomic<int> nextCallbackId_;
    mutable std::mutex callbackMutex_;
    
    // Mutex for thread safety
    mutable std::mutex dataMutex_;
};

} // namespace ai_editor 