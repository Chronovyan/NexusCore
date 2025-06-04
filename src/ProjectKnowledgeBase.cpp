#include "ProjectKnowledgeBase.hpp"
#include "EditorErrorReporter.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace ai_editor;

#include <algorithm>
#include <chrono>
#include <random>
#include <regex>
#include <cctype>
#include <set>
#include <filesystem>

namespace ai_editor {

ProjectKnowledgeBase::ProjectKnowledgeBase() {
    // Default constructor implementation
}

ProjectKnowledgeBase::ProjectKnowledgeBase(const std::vector<KnowledgeEntry>& entries) {
    // Add all entries to the knowledge base
    for (const auto& entry : entries) {
        addEntry(entry);
    }
}

float ProjectKnowledgeBase::calculateRelevance(
    const KnowledgeEntry& entry, 
    const std::string& queryText,
    const std::vector<std::string>& contextTerms) const {
    
    // Apply all registered relevance scorers and average the results
    float totalScore = 0.0f;
    
    // Apply each registered scorer
    for (const auto& [name, scorer] : relevanceScorers_) {
        totalScore += scorer(entry, queryText, contextTerms);
    }
    
    // Average the scores
    float avgScore = relevanceScorers_.empty() ? 0.5f : totalScore / relevanceScorers_.size();
    
    // Ensure the score is in the valid range
    return std::max(0.0f, std::min(1.0f, avgScore));
}

std::vector<KnowledgeEntry> ProjectKnowledgeBase::findByCategory(
    KnowledgeCategory category,
    size_t maxResults) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeEntry> results;
    
    // Find all entries in the specified category
    auto it = entriesByCategory_.find(category);
    if (it != entriesByCategory_.end()) {
        for (const auto& entryId : it->second) {
            auto entryIt = entries_.find(entryId);
            if (entryIt != entries_.end()) {
                results.push_back(entryIt->second);
            }
        }
    }
    
    // Sort by relevance score (highest first)
    std::sort(results.begin(), results.end(),
             [](const auto& a, const auto& b) {
                 return a.relevanceScore > b.relevanceScore;
             });
    
    // Limit results
    if (results.size() > maxResults) {
        results.resize(maxResults);
    }
    
    return results;
}

std::vector<KnowledgeEntry> ProjectKnowledgeBase::findByCustomCategory(
    const std::string& customCategory,
    size_t maxResults) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeEntry> results;
    
    // Find all entries in the specified custom category
    auto it = entriesByCustomCategory_.find(customCategory);
    if (it != entriesByCustomCategory_.end()) {
        for (const auto& entryId : it->second) {
            auto entryIt = entries_.find(entryId);
            if (entryIt != entries_.end()) {
                results.push_back(entryIt->second);
            }
        }
    }
    
    // Sort by relevance score (highest first)
    std::sort(results.begin(), results.end(),
             [](const auto& a, const auto& b) {
                 return a.relevanceScore > b.relevanceScore;
             });
    
    // Limit results
    if (results.size() > maxResults) {
        results.resize(maxResults);
    }
    
    return results;
}

std::vector<KnowledgeEntry> ProjectKnowledgeBase::findByTags(
    const std::vector<std::string>& tags,
    bool matchAll,
    size_t maxResults) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeEntry> results;
    
    if (tags.empty()) {
        return results;
    }
    
    // Create a set of entry IDs that match the tags
    std::set<std::string> matchingEntryIds;
    bool firstTag = true;
    
    for (const auto& tag : tags) {
        std::string lowerTag = toLower(tag);
        
        auto it = entriesByTag_.find(lowerTag);
        if (it == entriesByTag_.end()) {
            // If we need to match all tags and one is missing, return empty results
            if (matchAll) {
                return results;
            }
            continue;
        }
        
        if (firstTag || !matchAll) {
            // For the first tag, initialize the set
            // For OR matching, add all matching entries
            for (const auto& entryId : it->second) {
                matchingEntryIds.insert(entryId);
            }
            firstTag = false;
        } else {
            // For AND matching after the first tag, keep only entries that match all tags
            std::set<std::string> currentTagEntries(it->second.begin(), it->second.end());
            std::set<std::string> intersection;
            
            std::set_intersection(
                matchingEntryIds.begin(), matchingEntryIds.end(),
                currentTagEntries.begin(), currentTagEntries.end(),
                std::inserter(intersection, intersection.begin())
            );
            
            matchingEntryIds = intersection;
        }
        
        // If no entries match all tags so far, return empty results
        if (matchAll && matchingEntryIds.empty()) {
            return results;
        }
    }
    
    // Collect matching entries
    for (const auto& entryId : matchingEntryIds) {
        auto entryIt = entries_.find(entryId);
        if (entryIt != entries_.end()) {
            results.push_back(entryIt->second);
        }
    }
    
    // Sort by relevance score (highest first)
    std::sort(results.begin(), results.end(),
             [](const auto& a, const auto& b) {
                 return a.relevanceScore > b.relevanceScore;
             });
    
    // Limit results
    if (results.size() > maxResults) {
        results.resize(maxResults);
    }
    
    return results;
}

