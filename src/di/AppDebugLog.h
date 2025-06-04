#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdarg>  // Required for va_start, va_end, va_list

namespace di {
namespace log {

/**
 * @brief Get current timestamp as a formatted string
 * 
 * This function returns the current time as a formatted string.
 * It uses a thread-safe approach appropriate for the platform.
 * 
 * @return Formatted timestamp string in "YYYY-MM-DD HH:MM:SS" format
 */
inline std::string getTimestamp() {
    const time_t now = time(nullptr);
    
#ifdef _WIN32
    // Windows-specific thread-safe implementation
    struct tm timeinfo_local;
    localtime_s(&timeinfo_local, &now);
    
    std::ostringstream oss;
    oss << std::put_time(&timeinfo_local, "%Y-%m-%d %H:%M:%S");
    return oss.str();
#else
    // POSIX implementation
    struct tm timeinfo_local;
    localtime_r(&now, &timeinfo_local);
    
    std::ostringstream oss;
    oss << std::put_time(&timeinfo_local, "%Y-%m-%d %H:%M:%S");
    return oss.str();
#endif
}

/**
 * @brief Internal implementation for variadic logging
 * 
 * This function formats a log message with the provided format string and arguments.
 * 
 * @param level The log level (DEBUG, INFO, ERROR)
 * @param format The format string
 * @param ... The variadic arguments for format
 */
inline void logImplFormatted(const char* level, const char* format, ...) {
    // Format buffer
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Output the formatted message
    std::cout << "[" << getTimestamp() << "] [" << level << "] " << buffer << std::endl;
}

/**
 * @brief Log a debug message to the console
 * 
 * Outputs a debug message with timestamp to standard output.
 * 
 * @param format The format string
 * @param ... The variadic arguments for format
 */
inline void debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::cout << "[" << getTimestamp() << "] [DEBUG] " << buffer << std::endl;
}

/**
 * @brief Log an info message to the console
 * 
 * Outputs an info message with timestamp to standard output.
 * 
 * @param format The format string
 * @param ... The variadic arguments for format
 */
inline void info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::cout << "[" << getTimestamp() << "] [INFO] " << buffer << std::endl;
}

/**
 * @brief Log an error message to the console
 * 
 * Outputs an error message with timestamp to standard error.
 * 
 * @param format The format string
 * @param ... The variadic arguments for format
 */
inline void error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::cerr << "[" << getTimestamp() << "] [ERROR] " << buffer << std::endl;
}

// Overloads for std::string messages
inline void debug(const std::string& msg) {
    debug("%s", msg.c_str());
}

inline void info(const std::string& msg) {
    info("%s", msg.c_str());
}

inline void error(const std::string& msg) {
    error("%s", msg.c_str());
}

} // namespace log
} // namespace di

// Simple logging macros for the DI framework
// These macros make it easy to include logging in the DI framework
// while allowing for the possibility of disabling debug logs in production

#ifndef LOG_DEBUG
  #ifndef NDEBUG
    #define LOG_DEBUG(...) di::log::debug(__VA_ARGS__)
  #else
    #define LOG_DEBUG(...) ((void)0)
  #endif
#endif

#ifndef LOG_INFO
  #define LOG_INFO(...) di::log::info(__VA_ARGS__)
#endif

#ifndef LOG_ERROR
  #define LOG_ERROR(...) di::log::error(__VA_ARGS__)
#endif 