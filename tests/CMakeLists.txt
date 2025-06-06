cmake_minimum_required(VERSION 3.14)
project(TextEditorTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Download and build Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add SimpleEditorTest (self-contained test)
add_executable(SimpleEditorTest
  SimpleEditorTest.cpp
)

target_link_libraries(SimpleEditorTest
  PRIVATE
    GTest::gtest_main
)

# Add tests
include(GoogleTest)
gtest_discover_tests(SimpleEditorTest)
