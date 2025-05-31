#include <string>
#include <cstdarg>

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// Fix for log function that allows std::string as format
void logImpl(LogLevel level, const char* file, int line, const char* format, ...) {
    // Original implementation
}

// Overload for std::string
void logImpl(LogLevel level, const char* file, int line, const std::string& message) {
    // Convert std::string to const char*
    logImpl(level, file, line, message.c_str());
}

// Example usage
void example() {
    std::string message = "Test message";
    // This will now work:
    logImpl(INFO, __FILE__, __LINE__, message);
} 