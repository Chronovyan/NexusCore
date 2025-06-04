#pragma once

#include "interfaces/IDiffEngine.hpp"
#include "interfaces/IMergeEngine.hpp"
#include <string>
#include <vector>
#include <ostream>
#include <map>

/**
 * @enum AnsiColor
 * @brief ANSI color codes for terminal output
 */
enum class AnsiColor {
    RESET,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_WHITE
};

/**
 * @class DiffMergePresenter
 * @brief Utility class for presenting diffs and merges
 * 
 * This class provides methods for formatting and presenting diffs and merges
 * with color-coding and other visual enhancements.
 */
class DiffMergePresenter {
public:
    /**
     * @brief Constructor
     * 
     * @param useColors Whether to use colors in the output
     */
    explicit DiffMergePresenter(bool useColors = true);
    
    /**
     * @brief Set whether to use colors in the output
     * 
     * @param useColors Whether to use colors
     */
    void setUseColors(bool useColors);
    
    /**
     * @brief Get whether colors are used in the output
     * 
     * @return Whether colors are used
     */
    bool getUseColors() const;
    
    /**
     * @brief Format a diff as colored text
     * 
     * @param changes The diff changes
     * @param text1 The first text
     * @param text2 The second text
     * @param contextLines Number of context lines to include
     * @return Formatted diff as a string
     */
    std::string formatColoredDiff(
        const std::vector<DiffChange>& changes,
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2,
        size_t contextLines = 3);
    
    /**
     * @brief Format a merge result as colored text
     * 
     * @param mergeResult The merge result
     * @return Formatted merge result as a string
     */
    std::string formatColoredMerge(const MergeResult& mergeResult);
    
    /**
     * @brief Format a merge conflict as colored text
     * 
     * @param conflict The merge conflict
     * @return Formatted conflict as a string
     */
    std::string formatColoredConflict(const MergeConflict& conflict);
    
    /**
     * @brief Write colored text to an output stream
     * 
     * @param os The output stream
     * @param text The text to write
     * @param color The color to use
     */
    void writeColored(std::ostream& os, const std::string& text, AnsiColor color);
    
    /**
     * @brief Get the ANSI color code for a color
     * 
     * @param color The color
     * @return The ANSI color code
     */
    std::string getColorCode(AnsiColor color) const;
    
private:
    bool useColors_;
    std::map<AnsiColor, std::string> colorCodes_;
}; 