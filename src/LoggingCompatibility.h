#pragma once

/**
 * @file LoggingCompatibility.h
 * @brief Compatibility layer to handle conflicts between different logging systems.
 * 
 * This file should be included in source files that use logging and might encounter
 * conflicts between different logging systems (e.g., between AppDebugLog.h and di/AppDebugLog.h).
 * It undefines any existing LOG_* macros and includes the main logging header.
 */

// Undefine any existing logging macros to avoid conflicts
#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif

#ifdef LOG_INFO
#undef LOG_INFO
#endif

#ifdef LOG_WARNING
#undef LOG_WARNING
#endif

#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

// Include the main logging header
#include "AppDebugLog.h"

// Define our own macros to ensure consistent behavior
// Using the same format as in AppDebugLog.h with proper variadic arguments
#define LOG_DEBUG(...) logImpl(LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) logImpl(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) logImpl(LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logImpl(LogLevel::ErrorValue, __FILE__, __LINE__, __VA_ARGS__)

// No need to redefine the macros, they're already defined in AppDebugLog.h
// But we could add extra compatibility macros here if needed in the future 