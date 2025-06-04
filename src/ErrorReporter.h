#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <ctime>

namespace ai_editor {

/**
 * @class ErrorReporter
 * @brief Singleton class for reporting errors and other messages in the application.
 */
class ErrorReporter {
public:
    /**
     * @enum Severity
     * @brief Represents the severity level of a message.
     */
    enum class Severity {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    /**
     * @brief Get the singleton instance of the ErrorReporter.
     * @return Reference to the ErrorReporter instance.
     */
    static ErrorReporter& getInstance() {
        static ErrorReporter instance;
        return instance;
    }

    /**
     * @brief Report a message with the specified severity.
     * @param severity Severity level of the message.
     * @param message The message to report.
     * @param source Optional source of the message (e.g., class or function name).
     */
    void report(Severity severity, const std::string& message, const std::string& source = "Unknown") {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Get current time
        std::time_t now = std::time(nullptr);
        char timeStr[20];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        // Convert severity to string
        std::string severityStr;
        switch (severity) {
            case Severity::DEBUG: severityStr = "DEBUG"; break;
            case Severity::INFO: severityStr = "INFO"; break;
            case Severity::WARNING: severityStr = "WARNING"; break;
            case Severity::ERROR: severityStr = "ERROR"; break;
            case Severity::CRITICAL: severityStr = "CRITICAL"; break;
            default: severityStr = "UNKNOWN"; break;
        }
        
        // Format the message
        std::string formattedMessage = 
            std::string(timeStr) + " [" + severityStr + "] " + source + ": " + message;
        
        // Output to console
        if (severity == Severity::DEBUG) {
            std::cout << formattedMessage << std::endl;
        } else {
            std::cerr << formattedMessage << std::endl;
        }
        
        // Log to file if logging is enabled
        if (logToFile && logFile.is_open()) {
            logFile << formattedMessage << std::endl;
            logFile.flush();
        }
    }

    /**
     * @brief Enable or disable logging to a file.
     * @param enable Whether to enable file logging.
     * @param filename The name of the log file.
     * @return True if operation succeeded, false otherwise.
     */
    bool setFileLogging(bool enable, const std::string& filename = "error.log") {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Close existing log file if open
        if (logFile.is_open()) {
            logFile.close();
        }
        
        logToFile = enable;
        
        if (enable) {
            logFile.open(filename, std::ios::app);
            if (!logFile.is_open()) {
                std::cerr << "Failed to open log file: " << filename << std::endl;
                logToFile = false;
                return false;
            }
        }
        
        return true;
    }

    // Convenience methods for different severity levels
    void debug(const std::string& message, const std::string& source = "Unknown") {
        report(Severity::DEBUG, message, source);
    }
    
    void info(const std::string& message, const std::string& source = "Unknown") {
        report(Severity::INFO, message, source);
    }
    
    void warning(const std::string& message, const std::string& source = "Unknown") {
        report(Severity::WARNING, message, source);
    }
    
    void error(const std::string& message, const std::string& source = "Unknown") {
        report(Severity::ERROR, message, source);
    }
    
    void critical(const std::string& message, const std::string& source = "Unknown") {
        report(Severity::CRITICAL, message, source);
    }
    
    // Delete copy constructor and assignment operator
    ErrorReporter(const ErrorReporter&) = delete;
    ErrorReporter& operator=(const ErrorReporter&) = delete;

private:
    // Private constructor for singleton
    ErrorReporter() : logToFile(false) {}
    
    // Private destructor
    ~ErrorReporter() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    std::mutex mutex;
    bool logToFile;
    std::ofstream logFile;
};

// Global instance for easy access
#define ERROR_REPORTER ErrorReporter::getInstance()

} // namespace ai_editor

#endif // ERROR_REPORTER_H 