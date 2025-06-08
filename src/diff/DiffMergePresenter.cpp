#include "DiffMergePresenter.h"
#include "AppDebugLog.h"
#include <sstream>
#include <iostream>
#include <algorithm>

DiffMergePresenter::DiffMergePresenter(bool useColors)
    : useColors_(useColors) {
    
    // Initialize color codes
    colorCodes_[AnsiColor::RESET] = "\033[0m";
    colorCodes_[AnsiColor::RED] = "\033[31m";
    colorCodes_[AnsiColor::GREEN] = "\033[32m";
    colorCodes_[AnsiColor::YELLOW] = "\033[33m";
    colorCodes_[AnsiColor::BLUE] = "\033[34m";
    colorCodes_[AnsiColor::MAGENTA] = "\033[35m";
    colorCodes_[AnsiColor::CYAN] = "\033[36m";
    colorCodes_[AnsiColor::WHITE] = "\033[37m";
    colorCodes_[AnsiColor::BRIGHT_RED] = "\033[91m";
    colorCodes_[AnsiColor::BRIGHT_GREEN] = "\033[92m";
    colorCodes_[AnsiColor::BRIGHT_YELLOW] = "\033[93m";
    colorCodes_[AnsiColor::BRIGHT_BLUE] = "\033[94m";
    colorCodes_[AnsiColor::BRIGHT_MAGENTA] = "\033[95m";
    colorCodes_[AnsiColor::BRIGHT_CYAN] = "\033[96m";
    colorCodes_[AnsiColor::BRIGHT_WHITE] = "\033[97m";
    
    LOG_DEBUG("DiffMergePresenter created with colors " + 
              std::string(useColors_ ? "enabled" : "disabled"));
}

void DiffMergePresenter::setUseColors(bool useColors) {
    useColors_ = useColors;
}

bool DiffMergePresenter::getUseColors() const {
    return useColors_;
}

std::string DiffMergePresenter::formatColoredDiff(
    const std::vector<DiffChange>& changes,
    const std::vector<std::string>& text1,
    const std::vector<std::string>& text2,
    size_t contextLines) {
    
    if (changes.empty()) {
        return ""; // No changes
    }
    
    std::ostringstream result;
    
    // Group changes into hunks
    std::vector<std::pair<size_t, size_t>> hunks;
    size_t hunkStart = 0;
    size_t hunkEnd = 0;
    bool inHunk = false;
    
    for (const auto& change : changes) {
        if (change.isEqual()) {
            // Skip equal changes unless they're within context range
            if (inHunk) {
                hunkEnd = change.startLine1 + change.lineCount1;
                
                if (change.lineCount1 > 2 * contextLines) {
                    // End hunk if equal section is longer than 2*contextLines
                    hunks.emplace_back(hunkStart, hunkEnd - change.lineCount1 + contextLines);
                    
                    // Start new hunk if there are more changes
                    if (&change != &changes.back()) {
                        hunkStart = change.startLine1 + change.lineCount1 - contextLines;
                    } else {
                        inHunk = false;
                    }
                }
            }
        } else {
            // Start new hunk if not in one
            if (!inHunk) {
                hunkStart = (change.startLine1 > contextLines) ? 
                            (change.startLine1 - contextLines) : 0;
                inHunk = true;
            }
            
            // Update hunk end
            hunkEnd = change.startLine1 + change.lineCount1;
        }
    }
    
    // Add last hunk if any
    if (inHunk) {
        hunks.emplace_back(hunkStart, std::min(hunkEnd + contextLines, text1.size()));
    }
    
    // Process each hunk
    for (const auto& [start, end] : hunks) {
        // Calculate line numbers for hunk header
        size_t oldStart = start + 1; // 1-based line numbers
        size_t oldCount = 0;
        size_t newStart = 0; // Will be calculated
        size_t newCount = 0;
        
        // Find corresponding position in text2
        for (const auto& change : changes) {
            if (change.startLine1 >= start && change.startLine1 < end) {
                if (newStart == 0) {
                    // Set newStart to the corresponding position in text2
                    newStart = change.startLine2 + 1; // 1-based line numbers
                }
                
                // Update line counts
                oldCount += change.lineCount1;
                newCount += change.lineCount2;
            }
        }
        
        // Write hunk header with cyan color
        std::string header = "@@ -" + std::to_string(oldStart) + "," + 
                            std::to_string(oldCount) + " +" + 
                            std::to_string(newStart) + "," + 
                            std::to_string(newCount) + " @@";
        
        if (useColors_) {
            result << getColorCode(AnsiColor::CYAN) << header << 
                      getColorCode(AnsiColor::RESET) << "\n";
        } else {
            result << header << "\n";
        }
        
        // Write hunk lines
        for (size_t i = start; i < end; ++i) {
            // Find if this line is part of a change
            bool found = false;
            
            for (const auto& change : changes) {
                if (i >= change.startLine1 && i < change.startLine1 + change.lineCount1) {
                    found = true;
                    
                    if (change.isEqual()) {
                        // Equal line
                        result << " " << text1[i] << "\n";
                    } else if (change.isDelete()) {
                        // Deleted line with red color
                        if (useColors_) {
                            result << "-" << getColorCode(AnsiColor::RED) << 
                                      text1[i] << getColorCode(AnsiColor::RESET) << "\n";
                        } else {
                            result << "-" << text1[i] << "\n";
                        }
                    } else if (change.isInsert()) {
                        // Inserted line with green color
                        if (useColors_) {
                            result << "+" << getColorCode(AnsiColor::GREEN) << 
                                      text2[change.startLine2 + (i - change.startLine1)] << 
                                      getColorCode(AnsiColor::RESET) << "\n";
                        } else {
                            result << "+" << text2[change.startLine2 + (i - change.startLine1)] << "\n";
                        }
                    } else if (change.isReplace()) {
                        // Replaced line
                        if (useColors_) {
                            result << "-" << getColorCode(AnsiColor::RED) << 
                                      text1[i] << getColorCode(AnsiColor::RESET) << "\n";
                        } else {
                            result << "-" << text1[i] << "\n";
                        }
                        
                        // Add corresponding line from text2
                        size_t offset = i - change.startLine1;
                        if (offset < change.lineCount2) {
                            if (useColors_) {
                                result << "+" << getColorCode(AnsiColor::GREEN) << 
                                          text2[change.startLine2 + offset] << 
                                          getColorCode(AnsiColor::RESET) << "\n";
                            } else {
                                result << "+" << text2[change.startLine2 + offset] << "\n";
                            }
                        }
                    }
                    
                    break;
                }
            }
            
            // If not found in any change, it's a context line
            if (!found) {
                result << " " << text1[i] << "\n";
            }
        }
        
        // Add separator between hunks
        if (&hunks.back() != &(*hunks.rbegin())) {
            result << "\n";
        }
    }
    
    return result.str();
}

