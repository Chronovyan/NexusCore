#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>

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
 * @brief Log a debug message to the console
 * 
 * Outputs a debug message with timestamp to standard output.
 * 
 * @param msg The message to log
 */
inline void debug(const std::string& msg) {
    std::cout << "[" << getTimestamp() << "] [DEBUG] " << msg << std::endl;
}

/**
 * @brief Log an info message to the console
 * 
 * Outputs an info message with timestamp to standard output.
 * 
 * @param msg The message to log
 */
inline void info(const std::string& msg) {
    std::cout << "[" << getTimestamp() << "] [INFO] " << msg << std::endl;
}

/**
 * @brief Log an error message to the console
 * 
 * Outputs an error message with timestamp to standard error.
 * 
 * @param msg The message to log
 */
inline void error(const std::string& msg) {
    std::cerr << "[" << getTimestamp() << "] [ERROR] " << msg << std::endl;
}

} // namespace log
} // namespace di

// Simple logging macros for the DI framework
// These macros make it easy to include logging in the DI framework
// while allowing for the possibility of disabling debug logs in production

#ifndef NDEBUG
#define LOG_DEBUG(msg) di::log::debug(msg)
#else
#define LOG_DEBUG(msg) ((void)0)
#endif

#define LOG_INFO(msg) di::log::info(msg)
#define LOG_ERROR(msg) di::log::error(msg) 