#ifndef ERROR_REPORTER_IMPL_H
#define ERROR_REPORTER_IMPL_H

#include <string>
#include <iostream>
#include <mutex>
#include <ctime>

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
        std::lock_guard<std::mutex> lock(mutex_);
        
        const char* level = "UNKNOWN";
        switch (severity) {
            case Severity::DEBUG: level = "DEBUG"; break;
            case Severity::INFO: level = "INFO"; break;
            case Severity::WARNING: level = "WARNING"; break;
            case Severity::ERROR: level = "ERROR"; break;
            case Severity::CRITICAL: level = "CRITICAL"; break;
        }
        
        // Get current time
        std::time_t now = std::time(nullptr);
        char timeStr[20];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        std::cerr << "[" << timeStr << "] [" << level << "] " << source << ": " << message << std::endl;
    }

    static void logError(const std::string& message) {
        getInstance().report(Severity::ERROR, message, "TextBuffer");
    }

private:
    std::mutex mutex_;
    
    // Private constructor to prevent instantiation
    ErrorReporter() = default;
    ~ErrorReporter() = default;
    
    // Prevent copying and assignment
    ErrorReporter(const ErrorReporter&) = delete;
    ErrorReporter& operator=(const ErrorReporter&) = delete;
};

} // namespace ai_editor

#endif // ERROR_REPORTER_IMPL_H
