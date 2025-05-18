#ifndef MOCK_UTILS_H
#define MOCK_UTILS_H

#include "gmock/gmock.h"

// Import the necessary GMock symbols directly
namespace testing {
namespace internal {
    // Define the missing symbols from GoogleMock
    extern Mutex g_gmock_mutex;
    extern ThreadLocal<Sequence*> g_gmock_implicit_sequence;
} // namespace internal
} // namespace testing

#endif // MOCK_UTILS_H 