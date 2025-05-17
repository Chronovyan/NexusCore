# CPR Implementation for HTTP Requests

## Overview

This document explains the implementation of HTTP requests in the AI-First TextEditor using the CPR (C++ Requests) library. After evaluating both direct libcurl and CPR approaches, we determined that CPR provides the most reliable and maintainable solution for our OpenAI API client.

## Why CPR?

CPR (C++ Requests) is a modern C++ wrapper around libcurl that provides a clean, user-friendly API for making HTTP requests. Our evaluation found that:

1. **Higher-level API**: CPR provides a more intuitive and readable API compared to direct libcurl
2. **Memory Management**: CPR handles many memory management concerns automatically
3. **Built-in Features**: Headers, authentication, timeouts, and error handling are simplified
4. **Bundled curl**: CPR comes with its own bundled curl implementation, avoiding system dependency issues
5. **Modern C++ Design**: Uses modern C++ features like smart pointers and std::string

## Implementation Details

Our OpenAI API client uses CPR for all HTTP communication. The core implementation in `OpenAI_API_Client.cpp` uses:

```cpp
cpr::Response httpResponse = cpr::Post(
    cpr::Url{apiUrl_},
    headers_,
    cpr::Body{requestBodyStr},
    cpr::Timeout{30000} // 30 seconds timeout
);
```

## Build Configuration

In our CMakeLists.txt, CPR is included as a subdirectory and configured with:

```cmake
# --- CPR (C++ Requests) for HTTP ---
set(CPR_BUILD_TESTS OFF CACHE INTERNAL "Build CPR tests")
set(CPR_BUILD_TESTS_SSL OFF CACHE INTERNAL "Build CPR SSL tests")
set(CPR_ENABLE_CERTIFICATE_OPTIMIZATION OFF CACHE INTERNAL "Enable certificate optimization (CPR)")
set(CPR_FORCE_USE_SYSTEM_CURL OFF CACHE INTERNAL "Use system curl instead of built-in")
add_subdirectory(${CPR_DIR})
```

And linked to our executables with:

```cmake
target_link_libraries(OpenAIClientTest PRIVATE cpr::cpr nlohmann_json::nlohmann_json)
```

## Testing

We have created several batch files to test the CPR implementation:

1. `build_with_cpr.bat`: Builds the entire project with CPR
2. `test_openai_client_with_cpr.bat`: Tests the OpenAI client implementation using CPR

## Alternative Approaches Considered

We also explored using libcurl directly, which is documented in `DIRECT_CURL_APPROACH.md`. However, this approach presented several challenges:

1. More complex memory management
2. Verbose error handling
3. System dependency issues on Windows
4. Higher maintenance burden

The comparison between approaches is detailed in `CPR_VS_CURL_FINDINGS.md`.

## Future Considerations

1. **Error Handling**: Enhance error handling for network failures
2. **Timeout Configuration**: Make timeouts configurable
3. **Proxy Support**: Add support for proxy configurations
4. **Rate Limiting**: Implement rate limiting for API requests

## Related Files

- `src/OpenAI_API_Client.cpp`: Main implementation of the OpenAI API client
- `src/OpenAI_API_Client.h`: Header for the OpenAI API client
- `CMakeLists.txt`: Build configuration
- `build_with_cpr.bat`: Build script for the CPR implementation
- `CPR_VS_CURL_FINDINGS.md`: Comparative analysis of CPR vs direct libcurl
- `DIRECT_CURL_APPROACH.md`: Documentation of the alternative libcurl approach 