#pragma once

/**
 * @file LoggingCompat.h
 * @brief Compatibility layer for the logging system
 * 
 * This file ensures consistent logging macro definitions across the application
 * and prevents macro redefinition warnings.
 */

// Only define these macros if they haven't been defined by Logging.h
#ifndef LOGGING_DEFINED

// Undefine any existing logging macros to prevent conflicts
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

#ifdef LOG_CRITICAL
#undef LOG_CRITICAL
#endif

// Include the unified logging system
#include "Logging.h"

// Mark that we've defined the logging macros
#define LOGGING_DEFINED

#endif // LOGGING_DEFINED
