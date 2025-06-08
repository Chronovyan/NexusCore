#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <cstdio>
#include <ctime>

// Unified Logging System
class UnifiedLogger {
private:
    static std::mutex logMutex_;
    static bool enableVerboseLogging_;
    
    // Log level enum
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    static LogLevel currentLogLevel_;
    
    // Get current timestamp as string
    static std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm timeinfo;
    #ifdef _WIN32
        localtime_s(&timeinfo, &in_time_t);
    #else
        localtime_r(&in_time_t, &timeinfo);
    #endif
        
        std::stringstream ss;
        ss << std::put_time(&timeinfo, "[%Y-%m-%d %H:%M:%S]");
        return ss.str();
    }
    
    // Internal log implementation
    static void logInternal(LogLevel level, const char* file, int line, const std::string& message) {
        if (level < currentLogLevel_) return;
        
        const char* levelStr = "";
        switch (level) {
            case LogLevel::DEBUG:    levelStr = "DEBUG"; break;
            case LogLevel::INFO:     levelStr = "INFO"; break;
            case LogLevel::WARNING:  levelStr = "WARNING"; break;
            case LogLevel::ERROR:    levelStr = "ERROR"; break;
            case LogLevel::CRITICAL: levelStr = "CRITICAL"; break;
        }
        
        std::lock_guard<std::mutex> lock(logMutex_);
        
        // Extract filename from path
        const char* filename = file;
        const char* lastSlash = strrchr(file, '/');
        if (lastSlash) {
            filename = lastSlash + 1;
        } else {
            lastSlash = strrchr(file, '\\');
            if (lastSlash) {
                filename = lastSlash + 1;
            }
        }
        
        std::cout << getTimestamp() << " [" << levelStr << "] " 
                  << filename << ":" << line << " - " << message << std::endl;
    }
    
    // Format message with variadic arguments
    template<typename... Args>
    static std::string formatMessage(const std::string& format, Args... args) {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        return std::string(buffer);
    }
    
public:
    // Set log level
    static void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex_);
        currentLogLevel_ = level;
        enableVerboseLogging_ = (level == LogLevel::DEBUG);
    }
    
    // Get current log level
    static LogLevel getLogLevel() {
        return currentLogLevel_;
    }
    
    // Enable/disable verbose logging
    static void setVerboseLogging(bool enabled) {
        std::lock_guard<std::mutex> lock(logMutex_);
        enableVerboseLogging_ = enabled;
        if (enabled && currentLogLevel_ > LogLevel::DEBUG) {
            currentLogLevel_ = LogLevel::DEBUG;
        }
    }
    
    // Check if verbose logging is enabled
    static bool isVerboseLoggingEnabled() {
        return enableVerboseLogging_;
    }
    
    // Log methods
    template<typename... Args>
    static void debug(const char* file, int line, const std::string& format, Args... args) {
        logInternal(LogLevel::DEBUG, file, line, formatMessage(format, args...));
    }
    
    template<typename... Args>
    static void info(const char* file, int line, const std::string& format, Args... args) {
        logInternal(LogLevel::INFO, file, line, formatMessage(format, args...));
    }
    
    template<typename... Args>
    static void warning(const char* file, int line, const std::string& format, Args... args) {
        logInternal(LogLevel::WARNING, file, line, formatMessage(format, args...));
    }
    
    template<typename... Args>
    static void error(const char* file, int line, const std::string& format, Args... args) {
        logInternal(LogLevel::ERROR, file, line, formatMessage(format, args...));
    }
    
    template<typename... Args>
    static void critical(const char* file, int line, const std::string& format, Args... args) {
        logInternal(LogLevel::CRITICAL, file, line, formatMessage(format, args...));
    }
};

// Initialize static members
std::mutex UnifiedLogger::logMutex_;
bool UnifiedLogger::enableVerboseLogging_ = false;
UnifiedLogger::LogLevel UnifiedLogger::currentLogLevel_ = UnifiedLogger::LogLevel::INFO;

// Logging macros
#define LOG_DEBUG(format, ...)    UnifiedLogger::debug(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)     UnifiedLogger::info(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)  UnifiedLogger::warning(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)    UnifiedLogger::error(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_CRITICAL(format, ...) UnifiedLogger::critical(__FILE__, __LINE__, format, ##__VA_ARGS__)

// For backward compatibility
#define LOG_INIT(component) \
    do { \
        LOG_INFO("Initializing %s", component); \
    } while(0)