std::vector<KnowledgeEntry> ProjectKnowledgeBase::findRelevantForContext(
    const std::vector<std::string>& contextTerms,
    std::optional<KnowledgeCategory> category,
    size_t maxResults) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeEntry> candidates;
    
    // If a category is specified, only consider entries in that category
    if (category) {
        auto it = entriesByCategory_.find(*category);
        if (it != entriesByCategory_.end()) {
            for (const auto& entryId : it->second) {
                auto entryIt = entries_.find(entryId);
                if (entryIt != entries_.end()) {
                    candidates.push_back(entryIt->second);
                }
            }
        }
    } else {
        // Otherwise, consider all entries
        for (const auto& [id, entry] : entries_) {
            candidates.push_back(entry);
        }
    }
    
    // Calculate relevance for each entry based on context
    std::vector<std::pair<KnowledgeEntry, float>> scoredCandidates;
    for (const auto& entry : candidates) {
        float score = calculateRelevance(entry, "", contextTerms);
        scoredCandidates.push_back({entry, score});
    }
    
    // Sort by relevance (highest first)
    std::sort(scoredCandidates.begin(), scoredCandidates.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second;
             });
    
    // Extract the top results
    std::vector<KnowledgeEntry> results;
    for (size_t i = 0; i < std::min(maxResults, scoredCandidates.size()); i++) {
        results.push_back(scoredCandidates[i].first);
    }
    
    return results;
}

void ProjectKnowledgeBase::registerRelevanceScorer(
    const std::string& name,
    KnowledgeRelevanceScorer scorer) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    relevanceScorers_[name] = scorer;
}

bool ProjectKnowledgeBase::loadFromFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeBase",
                "Failed to open knowledge base file",
                1001
            );
            return false;
        }

        json jsonData;
        file >> jsonData;

        if (!jsonData.is_object() || !jsonData.contains("entries")) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeBase", 
                "Invalid knowledge base file format",
                1002
            );
            return false;
        }

        for (const auto& entry : jsonData["entries"]) {
            try {
                KnowledgeEntry parsedEntry;
                parsedEntry.id = entry["id"];
                parsedEntry.type = entry["type"];
                parsedEntry.title = entry["title"];
                parsedEntry.content = entry["content"];
                
                if (entry.contains("tags")) {
                    parsedEntry.tags = entry["tags"].get<std::vector<std::string>>();
                }
                
                if (entry.contains("metadata")) {
                    parsedEntry.metadata = entry["metadata"];
                }
                
                if (entry.contains("created")) {
                    parsedEntry.created = entry["created"];
                }
                
                if (entry.contains("updated")) {
                    parsedEntry.updated = entry["updated"];
                }
                
                entries_[parsedEntry.id] = std::move(parsedEntry);
            } catch (const std::exception& e) {
                EditorErrorReporter::reportError(
                    "ProjectKnowledgeBase",
                    std::string("Failed to parse entry: ") + e.what(),
                    1003
                );
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "ProjectKnowledgeBase",
            std::string("Error loading knowledge base: ") + e.what(),
            1004
        );
        return false;
    }
}

