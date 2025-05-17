# CPR vs Direct libcurl: Implementation Findings

## Overview
This document summarizes our tests and findings regarding the two approaches for HTTP requests in the OpenAI API client:
1. Using CPR (C++ Requests) library
2. Direct libcurl implementation

## CPR Approach

### Advantages
- Higher-level API that is more readable and maintainable
- Simpler code with fewer lines
- Built-in support for modern C++ features (e.g., std::string, smart pointers)
- Handles memory management automatically
- Comes with its own bundled curl implementation

### Implementation Details
```cpp
cpr::Response httpResponse = cpr::Post(
    cpr::Url{apiUrl_},
    headers_,
    cpr::Body{requestBodyStr},
    cpr::Timeout{30000} // 30 seconds timeout
);
```

### Build Process
- Add CPR as a subdirectory in CMake
- Set CPR options such as `CPR_FORCE_USE_SYSTEM_CURL=OFF`
- Link against `cpr::cpr` target

## Direct libcurl Approach

### Advantages
- More fine-grained control over HTTP requests
- One less dependency (no CPR)
- Potential for slightly better performance
- Direct access to all libcurl features

### Implementation Details
```cpp
CURL* curl = curl_easy_init();
curl_easy_setopt(curl, CURLOPT_URL, apiUrl_.c_str());
curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBodyStr.c_str());
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
res = curl_easy_perform(curl);
```

### Build Process
- Find libcurl package in CMake
- Link against `CURL::libcurl` target
- Manage curl initialization/cleanup

## Testing Findings

1. **CPR Works Well**: The CPR implementation built and ran successfully, with CPR's bundled curl.

2. **Direct libcurl Had Issues**: 
   - System-wide libcurl was not found by CMake
   - Using `find_package(CURL REQUIRED)` failed

3. **Workaround for Direct libcurl**:
   - Either install libcurl properly on the system
   - Or use CPR's bundled curl but with your own direct libcurl code

## Conclusion

For this project, the CPR approach appears to be more reliable and simpler to use. The direct libcurl approach would require:

1. Proper installation of libcurl on the system, or
2. Custom configuration to use CPR's bundled curl, or
3. Bundling libcurl directly with the project

Given these findings, we recommend continuing with the CPR implementation for the OpenAI API client unless there's a strong reason to switch to direct libcurl. 