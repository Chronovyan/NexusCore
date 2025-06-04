#pragma once

#include "IDiffEngine.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

/**
 * @enum MergeConflictResolution
 * @brief Enumeration of possible conflict resolution strategies
 */
enum class MergeConflictResolution {
    TAKE_BASE,      // Use the base version (original)
    TAKE_OURS,      // Use our version
    TAKE_THEIRS,    // Use their version
    TAKE_BOTH,      // Use both versions (ours then theirs)
    TAKE_BOTH_REVERSE, // Use both versions (theirs then ours)
    CUSTOM          // Use a custom resolution
};

/**
 * @struct MergeConflict
 * @brief Represents a conflict during a merge operation
 */
struct MergeConflict {
    size_t startLine;          // Start line in the merged result (0-based)
    size_t lineCount;          // Number of lines in the conflict
    
    std::vector<std::string> baseLines;   // Lines from base version
    std::vector<std::string> ourLines;    // Lines from our version
    std::vector<std::string> theirLines;  // Lines from their version
    
    MergeConflictResolution resolution = MergeConflictResolution::TAKE_BASE;
    std::vector<std::string> customResolution; // Used if resolution is CUSTOM
};

/**
 * @struct MergeResult
 * @brief Result of a merge operation
 */
struct MergeResult {
    std::vector<std::string> mergedLines;    // The merged text lines
    std::vector<MergeConflict> conflicts;    // List of conflicts
    bool hasConflicts = false;               // Whether the merge has conflicts
};

/**
 * @interface IMergeEngine
 * @brief Interface for merge engine components
 * 
 * This interface defines the contract for merge engine implementations,
 * providing methods to merge different versions of text.
 */
class IMergeEngine {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IMergeEngine() = default;
    
    /**
     * @brief Set the diff engine to use for computing differences
     * 
     * @param diffEngine The diff engine to use
     */
    virtual void setDiffEngine(IDiffEnginePtr diffEngine) = 0;
    
    /**
     * @brief Get the diff engine used for computing differences
     * 
     * @return The diff engine
     */
    virtual IDiffEnginePtr getDiffEngine() const = 0;
    
    /**
     * @brief Perform a three-way merge
     * 
     * @param base Base version (common ancestor)
     * @param ours Our version
     * @param theirs Their version
     * @return Merge result with the merged text and any conflicts
     */
    virtual MergeResult merge(
        const std::vector<std::string>& base,
        const std::vector<std::string>& ours,
        const std::vector<std::string>& theirs) = 0;
    
    /**
     * @brief Resolve a merge conflict
     * 
     * @param mergeResult The merge result to modify
     * @param conflictIndex Index of the conflict to resolve
     * @param resolution Resolution strategy to apply
     * @param customResolution Custom resolution lines (used if resolution is CUSTOM)
     * @return True if the conflict was resolved successfully
     */
    virtual bool resolveConflict(
        MergeResult& mergeResult,
        size_t conflictIndex,
        MergeConflictResolution resolution,
        const std::vector<std::string>& customResolution = {}) = 0;
    
    /**
     * @brief Apply all conflict resolutions in a merge result
     * 
     * @param mergeResult The merge result to modify
     * @return True if all conflicts were resolved successfully
     */
    virtual bool applyResolutions(MergeResult& mergeResult) = 0;
    
    /**
     * @brief Format a merge conflict with markers
     * 
     * @param conflict The conflict to format
     * @return Formatted conflict with markers as a vector of lines
     */
    virtual std::vector<std::string> formatConflict(const MergeConflict& conflict) = 0;
};

// Type alias for smart pointer to IMergeEngine
using IMergeEnginePtr = std::shared_ptr<IMergeEngine>; 