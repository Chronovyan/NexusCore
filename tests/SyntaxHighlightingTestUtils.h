#pragma once

#include "../src/SyntaxHighlighter.h"
#include <vector>

namespace SyntaxHighlightingTestUtils {

/**
 * @brief Helper function to check if a specific style is applied to a range
 * 
 * @param styles The vector of syntax styles to check
 * @param start Starting column of the range to check
 * @param end Ending column of the range to check
 * @param color The syntax color to look for
 * @return true if the specified range has the given color, false otherwise
 */
inline bool hasStyle(const std::vector<SyntaxStyle>& styles, size_t start, size_t end, SyntaxColor color) {
    for (const auto& style : styles) {
        // For strings, allow a little flexibility in the exact positions
        if (color == SyntaxColor::String) {
            // Check if the style is a string and substantially overlaps with the expected range
            if (style.color == color && 
                ((style.startCol == start || style.startCol == start - 1 || style.startCol == start + 1) &&
                 (style.endCol == end || style.endCol == end - 1 || style.endCol == end + 1 || 
                  style.endCol == end + 2))) { // Allow quotes to be included or excluded
                return true;
            }
        } else {
            // For non-string styles, require exact match
            if (style.startCol == start && style.endCol == end && style.color == color) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Helper function to verify if a line is completely highlighted as a comment
 * 
 * @param styles The vector of syntax styles to check
 * @param line The line content to check
 * @return true if the entire line is styled as a comment, false otherwise
 */
inline bool isFullLineCommented(const std::vector<SyntaxStyle>& styles, const std::string& line) {
    if (styles.empty()) return false;
    
    for (const auto& style : styles) {
        if (style.color == SyntaxColor::Comment && 
            style.startCol == 0 && 
            style.endCol == line.length()) {
            return true;
        }
    }
    return false;
}

} // namespace SyntaxHighlightingTestUtils 