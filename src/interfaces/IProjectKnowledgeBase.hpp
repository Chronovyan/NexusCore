#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <functional>

namespace ai_editor {

/**
 * @enum KnowledgeCategory
 * @brief Categories of project-specific knowledge
 */
enum class KnowledgeCategory {
    ARCHITECTURE,      // Project architecture and design patterns
    CODING_STANDARDS,  // Coding conventions and standards
    TERMINOLOGY,       // Domain-specific terminology and concepts
    API_USAGE,         // Common APIs and their usage patterns
    PATTERNS,          // Recurring code patterns
    DOCUMENTATION,     // Project documentation
    CUSTOM             // Custom categories
};

/**
 * @struct KnowledgeEntry
 * @brief An entry in the project knowledge base
 */
struct KnowledgeEntry {
    std::string id;                    // Unique identifier for the entry
    std::string title;                 // Title of the entry
    std::string content;               // Content of the entry
    std::string type;                  // Type of the entry
    KnowledgeCategory category;        // Category of the entry
    std::vector<std::string> tags;     // Tags for the entry
    std::string customCategory;        // Name for custom category
    float relevanceScore;              // Score for ranking entries (0.0 - 1.0)
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
    std::string created;               // Creation timestamp
    std::string updated;               // Last update timestamp
    
    // Constructors
    KnowledgeEntry() : relevanceScore(0.5f) {}
    
    KnowledgeEntry(
        const std::string& id,
        const std::string& title,
        const std::string& content,
        KnowledgeCategory category,
        const std::vector<std::string>& tags = {},
        float relevanceScore = 0.5f
    ) : id(id), title(title), content(content), category(category), 
        tags(tags), relevanceScore(relevanceScore) {}
};

/**
 * @struct KnowledgeQuery
 * @brief A query for retrieving knowledge entries
 */
struct KnowledgeQuery {
    std::string searchText;            // Text to search for
    std::optional<KnowledgeCategory> category; // Optional category filter
    std::vector<std::string> tags;     // Tags to filter by
    std::string customCategory;        // Custom category to filter by
    float minRelevance;                // Minimum relevance score (0.0 - 1.0)
    size_t maxResults;                 // Maximum number of results to return
    
    // Constructor with defaults
    KnowledgeQuery(
        const std::string& searchText = "",
        std::optional<KnowledgeCategory> category = std::nullopt,
        const std::vector<std::string>& tags = {},
        const std::string& customCategory = "",
        float minRelevance = 0.0f,
        size_t maxResults = 10
    ) : searchText(searchText), category(category), tags(tags),
        customCategory(customCategory), minRelevance(minRelevance),
        maxResults(maxResults) {}
};

/**
 * @typedef KnowledgeRelevanceScorer
 * @brief Function type for scoring knowledge entry relevance
 */
using KnowledgeRelevanceScorer = std::function<float(
    const KnowledgeEntry&, 
    const std::string& queryText,
    const std::vector<std::string>& contextTerms
)>;

/**
 * @interface IProjectKnowledgeBase
 * @brief Interface for accessing and managing project-specific knowledge
 * 
 * This interface provides methods to store, retrieve, and manage
 * project-specific knowledge that enhances AI assistance.
 */
class IProjectKnowledgeBase {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IProjectKnowledgeBase() = default;
    
    /**
     * @brief Add a knowledge entry to the knowledge base
     * 
     * @param entry The entry to add
     * @return bool True if successful
     */
    virtual bool addEntry(const KnowledgeEntry& entry) = 0;
    
    /**
     * @brief Update an existing knowledge entry
     * 
     * @param entryId The ID of the entry to update
     * @param updatedEntry The updated entry
     * @return bool True if successful
     */
    virtual bool updateEntry(const std::string& entryId, const KnowledgeEntry& updatedEntry) = 0;
    
    /**
     * @brief Remove a knowledge entry
     * 
     * @param entryId The ID of the entry to remove
     * @return bool True if successful
     */
    virtual bool removeEntry(const std::string& entryId) = 0;
    
