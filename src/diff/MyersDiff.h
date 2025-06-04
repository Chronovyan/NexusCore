#pragma once

#include "interfaces/IDiffEngine.hpp"
#include "AppDebugLog.h"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>

/**
 * @class MyersDiff
 * @brief Implementation of the Myers diff algorithm
 * 
 * This class implements the Myers diff algorithm, which is an efficient algorithm
 * for computing the shortest edit script between two sequences.
 * 
 * Reference: "An O(ND) Difference Algorithm and Its Variations" by Eugene W. Myers
 */
class MyersDiff : public IDiffEngine {
public:
    /**
     * @brief Constructor
     */
    MyersDiff() {
        LOG_DEBUG("MyersDiff created");
    }
    
    /**
     * @brief Destructor
     */
    ~MyersDiff() override = default;
    
    /**
     * @brief Compute line-level differences between two texts
     * 
     * @param text1 First text (lines)
     * @param text2 Second text (lines)
     * @return Vector of line-level diff changes
     */
    std::vector<DiffChange> computeLineDiff(
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2) override {
        
        // Use Myers algorithm to compute the shortest edit script
        auto script = computeEditScript(text1, text2);
        
        // Convert edit script to diff changes
        return convertScriptToChanges(script, text1, text2, true);
    }
    
    /**
     * @brief Compute character-level differences between two texts
     * 
     * @param text1 First text (lines)
     * @param text2 Second text (lines)
     * @param charLevelForEqualLines Whether to compute character-level diffs for equal lines
     * @return Vector of line and character level diff changes
     */
    std::vector<DiffChange> computeCharacterDiff(
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2,
        bool charLevelForEqualLines = false) override {
        
        // First compute line-level differences
        auto lineDiff = computeLineDiff(text1, text2);
        
        // Then compute character-level differences for each line
        std::vector<DiffChange> result;
        
        for (const auto& change : lineDiff) {
            if (change.isEqual()) {
                if (charLevelForEqualLines) {
                    // For equal lines, compute character-level diff if requested
                    for (size_t i = 0; i < change.lineCount1; ++i) {
                        size_t line1 = change.startLine1 + i;
                        size_t line2 = change.startLine2 + i;
                        
                        auto charDiff = computeStringDiff(text1[line1], text2[line2]);
                        
                        // Adjust line numbers in character diff
                        for (auto& charChange : charDiff) {
                            charChange.startLine1 = line1;
                            charChange.startLine2 = line2;
                        }
                        
                        result.insert(result.end(), charDiff.begin(), charDiff.end());
                    }
                } else {
                    // Otherwise, just add the line-level change
                    result.push_back(change);
                }
            } else if (change.isReplace() && change.lineCount1 == 1 && change.lineCount2 == 1) {
                // For single-line replacements, compute character-level diff
                auto charDiff = computeStringDiff(
                    text1[change.startLine1], 
                    text2[change.startLine2]
                );
                
                // Adjust line numbers in character diff
                for (auto& charChange : charDiff) {
                    charChange.startLine1 = change.startLine1;
                    charChange.startLine2 = change.startLine2;
                }
                
                result.insert(result.end(), charDiff.begin(), charDiff.end());
            } else {
                // For other changes, just add the line-level change
                result.push_back(change);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Compute differences between two strings
     * 
     * @param str1 First string
     * @param str2 Second string
     * @return Vector of character-level diff changes
     */
    std::vector<DiffChange> computeStringDiff(
        const std::string& str1, 
        const std::string& str2) override {
        
        // Convert strings to vectors of characters
        std::vector<char> chars1(str1.begin(), str1.end());
        std::vector<char> chars2(str2.begin(), str2.end());
        
        // Use Myers algorithm to compute the shortest edit script
        auto script = computeEditScript(chars1, chars2);
        
        // Convert edit script to diff changes (character level)
        auto changes = convertScriptToChanges(script, chars1, chars2, false);
        
        // Set line information for character-level changes
        for (auto& change : changes) {
            change.startLine1 = 0;
            change.lineCount1 = 1;
            change.startLine2 = 0;
            change.lineCount2 = 1;
            change.isLineLevel = false;
        }
        
        return changes;
    }
    
    /**
     * @brief Format changes as a unified diff
     * 
     * @param changes Vector of diff changes
     * @param text1 First text (lines)
     * @param text2 Second text (lines)
     * @param contextLines Number of context lines to include
     * @return Formatted unified diff as a string
     */
    std::string formatUnifiedDiff(
        const std::vector<DiffChange>& changes,
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2,
        size_t contextLines = 3) override {
        
        if (changes.empty()) {
            return ""; // No changes
        }
        
        std::string result;
        
        // Group changes that are close together
        std::vector<std::pair<size_t, size_t>> hunks; // start, end indices in changes
        
        size_t startIdx = 0;
        size_t endIdx = 0;
        
        for (size_t i = 1; i < changes.size(); ++i) {
            const auto& prevChange = changes[endIdx];
            const auto& currChange = changes[i];
            
            // If current change is far from previous change, start a new hunk
            bool farApart = false;
            
            if (prevChange.isLineLevel && currChange.isLineLevel) {
                size_t prevEnd1 = prevChange.startLine1 + prevChange.lineCount1;
                size_t currStart1 = currChange.startLine1;
                
                farApart = (currStart1 > prevEnd1 + contextLines * 2);
            }
            
            if (farApart) {
                hunks.emplace_back(startIdx, endIdx);
                startIdx = i;
            }
            
            endIdx = i;
        }
        
        // Add the last hunk
        hunks.emplace_back(startIdx, endIdx);
        
        // Format each hunk
        for (const auto& [hunkStart, hunkEnd] : hunks) {
            const auto& firstChange = changes[hunkStart];
            const auto& lastChange = changes[hunkEnd];
            
            // Determine hunk range
            size_t hunkStartLine1 = firstChange.startLine1;
            size_t hunkEndLine1 = lastChange.startLine1 + lastChange.lineCount1;
            
            size_t hunkStartLine2 = firstChange.startLine2;
            size_t hunkEndLine2 = lastChange.startLine2 + lastChange.lineCount2;
            
            // Adjust for context lines
            hunkStartLine1 = (hunkStartLine1 >= contextLines) ? hunkStartLine1 - contextLines : 0;
            hunkEndLine1 = std::min(hunkEndLine1 + contextLines, text1.size());
            
            hunkStartLine2 = (hunkStartLine2 >= contextLines) ? hunkStartLine2 - contextLines : 0;
            hunkEndLine2 = std::min(hunkEndLine2 + contextLines, text2.size());
            
            // Write hunk header
            result += "@@ -" + std::to_string(hunkStartLine1 + 1) + "," 
                   + std::to_string(hunkEndLine1 - hunkStartLine1) + " +" 
                   + std::to_string(hunkStartLine2 + 1) + "," 
                   + std::to_string(hunkEndLine2 - hunkStartLine2) + " @@\n";
            
            // Write hunk content
            size_t line1 = hunkStartLine1;
            size_t line2 = hunkStartLine2;
            
            while (line1 < hunkEndLine1 || line2 < hunkEndLine2) {
                // Find the change that contains this line
                bool foundChange = false;
                
                for (size_t i = hunkStart; i <= hunkEnd; ++i) {
                    const auto& change = changes[i];
                    
                    if (!change.isLineLevel) {
                        continue; // Skip character-level changes
                    }
                    
                    // Check if this change contains the current line
                    if (line1 >= change.startLine1 && line1 < change.startLine1 + change.lineCount1) {
                        foundChange = true;
                        
                        if (change.isEqual()) {
                            // Equal lines
                            result += " " + text1[line1] + "\n";
                            ++line1;
                            ++line2;
                        } else if (change.isDelete()) {
                            // Deleted lines
                            result += "-" + text1[line1] + "\n";
                            ++line1;
                        } else if (change.isInsert()) {
                            // Inserted lines
                            result += "+" + text2[line2] + "\n";
                            ++line2;
                        } else if (change.isReplace()) {
                            // Replaced lines - show as delete then insert
                            if (line1 < change.startLine1 + change.lineCount1) {
                                result += "-" + text1[line1] + "\n";
                                ++line1;
                            } else if (line2 < change.startLine2 + change.lineCount2) {
                                result += "+" + text2[line2] + "\n";
                                ++line2;
                            }
                        }
                        
                        break;
                    }
                }
                
                if (!foundChange) {
                    // Context line
                    if (line1 < text1.size() && line1 < hunkEndLine1) {
                        result += " " + text1[line1] + "\n";
                        ++line1;
                        ++line2;
                    } else {
                        break;
                    }
                }
            }
        }
        
        return result;
    }
    
private:
    /**
     * @brief EditOp enumeration for edit script operations
     */
    enum class EditOp {
        KEEP,    // Keep the element (no change)
        INSERT,  // Insert an element
        DELETE   // Delete an element
    };
    
    /**
     * @brief EditScriptItem struct for items in an edit script
     */
    struct EditScriptItem {
        EditOp op;         // Operation
        size_t idx1;       // Index in sequence 1
        size_t idx2;       // Index in sequence 2
    };
    
    /**
     * @brief Point struct for the Myers algorithm
     */
    struct Point {
        int x;
        int y;
    };
    
    /**
     * @brief Compute the shortest edit script between two sequences using Myers algorithm
     * 
     * @tparam T Type of sequence elements
     * @param seq1 First sequence
     * @param seq2 Second sequence
     * @return Vector of edit script items
     */
    template<typename T>
    std::vector<EditScriptItem> computeEditScript(
        const std::vector<T>& seq1,
        const std::vector<T>& seq2) {
        
        int n = static_cast<int>(seq1.size());
        int m = static_cast<int>(seq2.size());
        
        std::vector<EditScriptItem> script;
        
        // Handle special cases
        if (n == 0 && m == 0) {
            return script;
        }
        
        if (n == 0) {
            // First sequence is empty, insert all elements from second sequence
            for (int j = 0; j < m; ++j) {
                script.push_back({EditOp::INSERT, 0, static_cast<size_t>(j)});
            }
            return script;
        }
        
        if (m == 0) {
            // Second sequence is empty, delete all elements from first sequence
            for (int i = 0; i < n; ++i) {
                script.push_back({EditOp::DELETE, static_cast<size_t>(i), 0});
            }
            return script;
        }
        
        // Myers algorithm
        int max = n + m;
        std::vector<int> v(2 * max + 1, 0);
        std::vector<std::unordered_map<int, Point>> trace;
        
        int x, y;
        
        for (int d = 0; d <= max; ++d) {
            trace.push_back({});
            
            for (int k = -d; k <= d; k += 2) {
                if (k == -d || (k != d && v[k - 1 + max] < v[k + 1 + max])) {
                    x = v[k + 1 + max];
                } else {
                    x = v[k - 1 + max] + 1;
                }
                
                y = x - k;
                
                // Save the point we came from
                if (k == -d || (k != d && v[k - 1 + max] < v[k + 1 + max])) {
                    trace[d][k] = {x, y - 1}; // Came from below (insertion)
                } else {
                    trace[d][k] = {x - 1, y}; // Came from left (deletion)
                }
                
                // Follow diagonal as far as possible
                while (x < n && y < m && seq1[x] == seq2[y]) {
                    ++x;
                    ++y;
                }
                
                v[k + max] = x;
                
                if (x >= n && y >= m) {
                    // Found the end
                    
                    // Backtrack to construct the edit script
                    std::vector<EditScriptItem> backScript;
                    
                    int curX = n;
                    int curY = m;
                    
                    for (int i = d; i > 0; --i) {
                        int curK = curX - curY;
                        Point prev = trace[i][curK];
                        
                        if (prev.x < curX && prev.y < curY) {
                            // Diagonal move (keep)
                            while (curX > prev.x && curY > prev.y) {
                                --curX;
                                --curY;
                                backScript.push_back({EditOp::KEEP, static_cast<size_t>(curX), static_cast<size_t>(curY)});
                            }
                        } else if (prev.x < curX) {
                            // Horizontal move (delete)
                            --curX;
                            backScript.push_back({EditOp::DELETE, static_cast<size_t>(curX), static_cast<size_t>(curY)});
                        } else {
                            // Vertical move (insert)
                            --curY;
                            backScript.push_back({EditOp::INSERT, static_cast<size_t>(curX), static_cast<size_t>(curY)});
                        }
                        
                        curX = prev.x;
                        curY = prev.y;
                    }
                    
                    // Include any initial diagonal
                    while (curX > 0 && curY > 0) {
                        --curX;
                        --curY;
                        backScript.push_back({EditOp::KEEP, static_cast<size_t>(curX), static_cast<size_t>(curY)});
                    }
                    
                    while (curX > 0) {
                        --curX;
                        backScript.push_back({EditOp::DELETE, static_cast<size_t>(curX), static_cast<size_t>(curY)});
                    }
                    
                    while (curY > 0) {
                        --curY;
                        backScript.push_back({EditOp::INSERT, static_cast<size_t>(curX), static_cast<size_t>(curY)});
                    }
                    
                    // Reverse the script to get the correct order
                    std::reverse(backScript.begin(), backScript.end());
                    
                    return backScript;
                }
            }
        }
        
        // Should not reach here
        LOG_ERROR("Myers diff algorithm failed to find a solution");
        return script;
    }
    
    /**
     * @brief Convert an edit script to diff changes
     * 
     * @tparam T Type of sequence elements
     * @param script Edit script
     * @param seq1 First sequence
     * @param seq2 Second sequence
     * @param isLineLevel Whether this is a line-level diff
     * @return Vector of diff changes
     */
    template<typename T>
    std::vector<DiffChange> convertScriptToChanges(
        const std::vector<EditScriptItem>& script,
        const std::vector<T>& seq1,
        const std::vector<T>& seq2,
        bool isLineLevel) {
        
        std::vector<DiffChange> changes;
        
        if (script.empty()) {
            // No changes, everything is equal
            if (!seq1.empty() || !seq2.empty()) {
                DiffChange change;
                change.type = DiffChange::ChangeType::EQUAL;
                change.startLine1 = 0;
                change.lineCount1 = seq1.size();
                change.startLine2 = 0;
                change.lineCount2 = seq2.size();
                change.startChar1 = 0;
                change.charCount1 = 0;
                change.startChar2 = 0;
                change.charCount2 = 0;
                change.isLineLevel = isLineLevel;
                
                changes.push_back(change);
            }
            
            return changes;
        }
        
        // Group adjacent operations of the same type
        DiffChange currentChange;
        currentChange.type = DiffChange::ChangeType::EQUAL;
        currentChange.startLine1 = 0;
        currentChange.lineCount1 = 0;
        currentChange.startLine2 = 0;
        currentChange.lineCount2 = 0;
        currentChange.startChar1 = 0;
        currentChange.charCount1 = 0;
        currentChange.startChar2 = 0;
        currentChange.charCount2 = 0;
        currentChange.isLineLevel = isLineLevel;
        
        bool hasCurrentChange = false;
        
        for (const auto& item : script) {
            DiffChange::ChangeType itemType;
            
            switch (item.op) {
                case EditOp::KEEP:
                    itemType = DiffChange::ChangeType::EQUAL;
                    break;
                case EditOp::INSERT:
                    itemType = DiffChange::ChangeType::INSERT;
                    break;
                case EditOp::DELETE:
                    itemType = DiffChange::ChangeType::DELETE;
                    break;
                default:
                    LOG_ERROR("Unknown edit operation");
                    continue;
            }
            
            if (!hasCurrentChange) {
                // First item
                currentChange.type = itemType;
                
                if (isLineLevel) {
                    // Line-level diff
                    if (itemType == DiffChange::ChangeType::EQUAL) {
                        currentChange.startLine1 = item.idx1;
                        currentChange.lineCount1 = 1;
                        currentChange.startLine2 = item.idx2;
                        currentChange.lineCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::INSERT) {
                        currentChange.startLine1 = item.idx1;
                        currentChange.lineCount1 = 0;
                        currentChange.startLine2 = item.idx2;
                        currentChange.lineCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::DELETE) {
                        currentChange.startLine1 = item.idx1;
                        currentChange.lineCount1 = 1;
                        currentChange.startLine2 = item.idx2;
                        currentChange.lineCount2 = 0;
                    }
                } else {
                    // Character-level diff
                    if (itemType == DiffChange::ChangeType::EQUAL) {
                        currentChange.startChar1 = item.idx1;
                        currentChange.charCount1 = 1;
                        currentChange.startChar2 = item.idx2;
                        currentChange.charCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::INSERT) {
                        currentChange.startChar1 = item.idx1;
                        currentChange.charCount1 = 0;
                        currentChange.startChar2 = item.idx2;
                        currentChange.charCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::DELETE) {
                        currentChange.startChar1 = item.idx1;
                        currentChange.charCount1 = 1;
                        currentChange.startChar2 = item.idx2;
                        currentChange.charCount2 = 0;
                    }
                }
                
                hasCurrentChange = true;
            } else if (currentChange.type == itemType) {
                // Same type as current change, extend it
                if (isLineLevel) {
                    // Line-level diff
                    if (itemType == DiffChange::ChangeType::EQUAL) {
                        ++currentChange.lineCount1;
                        ++currentChange.lineCount2;
                    } else if (itemType == DiffChange::ChangeType::INSERT) {
                        ++currentChange.lineCount2;
                    } else if (itemType == DiffChange::ChangeType::DELETE) {
                        ++currentChange.lineCount1;
                    }
                } else {
                    // Character-level diff
                    if (itemType == DiffChange::ChangeType::EQUAL) {
                        ++currentChange.charCount1;
                        ++currentChange.charCount2;
                    } else if (itemType == DiffChange::ChangeType::INSERT) {
                        ++currentChange.charCount2;
                    } else if (itemType == DiffChange::ChangeType::DELETE) {
                        ++currentChange.charCount1;
                    }
                }
            } else {
                // Different type, start a new change
                changes.push_back(currentChange);
                
                currentChange.type = itemType;
                
                if (isLineLevel) {
                    // Line-level diff
                    if (itemType == DiffChange::ChangeType::EQUAL) {
                        currentChange.startLine1 = item.idx1;
                        currentChange.lineCount1 = 1;
                        currentChange.startLine2 = item.idx2;
                        currentChange.lineCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::INSERT) {
                        currentChange.startLine1 = item.idx1;
                        currentChange.lineCount1 = 0;
                        currentChange.startLine2 = item.idx2;
                        currentChange.lineCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::DELETE) {
                        currentChange.startLine1 = item.idx1;
                        currentChange.lineCount1 = 1;
                        currentChange.startLine2 = item.idx2;
                        currentChange.lineCount2 = 0;
                    }
                } else {
                    // Character-level diff
                    if (itemType == DiffChange::ChangeType::EQUAL) {
                        currentChange.startChar1 = item.idx1;
                        currentChange.charCount1 = 1;
                        currentChange.startChar2 = item.idx2;
                        currentChange.charCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::INSERT) {
                        currentChange.startChar1 = item.idx1;
                        currentChange.charCount1 = 0;
                        currentChange.startChar2 = item.idx2;
                        currentChange.charCount2 = 1;
                    } else if (itemType == DiffChange::ChangeType::DELETE) {
                        currentChange.startChar1 = item.idx1;
                        currentChange.charCount1 = 1;
                        currentChange.startChar2 = item.idx2;
                        currentChange.charCount2 = 0;
                    }
                }
            }
        }
        
        // Add the last change
        if (hasCurrentChange) {
            changes.push_back(currentChange);
        }
        
        // Post-process changes to identify replacements
        std::vector<DiffChange> processedChanges;
        
        for (size_t i = 0; i < changes.size(); ++i) {
            if (i + 1 < changes.size() && 
                changes[i].type == DiffChange::ChangeType::DELETE &&
                changes[i + 1].type == DiffChange::ChangeType::INSERT) {
                
                // Consecutive delete and insert - convert to replace
                DiffChange replace;
                replace.type = DiffChange::ChangeType::REPLACE;
                replace.isLineLevel = isLineLevel;
                
                if (isLineLevel) {
                    replace.startLine1 = changes[i].startLine1;
                    replace.lineCount1 = changes[i].lineCount1;
                    replace.startLine2 = changes[i + 1].startLine2;
                    replace.lineCount2 = changes[i + 1].lineCount2;
                } else {
                    replace.startChar1 = changes[i].startChar1;
                    replace.charCount1 = changes[i].charCount1;
                    replace.startChar2 = changes[i + 1].startChar2;
                    replace.charCount2 = changes[i + 1].charCount2;
                }
                
                processedChanges.push_back(replace);
                ++i; // Skip the insert
            } else if (i + 1 < changes.size() && 
                       changes[i].type == DiffChange::ChangeType::INSERT &&
                       changes[i + 1].type == DiffChange::ChangeType::DELETE) {
                
                // Consecutive insert and delete - convert to replace
                DiffChange replace;
                replace.type = DiffChange::ChangeType::REPLACE;
                replace.isLineLevel = isLineLevel;
                
                if (isLineLevel) {
                    replace.startLine1 = changes[i + 1].startLine1;
                    replace.lineCount1 = changes[i + 1].lineCount1;
                    replace.startLine2 = changes[i].startLine2;
                    replace.lineCount2 = changes[i].lineCount2;
                } else {
                    replace.startChar1 = changes[i + 1].startChar1;
                    replace.charCount1 = changes[i + 1].charCount1;
                    replace.startChar2 = changes[i].startChar2;
                    replace.charCount2 = changes[i].charCount2;
                }
                
                processedChanges.push_back(replace);
                ++i; // Skip the delete
            } else {
                processedChanges.push_back(changes[i]);
            }
        }
        
        return processedChanges;
    }
}; 