bool ProjectKnowledgeBase::saveToFile(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Create the directory if it doesn't exist
        std::filesystem::path path(filePath);
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }
        
        // Open the file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeBase",
                "Failed to open file for writing",
                1005
            );
            return false;
        }
        
        json jsonData;
        jsonData["entries"] = json::array();
        
        for (const auto& entry : entries_) {
            json jsonEntry;
            jsonEntry["id"] = entry.second.id;
            jsonEntry["type"] = entry.second.type;
            jsonEntry["title"] = entry.second.title;
            jsonEntry["content"] = entry.second.content;
            
            if (!entry.second.tags.empty()) {
                jsonEntry["tags"] = entry.second.tags;
            }
            
            if (!entry.second.metadata.empty()) {
                json metadataJson = json::object();
                for (const auto& [key, value] : entry.second.metadata) {
                    metadataJson[key] = value;
                }
                jsonEntry["metadata"] = metadataJson;
            }
            
            if (!entry.second.created.empty()) {
                jsonEntry["created"] = entry.second.created;
            }
            
            if (!entry.second.updated.empty()) {
                jsonEntry["updated"] = entry.second.updated;
            }
            
            jsonData["entries"].push_back(jsonEntry);
        }
        
        file << jsonData.dump(4);
        
        return true;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "ProjectKnowledgeBase",
            std::string("Failed to save knowledge base: ") + e.what(),
            1006
        );
        return false;
    }
}

size_t ProjectKnowledgeBase::getEntryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

std::vector<KnowledgeEntry> ProjectKnowledgeBase::getAllEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeEntry> result;
    result.reserve(entries_.size());
    
    for (const auto& [id, entry] : entries_) {
        result.push_back(entry);
    }
    
    return result;
}

void ProjectKnowledgeBase::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    entries_.clear();
    entriesByCategory_.clear();
    entriesByCustomCategory_.clear();
    entriesByTag_.clear();
    
    // Keep the relevance scorers
}

size_t ProjectKnowledgeBase::importEntries(
    const IProjectKnowledgeBase& otherKnowledgeBase,
    bool overwriteExisting) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get all entries from the other knowledge base
    std::vector<KnowledgeEntry> otherEntries = otherKnowledgeBase.getAllEntries();
    
    size_t importedCount = 0;
    
    for (const auto& entry : otherEntries) {
        // Check if entry with this ID already exists
        auto it = entries_.find(entry.id);
        if (it != entries_.end()) {
            if (overwriteExisting) {
                // Update existing entry
                if (updateEntry(entry.id, entry)) {
                    importedCount++;
                }
            }
            // Skip if not overwriting
        } else {
            // Add new entry
            if (addEntry(entry)) {
                importedCount++;
            }
        }
    }
    
    return importedCount;
}

std::vector<KnowledgeCategory> ProjectKnowledgeBase::getAvailableCategories() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeCategory> categories;
    
    for (const auto& [category, entryIds] : entriesByCategory_) {
        if (!entryIds.empty()) {
            categories.push_back(category);
        }
    }
    
    return categories;
}

std::vector<std::string> ProjectKnowledgeBase::getAvailableCustomCategories() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> customCategories;
    
    for (const auto& [category, entryIds] : entriesByCustomCategory_) {
        if (!entryIds.empty()) {
            customCategories.push_back(category);
        }
    }
    
    return customCategories;
}

std::vector<std::string> ProjectKnowledgeBase::getAvailableTags() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> tags;
    
    for (const auto& [tag, entryIds] : entriesByTag_) {
        if (!entryIds.empty()) {
            tags.push_back(tag);
        }
    }
    
    return tags;
}

// ProjectKnowledgeManager implementation
ProjectKnowledgeManager::ProjectKnowledgeManager() {
    // Nothing to initialize
}