std::string DiffMergePresenter::formatColoredMerge(const MergeResult& mergeResult) {
    std::ostringstream result;
    
    // If there are no conflicts, just return the merged text
    if (!mergeResult.hasConflicts) {
        for (const auto& line : mergeResult.mergedLines) {
            result << line << "\n";
        }
        return result.str();
    }
    
    // Format with conflicts highlighted
    size_t currentLine = 0;
    
    for (const auto& conflict : mergeResult.conflicts) {
        // Add lines before the conflict
        for (; currentLine < conflict.startLine; ++currentLine) {
            result << mergeResult.mergedLines[currentLine] << "\n";
        }
        
        // Add conflict with colors
        std::string coloredConflict = formatColoredConflict(conflict);
        result << coloredConflict;
        
        // Skip conflict lines
        currentLine += conflict.lineCount;
    }
    
    // Add remaining lines
    for (; currentLine < mergeResult.mergedLines.size(); ++currentLine) {
        result << mergeResult.mergedLines[currentLine] << "\n";
    }
    
    return result.str();
}

std::string DiffMergePresenter::formatColoredConflict(const MergeConflict& conflict) {
    std::ostringstream result;
    
    // Format conflict header
    if (useColors_) {
        result << getColorCode(AnsiColor::BRIGHT_RED) << 
                  "<<<<<<<" << getColorCode(AnsiColor::RESET) << "\n";
    } else {
        result << "<<<<<<<\n";
    }
    
    // Format our lines with blue color
    for (const auto& line : conflict.ourLines) {
        if (useColors_) {
            result << getColorCode(AnsiColor::BLUE) << line << 
                      getColorCode(AnsiColor::RESET) << "\n";
        } else {
            result << line << "\n";
        }
    }
    
    // Format separator
    if (useColors_) {
        result << getColorCode(AnsiColor::BRIGHT_YELLOW) << 
                  "=======" << getColorCode(AnsiColor::RESET) << "\n";
    } else {
        result << "=======\n";
    }
    
    // Format their lines with green color
    for (const auto& line : conflict.theirLines) {
        if (useColors_) {
            result << getColorCode(AnsiColor::GREEN) << line << 
                      getColorCode(AnsiColor::RESET) << "\n";
        } else {
            result << line << "\n";
        }
    }
    
    // Format conflict footer
    if (useColors_) {
        result << getColorCode(AnsiColor::BRIGHT_RED) << 
                  ">>>>>>>" << getColorCode(AnsiColor::RESET) << "\n";
    } else {
        result << ">>>>>>>\n";
    }
    
    return result.str();
}

void DiffMergePresenter::writeColored(std::ostream& os, const std::string& text, AnsiColor color) {
    if (useColors_) {
        os << getColorCode(color) << text << getColorCode(AnsiColor::RESET);
    } else {
        os << text;
    }
}

std::string DiffMergePresenter::getColorCode(AnsiColor color) const {
    if (!useColors_) {
        return "";
    }
    
    auto it = colorCodes_.find(color);
    if (it != colorCodes_.end()) {
        return it->second;
    }
    
    return "";
} 