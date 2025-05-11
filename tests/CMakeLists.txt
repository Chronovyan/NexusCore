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

add_executable(ExitTest ExitTest.cpp)
target_link_libraries(ExitTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

# Comprehensive all-tests executable (for GTest-based tests)
add_executable(RunAllTests 
    RunAllTests.cpp # Contains the GTest main function
    cpp_highlighter_multiline_test.cpp 
    fuzz_testing.cpp
    command_join_lines_test.cpp
    command_insert_text_test.cpp
    command_delete_char_test.cpp
    # CommandManagerTests.cpp # This has its own main, remove
    # ComprehensiveUndoRedoTest.cpp # This has its own main, remove
    # SelectionClipboardTest.cpp # This has its own main, remove
    # ExitTest.cpp # This has its own main, remove
)
target_link_libraries(RunAllTests PRIVATE EditorLib GTest::gtest GTest::gtest_main GTest::gmock)

# Add tests to CTest
add_test(NAME SimpleCommandTest COMMAND SimpleCommandTest)
add_test(NAME CommandManagerTests COMMAND CommandManagerTests)
add_test(NAME ComprehensiveUndoRedoTest COMMAND ComprehensiveUndoRedoTest)
add_test(NAME SelectionClipboardTest COMMAND SelectionClipboardTest)
add_test(NAME ExitTest COMMAND ExitTest)
add_test(NAME RunAllTests COMMAND RunAllTests)

message(STATUS "Configured test executables for TextEditor") 