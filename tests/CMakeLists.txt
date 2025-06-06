cmake_minimum_required(VERSION 3.14)
project(TextEditorTests)

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

# Include directories
include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/interfaces
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/tests
)

# Add the minimal test
add_executable(MinimalTest MinimalDocumentTest.cpp)
target_link_libraries(MinimalTest
  PRIVATE
    GTest::gtest_main
)

# Add mock Editor test
add_executable(EditorMockTest 
  EditorMockTest.cpp
  MockEditor.h
  IEditorTestInterface.hpp
)

target_link_libraries(EditorMockTest
  PRIVATE
    GTest::gtest_main
    GTest::gmock
)

# Add tests
include(GoogleTest)
gtest_discover_tests(MinimalTest)
gtest_discover_tests(EditorMockTest)