std::shared_ptr<IProjectKnowledgeBase> ProjectKnowledgeManager::getKnowledgeBase(
    const std::string& projectPath,
    bool createIfNotExists) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Check if knowledge base is already loaded
    auto it = knowledgeBases_.find(normalizedPath);
    if (it != knowledgeBases_.end()) {
        return it->second;
    }
    
    // Check if knowledge base file exists
    std::string kbPath = getDefaultKnowledgeBasePath(normalizedPath);
    bool exists = std::filesystem::exists(kbPath);
    
    if (exists) {
        // Load existing knowledge base
        auto kb = std::make_shared<ProjectKnowledgeBase>();
        if (kb->loadFromFile(kbPath)) {
            knowledgeBases_[normalizedPath] = kb;
            return kb;
        } else {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeManager",
                "Failed to load knowledge base for project: " + normalizedPath,
                1006
            );
            return nullptr;
        }
    } else if (createIfNotExists) {
        // Create new knowledge base
        auto kb = std::make_shared<ProjectKnowledgeBase>();
        knowledgeBases_[normalizedPath] = kb;
        return kb;
    }
    
    return nullptr;
}

std::shared_ptr<IProjectKnowledgeBase> ProjectKnowledgeManager::createKnowledgeBase(
    const std::string& projectPath,
    bool overwriteExisting) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Check if knowledge base is already loaded
    auto it = knowledgeBases_.find(normalizedPath);
    if (it != knowledgeBases_.end() && !overwriteExisting) {
        EditorErrorReporter::reportWarning(
            "ProjectKnowledgeManager",
            "Knowledge base for project already exists: " + normalizedPath
        );
        return it->second;
    }
    
    // Create new knowledge base
    auto kb = std::make_shared<ProjectKnowledgeBase>();
    knowledgeBases_[normalizedPath] = kb;
    
    return kb;
}

bool ProjectKnowledgeManager::closeKnowledgeBase(
    const std::string& projectPath,
    bool save) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Check if knowledge base is loaded
    auto it = knowledgeBases_.find(normalizedPath);
    if (it == knowledgeBases_.end()) {
        EditorErrorReporter::reportWarning(
            "ProjectKnowledgeManager",
            "No knowledge base loaded for project: " + normalizedPath
        );
        return false;
    }
    
    // Save if requested
    if (save) {
        std::string kbPath = getDefaultKnowledgeBasePath(normalizedPath);
        if (!it->second->saveToFile(kbPath)) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeManager",
                "Failed to save knowledge base for project: " + normalizedPath,
                1007
            );
            return false;
        }
    }
    
    // Remove from loaded knowledge bases
    knowledgeBases_.erase(it);
    
    return true;
}

std::string ProjectKnowledgeManager::getDefaultKnowledgeBasePath(
    const std::string& projectPath) const {
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Use a .kb.json file in the project root
    return normalizedPath + "/.ai-editor-kb.json";
}

bool ProjectKnowledgeManager::knowledgeBaseExists(
    const std::string& projectPath) const {
    
    std::string kbPath = getDefaultKnowledgeBasePath(normalizePath(projectPath));
    return std::filesystem::exists(kbPath);
}

std::shared_ptr<IProjectKnowledgeBase> ProjectKnowledgeManager::importKnowledgeBase(
    const std::string& projectPath,
    const std::string& filePath,
    bool overwriteExisting) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Get or create knowledge base
    auto kb = getKnowledgeBase(normalizedPath, true);
    if (!kb) {
        EditorErrorReporter::reportError(
            "ProjectKnowledgeManager",
            "Failed to create knowledge base for project: " + normalizedPath,
            1008
        );
        return nullptr;
    }
    
    // Create temporary knowledge base to load from file
    auto tempKb = std::make_shared<ProjectKnowledgeBase>();
    if (!tempKb->loadFromFile(filePath)) {
        EditorErrorReporter::reportError(
            "ProjectKnowledgeManager",
            "Failed to load knowledge base from file: " + filePath,
            1009
        );
        return nullptr;
    }
    
    // Clear existing entries if overwriting
    if (overwriteExisting) {
        kb->clear();
    }
    
    // Import entries
    size_t importedCount = kb->importEntries(*tempKb, overwriteExisting);
    
    if (importedCount == 0) {
        EditorErrorReporter::reportWarning(
            "ProjectKnowledgeManager",
            "No entries imported from file: " + filePath
        );
    } else {
        EditorErrorReporter::reportInfo(
            "ProjectKnowledgeManager",
            "Imported " + std::to_string(importedCount) + " entries from file: " + filePath
        );
    }
    
    return kb;
}

