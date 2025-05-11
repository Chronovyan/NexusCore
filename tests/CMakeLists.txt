cmake_minimum_required(VERSION 3.14)

# Include directories needed for tests
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Individual test executables
add_executable(SimpleCommandTest SimpleCommandTest.cpp)
target_link_libraries(SimpleCommandTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(CommandManagerTests CommandManagerTests.cpp)
target_link_libraries(CommandManagerTests PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(ComprehensiveUndoRedoTest ComprehensiveUndoRedoTest.cpp)
target_link_libraries(ComprehensiveUndoRedoTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(SelectionClipboardTest SelectionClipboardTest.cpp)
target_link_libraries(SelectionClipboardTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(EditorTest EditorTest.cpp)
target_link_libraries(EditorTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(TextBufferTest TextBufferTest.cpp)
target_link_libraries(TextBufferTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

# Other test files for individual running
add_executable(FutureFeatureTests FutureFeatureTests.cpp)
target_link_libraries(FutureFeatureTests PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(UndoRedoTest UndoRedoTest.cpp)
target_link_libraries(UndoRedoTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(ExitTest ExitTest.cpp)
target_link_libraries(ExitTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(ManualUndoRedoTest ManualUndoRedoTest.cpp)
target_link_libraries(ManualUndoRedoTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

# Main test executor with all tests
add_executable(RunAllTests
    RunAllTests.cpp
    ComprehensiveFunctionalityTest.cpp
    PerformanceBenchmark.cpp
    editor_file_io_test.cpp
    cpp_highlighter_multiline_test.cpp
    fuzz_testing.cpp
    memory_leak_test.cpp
    syntax_highlighting_test.cpp
    simple_syntax_test.cpp
    simplified_syntax_test.cpp
    command_join_lines_test.cpp
    command_insert_text_test.cpp
    command_delete_char_test.cpp
    command_replace_test.cpp
    command_new_line_test.cpp
    command_delete_line_test.cpp
    command_clipboard_operations_test.cpp
)

# Add GMock to RunAllTests
target_link_libraries(RunAllTests PRIVATE EditorLib GTest::gtest GTest::gtest_main GTest::gmock)

# Add test discovery via CTest
include(GoogleTest)
gtest_discover_tests(RunAllTests)

message(STATUS "Configured test executables for TextEditor") 