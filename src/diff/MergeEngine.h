#pragma once

#include "interfaces/IMergeEngine.hpp"
#include "AppDebugLog.h"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility>

/**
 * @class MergeEngine
 * @brief Implementation of the merge engine
 * 
 * This class implements a three-way merge algorithm using a diff engine
 * to compute differences between texts. It can handle conflicts and
 * provides methods to resolve them.
 */
class MergeEngine : public IMergeEngine {
public:
    /**
     * @brief Constructor
     * 
     * @param diffEngine Diff engine to use for computing differences
     */
    explicit MergeEngine(IDiffEnginePtr diffEngine)
        : diffEngine_(std::move(diffEngine)) {
        LOG_DEBUG("MergeEngine created");
    }
    
    /**
     * @brief Destructor
     */
    ~MergeEngine() override = default;
    
    /**
     * @brief Set the diff engine to use for computing differences
     * 
     * @param diffEngine The diff engine to use
     */
    void setDiffEngine(IDiffEnginePtr diffEngine) override {
        diffEngine_ = std::move(diffEngine);
    }
    
    /**
     * @brief Get the diff engine used for computing differences
     * 
     * @return The diff engine
     */
    IDiffEnginePtr getDiffEngine() const override {
        return diffEngine_;
    }
    
    /**
     * @brief Perform a three-way merge
     * 
     * @param base Base version (common ancestor)
     * @param ours Our version
     * @param theirs Their version
     * @return Merge result with the merged text and any conflicts
     */
    MergeResult merge(
        const std::vector<std::string>& base,
        const std::vector<std::string>& ours,
        const std::vector<std::string>& theirs) override {
        
        if (!diffEngine_) {
            LOG_ERROR("Diff engine not set");
            return {};
        }
        
        // Compute differences
        auto baseToDiff = diffEngine_->computeLineDiff(base, ours);
        auto baseToTheirs = diffEngine_->computeLineDiff(base, theirs);
        
        MergeResult result;
        
        // Start with base text
        result.mergedLines = base;
        
        // Apply non-conflicting changes
        std::vector<std::pair<size_t, size_t>> ourRanges;
        std::vector<std::pair<size_t, size_t>> theirRanges;
        
        // Collect ranges modified in our version
        for (const auto& change : baseToDiff) {
            if (!change.isEqual()) {
                ourRanges.emplace_back(change.startLine1, change.startLine1 + change.lineCount1);
            }
        }
        
        // Collect ranges modified in their version
        for (const auto& change : baseToTheirs) {
            if (!change.isEqual()) {
                theirRanges.emplace_back(change.startLine1, change.startLine1 + change.lineCount1);
            }
        }
        
        // Find overlapping ranges (conflicts)
        std::vector<std::pair<size_t, size_t>> conflictRanges;
        
        for (const auto& ourRange : ourRanges) {
            for (const auto& theirRange : theirRanges) {
                // Check if ranges overlap
                if (!(ourRange.second <= theirRange.first || theirRange.second <= ourRange.first)) {
                    // Merge overlapping ranges
                    size_t start = std::min(ourRange.first, theirRange.first);
                    size_t end = std::max(ourRange.second, theirRange.second);
                    
                    // Check if this range overlaps with an existing conflict range
                    bool merged = false;
                    
                    for (auto& conflictRange : conflictRanges) {
                        if (!(end <= conflictRange.first || conflictRange.second <= start)) {
                            // Merge with existing conflict range
                            conflictRange.first = std::min(conflictRange.first, start);
                            conflictRange.second = std::max(conflictRange.second, end);
                            merged = true;
                            break;
                        }
                    }
                    
                    if (!merged) {
                        conflictRanges.emplace_back(start, end);
                    }
                }
            }
        }
        
        // Sort conflict ranges
        std::sort(conflictRanges.begin(), conflictRanges.end());
        
        // Apply changes and mark conflicts
        std::vector<MergeConflict> conflicts;
        
        // Process conflicts first
        for (const auto& [start, end] : conflictRanges) {
            // Create a conflict
            MergeConflict conflict;
            conflict.startLine = start;
            conflict.lineCount = end - start;
            
            // Get base lines for this range
            conflict.baseLines.assign(base.begin() + start, base.begin() + end);
            
            // Get our lines for this range
            // Find the corresponding range in our version
            std::vector<std::string> ourLines;
            
            for (const auto& change : baseToDiff) {
                if (!change.isEqual() && 
                    change.startLine1 < end && 
                    change.startLine1 + change.lineCount1 > start) {
                    
                    // Apply this change to get our lines
                    if (change.isDelete()) {
                        // Lines were deleted in our version
                        // (nothing to add)
                    } else if (change.isInsert()) {
                        // Lines were inserted in our version
                        ourLines.insert(
                            ourLines.end(),
                            ours.begin() + change.startLine2,
                            ours.begin() + change.startLine2 + change.lineCount2
                        );
                    } else if (change.isReplace()) {
                        // Lines were replaced in our version
                        ourLines.insert(
                            ourLines.end(),
                            ours.begin() + change.startLine2,
                            ours.begin() + change.startLine2 + change.lineCount2
                        );
                    }
                }
            }
            
            conflict.ourLines = ourLines;
            
            // Get their lines for this range
            // Find the corresponding range in their version
            std::vector<std::string> theirLines;
            
            for (const auto& change : baseToTheirs) {
                if (!change.isEqual() && 
                    change.startLine1 < end && 
                    change.startLine1 + change.lineCount1 > start) {
                    
                    // Apply this change to get their lines
                    if (change.isDelete()) {
                        // Lines were deleted in their version
                        // (nothing to add)
                    } else if (change.isInsert()) {
                        // Lines were inserted in their version
                        theirLines.insert(
                            theirLines.end(),
                            theirs.begin() + change.startLine2,
                            theirs.begin() + change.startLine2 + change.lineCount2
                        );
                    } else if (change.isReplace()) {
                        // Lines were replaced in their version
                        theirLines.insert(
                            theirLines.end(),
                            theirs.begin() + change.startLine2,
                            theirs.begin() + change.startLine2 + change.lineCount2
                        );
                    }
                }
            }
            
            conflict.theirLines = theirLines;
            
            // Add conflict to list
            conflicts.push_back(conflict);
        }
        
        // Update result with conflicts
        result.conflicts = conflicts;
        result.hasConflicts = !conflicts.empty();
        
        // Apply non-conflicting changes
        // First apply our changes
        for (const auto& change : baseToDiff) {
            if (change.isEqual()) {
                continue; // No change
            }
            
            // Check if this change overlaps with any conflict
            bool isConflict = false;
            
            for (const auto& [start, end] : conflictRanges) {
                if (change.startLine1 < end && change.startLine1 + change.lineCount1 > start) {
                    isConflict = true;
                    break;
                }
            }
            
            if (isConflict) {
                continue; // Skip conflicting changes
            }
            
            // Apply this change
            if (change.isDelete()) {
                // Delete lines
                result.mergedLines.erase(
                    result.mergedLines.begin() + change.startLine1,
                    result.mergedLines.begin() + change.startLine1 + change.lineCount1
                );
            } else if (change.isInsert()) {
                // Insert lines
                result.mergedLines.insert(
                    result.mergedLines.begin() + change.startLine1,
                    ours.begin() + change.startLine2,
                    ours.begin() + change.startLine2 + change.lineCount2
                );
            } else if (change.isReplace()) {
                // Replace lines
                result.mergedLines.erase(
                    result.mergedLines.begin() + change.startLine1,
                    result.mergedLines.begin() + change.startLine1 + change.lineCount1
                );
                result.mergedLines.insert(
                    result.mergedLines.begin() + change.startLine1,
                    ours.begin() + change.startLine2,
                    ours.begin() + change.startLine2 + change.lineCount2
                );
            }
        }
        
        // Then apply their changes
        for (const auto& change : baseToTheirs) {
            if (change.isEqual()) {
                continue; // No change
            }
            
            // Check if this change overlaps with any conflict
            bool isConflict = false;
            
            for (const auto& [start, end] : conflictRanges) {
                if (change.startLine1 < end && change.startLine1 + change.lineCount1 > start) {
                    isConflict = true;
                    break;
                }
            }
            
            if (isConflict) {
                continue; // Skip conflicting changes
            }
            
            // Apply this change
            if (change.isDelete()) {
                // Delete lines
                result.mergedLines.erase(
                    result.mergedLines.begin() + change.startLine1,
                    result.mergedLines.begin() + change.startLine1 + change.lineCount1
                );
            } else if (change.isInsert()) {
                // Insert lines
                result.mergedLines.insert(
                    result.mergedLines.begin() + change.startLine1,
                    theirs.begin() + change.startLine2,
                    theirs.begin() + change.startLine2 + change.lineCount2
                );
            } else if (change.isReplace()) {
                // Replace lines
                result.mergedLines.erase(
                    result.mergedLines.begin() + change.startLine1,
                    result.mergedLines.begin() + change.startLine1 + change.lineCount1
                );
                result.mergedLines.insert(
                    result.mergedLines.begin() + change.startLine1,
                    theirs.begin() + change.startLine2,
                    theirs.begin() + change.startLine2 + change.lineCount2
                );
            }
        }
        
        // Insert conflict markers for each conflict
        size_t offset = 0;
        
        for (auto& conflict : result.conflicts) {
            // Update conflict start line due to previous insertions
            conflict.startLine += offset;
            
            // Format conflict with markers
            auto markedConflict = formatConflict(conflict);
            
            // Insert conflict markers
            result.mergedLines.erase(
                result.mergedLines.begin() + conflict.startLine,
                result.mergedLines.begin() + conflict.startLine + conflict.lineCount
            );
            result.mergedLines.insert(
                result.mergedLines.begin() + conflict.startLine,
                markedConflict.begin(),
                markedConflict.end()
            );
            
            // Update offset
            offset += markedConflict.size() - conflict.lineCount;
            
            // Update conflict line count
            conflict.lineCount = markedConflict.size();
        }
        
        return result;
    }
    
