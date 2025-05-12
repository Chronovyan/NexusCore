#ifndef THREAD_SAFETY_CONFIG_H
#define THREAD_SAFETY_CONFIG_H
#include <mutex>

/**
 * Thread Safety Configuration
 * 
 * This header provides compile-time configuration for thread-safety features
 * in the codebase. It allows conditional compilation of advanced C++17 threading
 * features based on compiler support and application needs.
 */

// Detect C++17 support automatically
#if __cplusplus >= 201703L
    #define CPP17_FEATURES_AVAILABLE 1
#else
    // Microsoft Visual Studio doesn't correctly set __cplusplus by default
    // So we need to check for _MSC_VER
    #if defined(_MSC_VER) && _MSC_VER >= 1914 // Visual Studio 2017 version 15.7 or higher
        #if defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
            #define CPP17_FEATURES_AVAILABLE 1
        #else
            #define CPP17_FEATURES_AVAILABLE 0
        #endif
    #else
        #define CPP17_FEATURES_AVAILABLE 0
    #endif
#endif

// Enable or disable full thread safety
// Set to 0 to use simpler threading primitives that are more widely compatible
// Set to 1 to use full C++17 thread safety features (requires C++17 compiler)
#if defined(_MSC_VER) && _MSC_VER >= 1914
    // Visual Studio has good C++17 support, so enable full thread safety
    #define ENABLE_FULL_THREAD_SAFETY 1
#else
    // Use automatic detection for other compilers
    #define ENABLE_FULL_THREAD_SAFETY CPP17_FEATURES_AVAILABLE
#endif

// Enable or disable thread debugging
// Set to 0 to disable thread-related logging and debugging
// Set to 1 to enable detailed thread debugging output
#define ENABLE_THREAD_DEBUGGING 0

// Configure thread safety primitives based on feature availability
#if ENABLE_FULL_THREAD_SAFETY
    #include <shared_mutex>
    #include <thread>
    
    // Use C++17 shared mutex for reader-writer lock
    #define READER_WRITER_MUTEX std::shared_mutex
    #define READ_LOCK(mutex) std::shared_lock<std::shared_mutex> lock(mutex)
    #define WRITE_LOCK(mutex) std::unique_lock<std::shared_mutex> lock(mutex)
    #define SCOPED_LOCK(mutex) std::scoped_lock lock(mutex);
    
    // Thread debugging helpers
    #if ENABLE_THREAD_DEBUGGING
        #define THREAD_DEBUG(message) std::cout << "[DEBUG THREAD " << std::this_thread::get_id() << "] " << message << std::endl
    #else
        #define THREAD_DEBUG(message) ((void)0) // No-op
    #endif
#else
    // <mutex> is already included at the top of the file.
    // No other includes are needed here for the C++11 fallback.

    // Use standard mutex with simplified locking patterns
    #define READER_WRITER_MUTEX std::mutex
    #define READ_LOCK(mutex) std::lock_guard<std::mutex> lock(mutex)
    #define WRITE_LOCK(mutex) std::lock_guard<std::mutex> lock(mutex)
    #define SCOPED_LOCK(mutex) std::lock_guard<std::mutex> lock(mutex);

    // Thread debugging not available
    #define THREAD_DEBUG(message) ((void)0) // No-op
#endif

#endif // THREAD_SAFETY_CONFIG_H 