bool ProjectKnowledgeManager::exportKnowledgeBase(
    const std::string& projectPath,
    const std::string& filePath) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Check if knowledge base is loaded
    auto it = knowledgeBases_.find(normalizedPath);
    if (it == knowledgeBases_.end()) {
        // Try to load the knowledge base
        std::string kbPath = getDefaultKnowledgeBasePath(normalizedPath);
        if (!std::filesystem::exists(kbPath)) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeManager",
                "No knowledge base found for project: " + normalizedPath,
                1020
            );
            return false;
        }
        
        auto tempKb = std::make_shared<ProjectKnowledgeBase>();
        if (!tempKb->loadFromFile(kbPath)) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeManager",
                "Failed to load knowledge base for project: " + normalizedPath,
                1010
            );
            return false;
        }
        
        // Save to the specified file
        return tempKb->saveToFile(filePath);
    }
    
    // Save to the specified file
    return it->second->saveToFile(filePath);
}

bool ProjectKnowledgeManager::deleteKnowledgeBase(
    const std::string& projectPath) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalizedPath = normalizePath(projectPath);
    
    // Close the knowledge base if it's loaded
    auto it = knowledgeBases_.find(normalizedPath);
    if (it != knowledgeBases_.end()) {
        knowledgeBases_.erase(it);
    }
    
    // Delete the knowledge base file
    std::string kbPath = getDefaultKnowledgeBasePath(normalizedPath);
    if (std::filesystem::exists(kbPath)) {
        try {
            std::filesystem::remove(kbPath);
            return true;
        } catch (const std::exception& e) {
            EditorErrorReporter::reportError(
                "ProjectKnowledgeManager",
                "Failed to delete knowledge base file: " + kbPath,
                1011
            );
            return false;
        }
    }
    
    return true; // No file to delete
}

std::vector<std::string> ProjectKnowledgeManager::getProjectsWithKnowledgeBases() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> projects;
    
    // Add loaded knowledge bases
    for (const auto& [path, kb] : knowledgeBases_) {
        projects.push_back(path);
    }
    
    return projects;
}

std::string ProjectKnowledgeManager::normalizePath(const std::string& projectPath) const {
    // Normalize path separators and remove trailing slash
    std::string normalized = projectPath;
    
    // Replace backslashes with forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Remove trailing slash if present
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    return normalized;
}

std::vector<KnowledgeEntry> ProjectKnowledgeBase::query(const KnowledgeQuery& query) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KnowledgeEntry> results;
    
    // Collect all entries that match the criteria
    for (const auto& pair : entries_) {
        const auto& entry = pair.second;
        bool matchesFilters = true;
        
        // Filter by category if specified
        if (query.category.has_value() && entry.category != query.category.value()) {
            matchesFilters = false;
        }
        
        // Filter by custom category if specified
        if (!query.customCategory.empty() && entry.customCategory != query.customCategory) {
            matchesFilters = false;
        }
        
        // Filter by tags if specified
        if (!query.tags.empty()) {
            bool tagsMatch = false;
            for (const auto& tag : query.tags) {
                auto it = std::find(entry.tags.begin(), entry.tags.end(), tag);
                if (it != entry.tags.end()) {
                    tagsMatch = true;
                    break;
                }
            }
            if (!tagsMatch) {
                matchesFilters = false;
            }
        }
        
        // Filter by search text if specified
        if (!query.searchText.empty()) {
            bool textFound = false;
            
            // Check title
            if (entry.title.find(query.searchText) != std::string::npos) {
                textFound = true;
            }
            
            // Check content
            if (!textFound && entry.content.find(query.searchText) != std::string::npos) {
                textFound = true;
            }
            
            if (!textFound) {
                matchesFilters = false;
            }
        }
        
        if (matchesFilters) {
            results.push_back(entry);
        }
    }
    
    // Sort by relevance
    std::sort(results.begin(), results.end(), [](const KnowledgeEntry& a, const KnowledgeEntry& b) {
        return a.relevanceScore > b.relevanceScore;
    });
    
    // Limit results
    if (results.size() > query.maxResults) {
        results.resize(query.maxResults);
    }
    
    return results;
}

