#pragma once

#include <string>
#include <vector>

namespace ai_editor {

/**
 * @brief Represents a single search result match
 */
struct SearchResult {
    int line;           // 1-based line number
    int start;          // 0-based start column
    int end;            // 0-based end column
    std::string match;  // The matched text
    std::string context; // Line of text containing the match
};

} // namespace ai_editor
