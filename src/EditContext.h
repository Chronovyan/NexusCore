#pragma once

// Stub file for EditContext.h
// This is a minimal implementation to satisfy include requirements

#include <string>
#include <vector>

namespace ai_editor {

// Simple struct to represent an editing context
struct EditContext {
    std::string filePath;
    size_t line;
    size_t column;
    std::string selectedText;
    std::vector<std::string> visibleFiles;
};

} // namespace ai_editor 