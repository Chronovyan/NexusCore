cmake_minimum_required(VERSION 3.14)
project(CurlTest VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CPR path (which includes its own curl)
set(CPR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/cpr)

# Allow CPR to find and use its bundled curl
set(CPR_ENABLE_CURL_SIGNAL_HANDLING OFF CACHE INTERNAL "")
set(CPR_BUILD_TESTS OFF CACHE INTERNAL "")
set(CPR_BUILD_DOCS OFF CACHE INTERNAL "")
set(CPR_FORCE_USE_SYSTEM_CURL OFF CACHE INTERNAL "")

# Add CPR as a subdirectory, with explicit binary directory
add_subdirectory(${CPR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/cpr_build)

# Create the test executable
add_executable(curl_test ../src/curl_test.cpp)

# Link against cpr (which links against curl internally)
target_link_libraries(curl_test PRIVATE cpr::cpr)

message(STATUS "Configured with CPR's bundled curl") 