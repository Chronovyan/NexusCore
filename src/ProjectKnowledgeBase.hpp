#pragma once

#include "interfaces/IProjectKnowledgeBase.hpp"
#include "EditorErrorReporter.h"
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace ai_editor {

/**
 * @class ProjectKnowledgeBase
 * @brief Implementation of the IProjectKnowledgeBase interface
 * 
 * This class provides a concrete implementation of the IProjectKnowledgeBase
 * interface for storing and retrieving project-specific knowledge.
 */
class ProjectKnowledgeBase : public IProjectKnowledgeBase {
public:
    /**
     * @brief Constructor
     */
    ProjectKnowledgeBase();
    
    /**
     * @brief Constructor with initial entries
     * 
     * @param entries Initial entries for the knowledge base
     */
    explicit ProjectKnowledgeBase(const std::vector<KnowledgeEntry>& entries);
    
    /**
     * @brief Destructor
     */
    ~ProjectKnowledgeBase() override = default;
    
    // IProjectKnowledgeBase implementation
    bool addEntry(const KnowledgeEntry& entry) override;
    bool updateEntry(const std::string& entryId, const KnowledgeEntry& updatedEntry) override;
    bool removeEntry(const std::string& entryId) override;
    std::optional<KnowledgeEntry> getEntry(const std::string& entryId) const override;
    std::vector<KnowledgeEntry> query(const KnowledgeQuery& query) const override;
    std::vector<KnowledgeEntry> query(const std::string& queryText, size_t maxResults = 10) const override;
    std::vector<KnowledgeEntry> findByCategory(KnowledgeCategory category, size_t maxResults) const override;
    std::vector<KnowledgeEntry> findByCustomCategory(const std::string& customCategory, size_t maxResults) const override;
    std::vector<KnowledgeEntry> findByTags(const std::vector<std::string>& tags, bool matchAll, size_t maxResults) const override;
    std::vector<KnowledgeEntry> findRelevantForContext(const std::vector<std::string>& contextTerms, std::optional<KnowledgeCategory> category, size_t maxResults) const override;
    void registerRelevanceScorer(const std::string& name, KnowledgeRelevanceScorer scorer) override;
    bool loadFromFile(const std::string& filePath) override;
    bool saveToFile(const std::string& filePath) const override;
    size_t getEntryCount() const override;
    std::vector<KnowledgeEntry> getAllEntries() const override;
    void clear() override;
    size_t importEntries(const IProjectKnowledgeBase& otherKnowledgeBase, bool overwriteExisting) override;
    std::vector<KnowledgeCategory> getAvailableCategories() const override;
    std::vector<std::string> getAvailableCustomCategories() const override;
    std::vector<std::string> getAvailableTags() const override;

private:
    /**
     * @brief Calculate the relevance score for an entry
     * 
     * @param entry The entry to score
     * @param queryText The query text
     * @param contextTerms The context terms
     * @return float The relevance score (0.0 - 1.0)
     */
    float calculateRelevance(
        const KnowledgeEntry& entry, 
        const std::string& queryText,
        const std::vector<std::string>& contextTerms) const;
    
    /**
     * @brief Filter entries by criteria
     * 
     * @param entries The entries to filter
     * @param query The query criteria
     * @return std::vector<KnowledgeEntry> The filtered entries
     */
    std::vector<KnowledgeEntry> filterEntries(
        const std::vector<KnowledgeEntry>& entries,
        const KnowledgeQuery& query) const;
    
    /**
     * @brief Search for text in an entry
     * 
     * @param entry The entry to search in
     * @param searchText The text to search for
     * @return bool True if the text is found
     */
    bool searchInEntry(const KnowledgeEntry& entry, const std::string& searchText) const;
    
    /**
     * @brief Check if an entry matches tags
     * 
     * @param entry The entry to check
     * @param tags The tags to match
     * @param matchAll Whether all tags must match
     * @return bool True if the entry matches the tags
     */
    bool matchesTags(const KnowledgeEntry& entry, const std::vector<std::string>& tags, bool matchAll) const;
    
    /**
     * @brief Generate a unique ID for an entry
     * 
     * @return std::string A unique ID
     */
    std::string generateUniqueId() const;
    
    /**
     * @brief Convert a string to lowercase
     * 
     * @param str The string to convert
     * @return std::string The lowercase string
     */
    std::string toLower(const std::string& str) const;
    
    /**
     * @brief Extract key terms from text
     * 
     * @param text The text to extract terms from
     * @return std::vector<std::string> The extracted terms
     */
    std::vector<std::string> extractKeyTerms(const std::string& text) const;
    
    /**
     * @brief Calculate the similarity between two terms
     * 
     * @param term1 The first term
     * @param term2 The second term
     * @return float The similarity score (0.0 - 1.0)
     */
    float calculateTermSimilarity(const std::string& term1, const std::string& term2) const;

    // Member variables
    std::unordered_map<std::string, KnowledgeEntry> entries_;
    std::unordered_map<KnowledgeCategory, std::vector<std::string>> entriesByCategory_;
    std::unordered_map<std::string, std::vector<std::string>> entriesByCustomCategory_;
    std::unordered_map<std::string, std::vector<std::string>> entriesByTag_;
    std::unordered_map<std::string, KnowledgeRelevanceScorer> relevanceScorers_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

/**
 * @class ProjectKnowledgeManager
 * @brief Implementation of the IProjectKnowledgeManager interface
 * 
 * This class provides a concrete implementation of the IProjectKnowledgeManager
 * interface for managing project knowledge bases.
 */
class ProjectKnowledgeManager : public IProjectKnowledgeManager {
public:
    /**
     * @brief Constructor
     */
    ProjectKnowledgeManager();
    
    /**
     * @brief Destructor
     */
    ~ProjectKnowledgeManager() override = default;
    
    // IProjectKnowledgeManager implementation
    std::shared_ptr<IProjectKnowledgeBase> getKnowledgeBase(
        const std::string& projectPath,
        bool createIfNotExists) override;
    
    std::shared_ptr<IProjectKnowledgeBase> createKnowledgeBase(
        const std::string& projectPath,
        bool overwriteExisting) override;
    
    bool closeKnowledgeBase(
        const std::string& projectPath,
        bool save) override;
    
    std::string getDefaultKnowledgeBasePath(
        const std::string& projectPath) const override;
    
    bool knowledgeBaseExists(
        const std::string& projectPath) const override;
    
    std::shared_ptr<IProjectKnowledgeBase> importKnowledgeBase(
        const std::string& projectPath,
        const std::string& filePath,
        bool overwriteExisting) override;
    
    bool exportKnowledgeBase(
        const std::string& projectPath,
        const std::string& filePath) const override;
    
    bool deleteKnowledgeBase(
        const std::string& projectPath) override;
    
    std::vector<std::string> getProjectsWithKnowledgeBases() const override;

private:
    /**
     * @brief Normalize a project path
     * 
     * @param projectPath The project path to normalize
     * @return std::string The normalized path
     */
    std::string normalizePath(const std::string& projectPath) const;
    
    // Member variables
    std::unordered_map<std::string, std::shared_ptr<IProjectKnowledgeBase>> knowledgeBases_;
    mutable std::mutex mutex_;
};

// Utility functions for JSON serialization

/**
 * @brief Convert a KnowledgeCategory to a string
 * 
 * @param category The category to convert
 * @return std::string The string representation
 */
std::string knowledgeCategoryToString(KnowledgeCategory category);

/**
 * @brief Convert a string to a KnowledgeCategory
 * 
 * @param categoryStr The string to convert
 * @return KnowledgeCategory The category
 */
KnowledgeCategory stringToKnowledgeCategory(const std::string& categoryStr);

/**
 * @brief Serialize a KnowledgeEntry to JSON
 * 
 * @param entry The entry to serialize
 * @return nlohmann::json The JSON representation
 */
nlohmann::json serializeKnowledgeEntry(const KnowledgeEntry& entry);

/**
 * @brief Deserialize a KnowledgeEntry from JSON
 * 
 * @param json The JSON to deserialize
 * @return KnowledgeEntry The deserialized entry
 */
KnowledgeEntry deserializeKnowledgeEntry(const nlohmann::json& json);

} // namespace ai_editor 