cmake_minimum_required(VERSION 3.14)
project(TextEditorTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enable testing
enable_testing()

# Find required packages
find_package(Threads REQUIRED)

# Download and build Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Make GoogleTest available
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Include GoogleTest module
include(GoogleTest)

# Add TextBuffer library
add_library(TextBuffer
    ../src/TextBuffer.cpp
    ../src/ErrorReporter.cpp
)

target_include_directories(TextBuffer
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/../include
)

# Download nlohmann_json header
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE ${json_SOURCE_DIR}/include)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

# Add Editor library
add_library(EditorLib
    ../src/Editor.cpp
    ../src/EditorCommands.cpp
    ../src/ModernEditorCommands.cpp
    ../src/SyntaxHighlighter.cpp
    ../src/SyntaxHighlightingManager.cpp
    ../src/MultiCursor.cpp
    ../src/EditorDiffMerge.cpp
    ../src/ThreadSafeTextBuffer.cpp
    ../src/ThreadSafeVirtualizedTextBuffer.cpp
    ../src/VirtualizedTextBuffer.cpp
    ../src/TextBufferOperationQueue.cpp
    ../src/EditorCoreThreadPool.cpp
    ../src/ThreadPool.cpp
    ../src/CodeContextProvider.cpp
    ../src/LanguageDetector.cpp
    ../src/BaseLanguageParser.cpp
    ../src/CStyleLanguageParser.cpp
)

target_include_directories(EditorLib
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/../src/editor
        ${CMAKE_SOURCE_DIR}/../src/editor/core
        ${CMAKE_SOURCE_DIR}/../src/editor/commands
        ${CMAKE_SOURCE_DIR}/../src/editor/features
        ${CMAKE_SOURCE_DIR}/../src/editor/rendering
        ${CMAKE_SOURCE_DIR}/../src/editor/undo
        ${CMAKE_SOURCE_DIR}/../src/editor/utils
        ${json_SOURCE_DIR}/include
)

target_link_libraries(EditorLib
    PRIVATE
        TextBuffer
        Threads::Threads
        nlohmann_json::nlohmann_json
)

# Add test executables
function(add_gtest_test TEST_NAME)
    add_executable(${TEST_NAME} ../tests/${TEST_NAME}.cpp)
    target_include_directories(${TEST_NAME} 
        PRIVATE 
            ${CMAKE_SOURCE_DIR}/../src
            ${CMAKE_SOURCE_DIR}/../include
            ${json_SOURCE_DIR}/include
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE
            gtest_main
            gtest
            TextBuffer
            EditorLib
            Threads::Threads
            nlohmann_json::nlohmann_json
    )
    gtest_discover_tests(${TEST_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Add all test executables
add_gtest_test(SimpleEditorTest)
add_gtest_test(TextBufferTest)
add_gtest_test(TextBufferThreadSafetyTest)
add_gtest_test(TextBufferFileIOTest)
add_gtest_test(EditorBasicTest)

# Add a custom target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS 
        SimpleEditorTest
        TextBufferTest
        TextBufferThreadSafetyTest
        TextBufferFileIOTest
        EditorBasicTest
    COMMENT "Running all tests..."
)