// Method overload for simple string query
std::vector<KnowledgeEntry> ProjectKnowledgeBase::query(const std::string& queryText, size_t maxResults) const {
    KnowledgeQuery query;
    query.searchText = queryText;
    query.maxResults = maxResults;
    return this->query(query);
}

// Utility method for converting strings to lowercase
std::string ProjectKnowledgeBase::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Utility functions for JSON serialization
std::string knowledgeCategoryToString(KnowledgeCategory category) {
    switch (category) {
        case KnowledgeCategory::ARCHITECTURE: return "ARCHITECTURE";
        case KnowledgeCategory::CODING_STANDARDS: return "CODING_STANDARDS";
        case KnowledgeCategory::TERMINOLOGY: return "TERMINOLOGY";
        case KnowledgeCategory::API_USAGE: return "API_USAGE";
        case KnowledgeCategory::PATTERNS: return "PATTERNS";
        case KnowledgeCategory::DOCUMENTATION: return "DOCUMENTATION";
        case KnowledgeCategory::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

KnowledgeCategory stringToKnowledgeCategory(const std::string& categoryStr) {
    if (categoryStr == "ARCHITECTURE") return KnowledgeCategory::ARCHITECTURE;
    if (categoryStr == "CODING_STANDARDS") return KnowledgeCategory::CODING_STANDARDS;
    if (categoryStr == "TERMINOLOGY") return KnowledgeCategory::TERMINOLOGY;
    if (categoryStr == "API_USAGE") return KnowledgeCategory::API_USAGE;
    if (categoryStr == "PATTERNS") return KnowledgeCategory::PATTERNS;
    if (categoryStr == "DOCUMENTATION") return KnowledgeCategory::DOCUMENTATION;
    if (categoryStr == "CUSTOM") return KnowledgeCategory::CUSTOM;
    
    // Default
    return KnowledgeCategory::CUSTOM;
}

nlohmann::json serializeKnowledgeEntry(const KnowledgeEntry& entry) {
    nlohmann::json json;
    json["id"] = entry.id;
    json["title"] = entry.title;
    json["content"] = entry.content;
    json["category"] = knowledgeCategoryToString(entry.category);
    
    if (!entry.tags.empty()) {
        json["tags"] = entry.tags;
    }
    
    if (!entry.customCategory.empty()) {
        json["customCategory"] = entry.customCategory;
    }
    
    json["relevanceScore"] = entry.relevanceScore;
    
    if (!entry.metadata.empty()) {
        nlohmann::json metadataJson = nlohmann::json::object();
        for (const auto& [key, value] : entry.metadata) {
            metadataJson[key] = value;
        }
        json["metadata"] = metadataJson;
    }
    
    if (!entry.created.empty()) {
        json["created"] = entry.created;
    }
    
    if (!entry.updated.empty()) {
        json["updated"] = entry.updated;
    }
    
    return json;
}

KnowledgeEntry deserializeKnowledgeEntry(const nlohmann::json& json) {
    KnowledgeEntry entry;
    
    if (json.contains("id")) {
        entry.id = json["id"];
    }
    
    if (json.contains("title")) {
        entry.title = json["title"];
    }
    
    if (json.contains("content")) {
        entry.content = json["content"];
    }
    
    if (json.contains("category")) {
        entry.category = stringToKnowledgeCategory(json["category"]);
    }
    
    if (json.contains("tags") && json["tags"].is_array()) {
        entry.tags.clear();
        for (const auto& tag : json["tags"]) {
            entry.tags.push_back(tag);
        }
    }
    
    if (json.contains("customCategory")) {
        entry.customCategory = json["customCategory"];
    }
    
    if (json.contains("relevanceScore")) {
        entry.relevanceScore = json["relevanceScore"];
    }
    
    if (json.contains("metadata") && json["metadata"].is_object()) {
        entry.metadata.clear();
        for (auto it = json["metadata"].begin(); it != json["metadata"].end(); ++it) {
            entry.metadata[it.key()] = it.value();
        }
    }
    
    if (json.contains("created")) {
        entry.created = json["created"];
    }
    
    if (json.contains("updated")) {
        entry.updated = json["updated"];
    }
    
    return entry;
}

bool ProjectKnowledgeBase::addEntry(const KnowledgeEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if entry with this ID already exists
    if (entries_.find(entry.id) != entries_.end()) {
        return false; // Entry with this ID already exists
    }
    
    // Add entry to the main map
    entries_[entry.id] = entry;
    
    // Add entry to category indices
    entriesByCategory_[entry.category].push_back(entry.id);
    
    // Add entry to custom category index if applicable
    if (!entry.customCategory.empty()) {
        entriesByCustomCategory_[entry.customCategory].push_back(entry.id);
    }
    
    // Add entry to tag indices
    for (const auto& tag : entry.tags) {
        entriesByTag_[tag].push_back(entry.id);
    }
    
    return true;
}

bool ProjectKnowledgeBase::updateEntry(const std::string& entryId, const KnowledgeEntry& updatedEntry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if entry exists
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false; // Entry not found
    }
    
    // Get the old entry
    const KnowledgeEntry& oldEntry = it->second;
    
    // Remove entry from category indices if category changed
    if (oldEntry.category != updatedEntry.category) {
        auto& categoryEntries = entriesByCategory_[oldEntry.category];
        categoryEntries.erase(std::remove(categoryEntries.begin(), categoryEntries.end(), entryId), categoryEntries.end());
        entriesByCategory_[updatedEntry.category].push_back(entryId);
    }
    
    // Update custom category indices if needed
    if (oldEntry.customCategory != updatedEntry.customCategory) {
        if (!oldEntry.customCategory.empty()) {
            auto& customCategoryEntries = entriesByCustomCategory_[oldEntry.customCategory];
            customCategoryEntries.erase(std::remove(customCategoryEntries.begin(), customCategoryEntries.end(), entryId), customCategoryEntries.end());
        }
        
        if (!updatedEntry.customCategory.empty()) {
            entriesByCustomCategory_[updatedEntry.customCategory].push_back(entryId);
        }
    }
    
    // Update tag indices
    // Remove from old tags
    for (const auto& tag : oldEntry.tags) {
        auto& tagEntries = entriesByTag_[tag];
        tagEntries.erase(std::remove(tagEntries.begin(), tagEntries.end(), entryId), tagEntries.end());
    }
    
    // Add to new tags
    for (const auto& tag : updatedEntry.tags) {
        entriesByTag_[tag].push_back(entryId);
    }
    
    // Update the entry in the main map
    // Use the original ID, but with updated content
    KnowledgeEntry newEntry = updatedEntry;
    newEntry.id = entryId; // Ensure the ID doesn't change
    entries_[entryId] = newEntry;
    
    return true;
}

bool ProjectKnowledgeBase::removeEntry(const std::string& entryId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if entry exists
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false; // Entry not found
    }
    
    const KnowledgeEntry& entry = it->second;
    
    // Remove from category indices
    auto& categoryEntries = entriesByCategory_[entry.category];
    categoryEntries.erase(std::remove(categoryEntries.begin(), categoryEntries.end(), entryId), categoryEntries.end());
    
    // Remove from custom category indices if applicable
    if (!entry.customCategory.empty()) {
        auto& customCategoryEntries = entriesByCustomCategory_[entry.customCategory];
        customCategoryEntries.erase(std::remove(customCategoryEntries.begin(), customCategoryEntries.end(), entryId), customCategoryEntries.end());
    }
    
    // Remove from tag indices
    for (const auto& tag : entry.tags) {
        auto& tagEntries = entriesByTag_[tag];
        tagEntries.erase(std::remove(tagEntries.begin(), tagEntries.end(), entryId), tagEntries.end());
    }
    
    // Remove the entry from the main map
    entries_.erase(entryId);
    
    return true;
}

std::optional<KnowledgeEntry> ProjectKnowledgeBase::getEntry(const std::string& entryId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entries_.find(entryId);
    if (it != entries_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

} // namespace ai_editor 