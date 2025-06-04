cmake_minimum_required(VERSION 3.10)
project(MinimalTest VERSION 1.0 LANGUAGES CXX)

# GoogleTest path
set(GTEST_DIR ${CMAKE_SOURCE_DIR}/../external/googletest)

# Set compiler flags based on compiler being used
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Use clang-specific compiler flags
    message(STATUS "Using Clang compiler flags with language extension exception")
    add_compile_options(-Wno-language-extension-token)
endif()

# GoogleTest Configuration
set(gtest_force_shared_crt ON CACHE BOOL "Use shared runtime libraries")
add_subdirectory(${GTEST_DIR} googletest)

# Add test executable
add_executable(MinimalTest MinimalTest.cpp)
target_link_libraries(MinimalTest gtest gtest_main)

# Enable testing
enable_testing()
add_test(NAME MinimalTest COMMAND MinimalTest)
