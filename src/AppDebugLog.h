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
    ErrorValue
};

// Internal implementation for logging
inline void logImpl(LogLevel level, const char* file, int line, const char* format, ...) {
    // Get current time
    time_t now = time(nullptr);
    tm timeinfo;
    #ifdef _WIN32
    localtime_s(&timeinfo, &now);
    #else
    // Use localtime_r for POSIX systems
    localtime_r(&now, &timeinfo);
    #endif
    char timeBuffer[80];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    // Convert level to string
    const char* levelStr = "";
    switch (level) {
        case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
        case LogLevel::INFO:    levelStr = "INFO"; break;
        case LogLevel::WARNING: levelStr = "WARNING"; break;
        case LogLevel::ErrorValue: levelStr = "ERROR"; break;
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

// Overload for std::string messages
inline void logImpl(LogLevel level, const char* file, int line, const std::string& message) {
    // Call the original implementation with c_str()
    logImpl(level, file, line, "%s", message.c_str());
}

// Macros for easy logging
#ifndef LOG_INIT
  #define LOG_INIT(component) initAppDebugLog(); LOG_INFO("Initializing %s", component)
#endif

// Fix for the broken macros: ensure proper handling of variadic arguments with correct syntax
// Using double parentheses to ensure proper expansion of the arguments
#ifndef LOG_DEBUG
  #define LOG_DEBUG(...) logImpl(LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef LOG_INFO
  #define LOG_INFO(...) logImpl(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef LOG_WARNING
  #define LOG_WARNING(...) logImpl(LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef LOG_ERROR
  #define LOG_ERROR(...) logImpl(LogLevel::ErrorValue, __FILE__, __LINE__, __VA_ARGS__)
#endif
