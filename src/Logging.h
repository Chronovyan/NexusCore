#pragma once

// Unified Logging System for the entire project
// This file provides a consistent logging interface that resolves conflicts
// between different logging implementations in the codebase.

#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cstring>

class Logger {
public:
    // Log level enum
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        LOG_ERROR,  // Renamed from ERROR to avoid conflict with Windows.h macro
        CRITICAL
    };

private:
    // Inline static members (C++17 feature)
    inline static std::mutex logMutex_;
    inline static Level minLogLevel_ = Level::INFO;
    inline static bool useColors_ = true;

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

    // Convert log level to string
    static const char* levelToString(Level level) {
        switch (level) {
            case Level::DEBUG:    return "DEBUG";
            case Level::INFO:     return "INFO";
            case Level::WARNING:  return "WARNING";
            case Level::LOG_ERROR: return "ERROR";
            case Level::CRITICAL: return "CRITICAL";
            default:              return "UNKNOWN";
        }
    }

    // Get color code for log level (for terminal output)
    static const char* getLevelColor(Level level) {
        if (!useColors_) return "";
        
        switch (level) {
            case Level::DEBUG:    return "\033[36m"; // Cyan
            case Level::INFO:     return "\033[32m"; // Green
            case Level::WARNING:  return "\033[33m"; // Yellow
            case Level::LOG_ERROR: return "\033[31m"; // Red
            case Level::CRITICAL: return "\033[1;31m"; // Bold Red
            default:              return "\033[0m";
        }
    }

    
    static const char* getResetColor() {
        return useColors_ ? "\033[0m" : "";
    }

public:
    // Set the minimum log level (messages below this level will be ignored)
    static void setLogLevel(Level level) {
        std::lock_guard<std::mutex> lock(logMutex_);
        minLogLevel_ = level;
    }
    
    // Get the current minimum log level
    static Level getLogLevel() {
        std::lock_guard<std::mutex> lock(logMutex_);
        return minLogLevel_;
    }
    
    // Enable/disable colored output
    static void setUseColors(bool enabled) {
        std::lock_guard<std::mutex> lock(logMutex_);
        useColors_ = enabled;
    }
    
    // Log a message with the specified level
    template<typename... Args>
    static void log(Level level, const char* file, int line, const std::string& format, Args... args) {
        if (level < minLogLevel_) return;
        
        char buffer[4096];
        // Use a format string to avoid the warning
        std::string fmt = format;
        if constexpr (sizeof...(args) > 0) {
            snprintf(buffer, sizeof(buffer), fmt.c_str(), args...);
        } else {
            snprintf(buffer, sizeof(buffer), "%s", fmt.c_str());
        }
        
        std::lock_guard<std::mutex> lock(logMutex_);
        
        // Extract filename from path
        const char* filename = file;
        const char* lastSlash = strrchr(file, '/');
        if (!lastSlash) lastSlash = strrchr(file, '\\');
        if (lastSlash) filename = lastSlash + 1;
        
        // Format: [timestamp] [LEVEL] file:line - message
        std::cerr << getTimestamp() << " "
                  << getLevelColor(level) << "[" << levelToString(level) << "]" << getResetColor() << " "
                  << filename << ":" << line << " - " << buffer << std::endl;
    }
};

// Logging macros
#define LOG_DEBUG(format, ...)    Logger::log(Logger::Level::DEBUG,    __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)     Logger::log(Logger::Level::INFO,     __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)  Logger::log(Logger::Level::WARNING,  __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)    Logger::log(Logger::Level::LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_CRITICAL(format, ...) Logger::log(Logger::Level::CRITICAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

// For backward compatibility
#define LOG_INIT(component) LOG_INFO("Initializing %s", component)
