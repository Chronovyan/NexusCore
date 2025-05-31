#pragma once

// Thread safety configuration for the DI container
// This file defines the thread safety configuration for the DI container.

#include <mutex>
#include <shared_mutex>

namespace di {
namespace lifetime {

// Thread safety typedefs to ensure compatibility with compilers that don't support C++17 features
#if __cplusplus >= 201703L || (defined(_MSC_VER) && _MSC_VER >= 1914)
    // C++17 or newer
    using shared_mutex = std::shared_mutex;
    using shared_lock = std::shared_lock<std::shared_mutex>;
    using unique_lock = std::unique_lock<std::shared_mutex>;
#else
    // C++14 or older
    using shared_mutex = std::mutex;  // Fallback for older compilers
    using shared_lock = std::unique_lock<std::mutex>;
    using unique_lock = std::unique_lock<std::mutex>;
#endif

} // namespace lifetime
} // namespace di 