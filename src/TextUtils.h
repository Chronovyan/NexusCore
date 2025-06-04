#pragma once

#include <string>
#include <vector>
#include <cctype>

namespace ai_editor {

class TextUtils {
public:
    // Split a string into lines
    static std::vector<std::string> splitLines(const std::string& text) {
        std::vector<std::string> lines;
        size_t start = 0;
        size_t end = text.find('\n');
        
        while (end != std::string::npos) {
            if (end > 0 && text[end - 1] == '\r') {
                lines.push_back(text.substr(start, end - start - 1));
            } else {
                lines.push_back(text.substr(start, end - start));
            }
            
            start = end + 1;
            end = text.find('\n', start);
        }
        
        if (start < text.length()) {
            lines.push_back(text.substr(start));
        }
        
        return lines;
    }

    // Trim whitespace from the beginning and end of a string
    static std::string trim(const std::string& text) {
        size_t start = 0;
        size_t end = text.length();
        
        while (start < end && std::isspace(text[start])) {
            ++start;
        }
        
        while (end > start && std::isspace(text[end - 1])) {
            --end;
        }
        
        return text.substr(start, end - start);
    }
};

} // namespace ai_editor 