    /**
     * @brief Get a knowledge entry by ID
     * 
     * @param entryId The ID of the entry
     * @return std::optional<KnowledgeEntry> The entry, if found
     */
    virtual std::optional<KnowledgeEntry> getEntry(const std::string& entryId) const = 0;
    
    /**
     * @brief Query the knowledge base
     * 
     * @param query The query parameters
     * @return std::vector<KnowledgeEntry> Matching entries
     */
    virtual std::vector<KnowledgeEntry> query(const KnowledgeQuery& query) const = 0;
    
    /**
     * @brief Query the knowledge base with a simple text search
     * 
     * @param queryText Text to search for
     * @param maxResults Maximum number of results to return
     * @return std::vector<KnowledgeEntry> Matching entries
     */
    virtual std::vector<KnowledgeEntry> query(const std::string& queryText, size_t maxResults = 10) const = 0;
    
    /**
     * @brief Find entries by category
     * 
     * @param category The category to filter by
     * @param maxResults Maximum number of results to return
     * @return std::vector<KnowledgeEntry> Matching entries
     */
    virtual std::vector<KnowledgeEntry> findByCategory(
        KnowledgeCategory category,
        size_t maxResults = 10) const = 0;
    
    /**
     * @brief Find entries by custom category
     * 
     * @param customCategory The custom category to filter by
     * @param maxResults Maximum number of results to return
     * @return std::vector<KnowledgeEntry> Matching entries
     */
    virtual std::vector<KnowledgeEntry> findByCustomCategory(
        const std::string& customCategory,
        size_t maxResults = 10) const = 0;
    
    /**
     * @brief Find entries by tags
     * 
     * @param tags The tags to filter by
     * @param matchAll Whether all tags must match (AND) or any tag (OR)
     * @param maxResults Maximum number of results to return
     * @return std::vector<KnowledgeEntry> Matching entries
     */
    virtual std::vector<KnowledgeEntry> findByTags(
        const std::vector<std::string>& tags,
        bool matchAll = true,
        size_t maxResults = 10) const = 0;
    
    /**
     * @brief Find relevant entries for a context
     * 
     * @param contextTerms Key terms from the current context
     * @param category Optional category filter
     * @param maxResults Maximum number of results to return
     * @return std::vector<KnowledgeEntry> Relevant entries
     */
    virtual std::vector<KnowledgeEntry> findRelevantForContext(
        const std::vector<std::string>& contextTerms,
        std::optional<KnowledgeCategory> category = std::nullopt,
        size_t maxResults = 5) const = 0;
    
    /**
     * @brief Register a custom relevance scorer
     * 
     * @param name Name for the scorer
     * @param scorer The scorer function
     */
    virtual void registerRelevanceScorer(
        const std::string& name,
        KnowledgeRelevanceScorer scorer) = 0;
    
    /**
     * @brief Load knowledge base from file
     * 
     * @param filePath Path to the knowledge base file
     * @return bool True if successful
     */
    virtual bool loadFromFile(const std::string& filePath) = 0;
    
    /**
     * @brief Save knowledge base to file
     * 
     * @param filePath Path to save the knowledge base
     * @return bool True if successful
     */
    virtual bool saveToFile(const std::string& filePath) const = 0;
    
    /**
     * @brief Get the number of entries in the knowledge base
     * 
     * @return size_t Number of entries
     */
    virtual size_t getEntryCount() const = 0;
    
    /**
     * @brief Get all entries in the knowledge base
     * 
     * @return std::vector<KnowledgeEntry> All entries
     */
    virtual std::vector<KnowledgeEntry> getAllEntries() const = 0;
    
    /**
     * @brief Clear all entries in the knowledge base
     */
    virtual void clear() = 0;
    
    /**
     * @brief Import entries from another knowledge base
     * 
     * @param otherKnowledgeBase The knowledge base to import from
     * @param overwriteExisting Whether to overwrite existing entries
     * @return size_t Number of entries imported
     */
    virtual size_t importEntries(
        const IProjectKnowledgeBase& otherKnowledgeBase,
        bool overwriteExisting = false) = 0;
    