    /**
     * @brief Resolve a merge conflict
     * 
     * @param mergeResult The merge result to modify
     * @param conflictIndex Index of the conflict to resolve
     * @param resolution Resolution strategy to apply
     * @param customResolution Custom resolution lines (used if resolution is CUSTOM)
     * @return True if the conflict was resolved successfully
     */
    bool resolveConflict(
        MergeResult& mergeResult,
        size_t conflictIndex,
        MergeConflictResolution resolution,
        const std::vector<std::string>& customResolution = {}) override {
        
        if (conflictIndex >= mergeResult.conflicts.size()) {
            LOG_ERROR("Invalid conflict index: " + std::to_string(conflictIndex));
            return false;
        }
        
        auto& conflict = mergeResult.conflicts[conflictIndex];
        
        // Store resolution
        conflict.resolution = resolution;
        
        if (resolution == MergeConflictResolution::CUSTOM) {
            conflict.customResolution = customResolution;
        }
        
        return true;
    }
    
    /**
     * @brief Apply all conflict resolutions in a merge result
     * 
     * @param mergeResult The merge result to modify
     * @return True if all conflicts were resolved successfully
     */
    bool applyResolutions(MergeResult& mergeResult) override {
        if (mergeResult.conflicts.empty()) {
            return true; // No conflicts to resolve
        }
        
        // Sort conflicts by start line, in reverse order
        std::sort(mergeResult.conflicts.begin(), mergeResult.conflicts.end(),
            [](const MergeConflict& a, const MergeConflict& b) {
                return a.startLine > b.startLine;
            });
        
        // Apply resolutions
        for (const auto& conflict : mergeResult.conflicts) {
            // Get the lines to replace the conflict with
            std::vector<std::string> resolutionLines;
            
            switch (conflict.resolution) {
                case MergeConflictResolution::TAKE_BASE:
                    resolutionLines = conflict.baseLines;
                    break;
                case MergeConflictResolution::TAKE_OURS:
                    resolutionLines = conflict.ourLines;
                    break;
                case MergeConflictResolution::TAKE_THEIRS:
                    resolutionLines = conflict.theirLines;
                    break;
                case MergeConflictResolution::TAKE_BOTH:
                    resolutionLines = conflict.ourLines;
                    resolutionLines.insert(
                        resolutionLines.end(),
                        conflict.theirLines.begin(),
                        conflict.theirLines.end()
                    );
                    break;
                case MergeConflictResolution::TAKE_BOTH_REVERSE:
                    resolutionLines = conflict.theirLines;
                    resolutionLines.insert(
                        resolutionLines.end(),
                        conflict.ourLines.begin(),
                        conflict.ourLines.end()
                    );
                    break;
                case MergeConflictResolution::CUSTOM:
                    resolutionLines = conflict.customResolution;
                    break;
                default:
                    LOG_ERROR("Unknown conflict resolution strategy");
                    return false;
            }
            
            // Replace conflict with resolution
            mergeResult.mergedLines.erase(
                mergeResult.mergedLines.begin() + conflict.startLine,
                mergeResult.mergedLines.begin() + conflict.startLine + conflict.lineCount
            );
            mergeResult.mergedLines.insert(
                mergeResult.mergedLines.begin() + conflict.startLine,
                resolutionLines.begin(),
                resolutionLines.end()
            );
        }
        
        // Clear conflicts
        mergeResult.conflicts.clear();
        mergeResult.hasConflicts = false;
        
        return true;
    }
    
    /**
     * @brief Format a merge conflict with markers
     * 
     * @param conflict The conflict to format
     * @return Formatted conflict with markers as a vector of lines
     */
    std::vector<std::string> formatConflict(const MergeConflict& conflict) override {
        std::vector<std::string> result;
        
        // Add conflict header
        result.push_back("<<<<<<<");
        
        // Add our lines
        result.insert(result.end(), conflict.ourLines.begin(), conflict.ourLines.end());
        
        // Add separator
        result.push_back("=======");
        
        // Add their lines
        result.insert(result.end(), conflict.theirLines.begin(), conflict.theirLines.end());
        
        // Add conflict footer
        result.push_back(">>>>>>>");
        
        return result;
    }
    
private:
    IDiffEnginePtr diffEngine_;
}; 