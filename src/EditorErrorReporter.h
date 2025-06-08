#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

/**
 * @class EditorErrorReporter
 * @brief Utility class for reporting errors in the editor
 */
class EditorErrorReporter {
public:
    // Severity levels
    static inline const std::unordered_map<int, std::string> severityNames = {
        {1, "INFO"},
        {2, "WARNING"},
        {3, "ERROR"},
        {4, "CRITICAL"}
    };

    /**
     * @brief Report an error to the user or log
     * 
     * @param source The source of the error (e.g., class or component name)
     * @param errorMessage The error message
     * @param level The severity level (1 = info, 2 = warning, 3 = error, 4 = critical)
     */
    static void reportError(
        const std::string& source = "Unknown",
        const std::string& errorMessage = "",
        int level = 3
    ) {
        std::string levelStr;
        switch (level) {
            case 1: levelStr = "INFO"; break;
            case 2: levelStr = "WARNING"; break;
            case 3: levelStr = "ERROR"; break;
            case 4: levelStr = "CRITICAL"; break;
            default: levelStr = "UNKNOWN"; break;
        }
        
        std::cerr << "[" << levelStr << "] " << source << ": " << errorMessage << std::endl;
    }
    
    /**
     * @brief Report a warning to the user or log
     * 
     * @param source The source of the warning
     * @param warningMessage The warning message
     */
    static void reportWarning(
        const std::string& source = "Unknown",
        const std::string& warningMessage = ""
    ) {
        reportError(source, warningMessage, 2);
    }
    
    /**
     * @brief Report information to the user or log
     * 
     * @param source The source of the information
     * @param infoMessage The information message
     */
    static void reportInfo(
        const std::string& source = "Unknown",
        const std::string& infoMessage = ""
    ) {
        reportError(source, infoMessage, 1);
    }
}; 