    /**
     * @brief Get all available categories
     * 
     * @return std::vector<KnowledgeCategory> All categories with entries
     */
    virtual std::vector<KnowledgeCategory> getAvailableCategories() const = 0;
    
    /**
     * @brief Get all available custom categories
     * 
     * @return std::vector<std::string> All custom categories with entries
     */
    virtual std::vector<std::string> getAvailableCustomCategories() const = 0;
    
    /**
     * @brief Get all available tags
     * 
     * @return std::vector<std::string> All tags used in entries
     */
    virtual std::vector<std::string> getAvailableTags() const = 0;
};

/**
 * @interface IProjectKnowledgeManager
 * @brief Interface for managing project knowledge bases
 * 
 * This interface provides methods to create, open, and manage
 * project knowledge bases for different projects.
 */
class IProjectKnowledgeManager {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IProjectKnowledgeManager() = default;
    
    /**
     * @brief Get the knowledge base for a project
     * 
     * @param projectPath Path to the project
     * @param createIfNotExists Whether to create a new knowledge base if one doesn't exist
     * @return std::shared_ptr<IProjectKnowledgeBase> The knowledge base
     */
    virtual std::shared_ptr<IProjectKnowledgeBase> getKnowledgeBase(
        const std::string& projectPath,
        bool createIfNotExists = true) = 0;
    
    /**
     * @brief Create a new knowledge base for a project
     * 
     * @param projectPath Path to the project
     * @param overwriteExisting Whether to overwrite an existing knowledge base
     * @return std::shared_ptr<IProjectKnowledgeBase> The new knowledge base
     */
    virtual std::shared_ptr<IProjectKnowledgeBase> createKnowledgeBase(
        const std::string& projectPath,
        bool overwriteExisting = false) = 0;
    
    /**
     * @brief Close a knowledge base
     * 
     * @param projectPath Path to the project
     * @param save Whether to save changes before closing
     * @return bool True if successful
     */
    virtual bool closeKnowledgeBase(
        const std::string& projectPath,
        bool save = true) = 0;
    
    /**
     * @brief Get the default knowledge base file path for a project
     * 
     * @param projectPath Path to the project
     * @return std::string The default knowledge base file path
     */
    virtual std::string getDefaultKnowledgeBasePath(
        const std::string& projectPath) const = 0;
    
    /**
     * @brief Check if a knowledge base exists for a project
     * 
     * @param projectPath Path to the project
     * @return bool True if a knowledge base exists
     */
    virtual bool knowledgeBaseExists(
        const std::string& projectPath) const = 0;
    
    /**
     * @brief Import a knowledge base from a file
     * 
     * @param projectPath Path to the project
     * @param filePath Path to the knowledge base file
     * @param overwriteExisting Whether to overwrite existing entries
     * @return std::shared_ptr<IProjectKnowledgeBase> The imported knowledge base
     */
    virtual std::shared_ptr<IProjectKnowledgeBase> importKnowledgeBase(
        const std::string& projectPath,
        const std::string& filePath,
        bool overwriteExisting = false) = 0;
    
    /**
     * @brief Export a knowledge base to a file
     * 
     * @param projectPath Path to the project
     * @param filePath Path to save the knowledge base
     * @return bool True if successful
     */
    virtual bool exportKnowledgeBase(
        const std::string& projectPath,
        const std::string& filePath) const = 0;
    
    /**
     * @brief Delete a knowledge base
     * 
     * @param projectPath Path to the project
     * @return bool True if successful
     */
    virtual bool deleteKnowledgeBase(
        const std::string& projectPath) = 0;
    
    /**
     * @brief Get a list of projects with knowledge bases
     * 
     * @return std::vector<std::string> Paths to projects with knowledge bases
     */
    virtual std::vector<std::string> getProjectsWithKnowledgeBases() const = 0;
};

} // namespace ai_editor 