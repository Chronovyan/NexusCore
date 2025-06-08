cmake_minimum_required(VERSION 3.14)
project(MinimalTest)

# Enable testing
enable_testing()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download and include Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add the test executable
add_executable(
  MinimalTest
  minimal_test.cpp
)

# Link with Google Test
target_link_libraries(
  MinimalTest
  PRIVATE
    GTest::gtest_main
)

# Add test
include(GoogleTest)
gtest_discover_tests(MinimalTest)
