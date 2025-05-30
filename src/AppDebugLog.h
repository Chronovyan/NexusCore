#pragma once

#include <iostream>
#include <string>
#include <cstdio>
#include <ctime>
#include <cstdarg>  // Required for va_start, va_end, va_list

// Initialize the debug log
inline void initAppDebugLog() {
    // You could initialize a file logger here if needed
    std::cout << "Debug log initialized" << std::endl;
}

// Debug log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Internal implementation for logging
inline void logImpl(LogLevel level, const char* file, int line, const char* format, ...) {
    // Get current time
    time_t now = time(nullptr);
    tm* timeinfo = localtime(&now);
    char timeBuffer[80];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Convert level to string
    const char* levelStr = "";
    switch (level) {
        case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
        case LogLevel::INFO:    levelStr = "INFO"; break;
        case LogLevel::WARNING: levelStr = "WARNING"; break;
        case LogLevel::ERROR:   levelStr = "ERROR"; break;
    }
    
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
    
    // Format log prefix
    char prefixBuffer[512];
    snprintf(prefixBuffer, sizeof(prefixBuffer), "[%s] %s %s:%d: ", 
             timeBuffer, levelStr, filename, line);
    
    // Format the message
    char messageBuffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);
    
    // Print to console (could also write to file)
    std::cout << prefixBuffer << messageBuffer << std::endl;
}

// Macros for easy logging
#define LOG_DEBUG(format, ...) logImpl(LogLevel::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) logImpl(LogLevel::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) logImpl(LogLevel::WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) logImpl(LogLevel::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
