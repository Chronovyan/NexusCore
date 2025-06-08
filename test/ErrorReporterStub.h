#ifndef ERROR_REPORTER_STUB_H
#define ERROR_REPORTER_STUB_H

#include <string>
#include <iostream>

namespace ai_editor {

class ErrorReporter {
public:
    enum class Severity {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    static ErrorReporter& getInstance() {
        static ErrorReporter instance;
        return instance;
    }

    void report(Severity severity, const std::string& message, const std::string& source = "Unknown") {
        // Simple console output for testing
        const char* severityStr = "UNKNOWN";
        switch (severity) {
            case Severity::DEBUG: severityStr = "DEBUG"; break;
            case Severity::INFO: severityStr = "INFO"; break;
            case Severity::WARNING: severityStr = "WARNING"; break;
            case Severity::ERROR: severityStr = "ERROR"; break;
            case Severity::CRITICAL: severityStr = "CRITICAL"; break;
        }
        std::cerr << "[" << severityStr << "] " << source << ": " << message << std::endl;
    }

    // Stub implementation for logError
    static void logError(const std::string& message) {
        getInstance().report(Severity::ERROR, message, "TextBuffer");
    }
};

} // namespace ai_editor

#endif // ERROR_REPORTER_STUB_H
