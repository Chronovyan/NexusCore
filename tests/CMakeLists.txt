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

add_executable(EditorApiExtensionsTest EditorApiExtensionsTest.cpp)
target_link_libraries(EditorApiExtensionsTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

# Dependency Injection Framework Test
add_executable(DependencyInjectionTest DependencyInjectionTest.cpp)
target_link_libraries(DependencyInjectionTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

# OpenAI API endpoints test
add_executable(OpenAIApiEndpointsTest 
  OpenAIApiEndpointsTest.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
)
target_link_libraries(OpenAIApiEndpointsTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# OpenAI Schema Validation Test
add_executable(OpenAISchemaValidationTest 
  OpenAISchemaValidationTest.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.cpp
)
target_link_libraries(OpenAISchemaValidationTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# OpenAI API Error Handling Test
add_executable(OpenAIApiErrorHandlingTest 
  OpenAIApiErrorHandlingTest.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.cpp
)
target_link_libraries(OpenAIApiErrorHandlingTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# OpenAI API Retry Statistics Test
add_executable(OpenAIApiRetryStatisticsTest 
  OpenAIApiRetryStatisticsTest.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.cpp
)
target_link_libraries(OpenAIApiRetryStatisticsTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# OpenAI API Extended Mock Tests
add_executable(OpenAIApiExtendedMockTests 
  OpenAIApiExtendedMockTests.cpp
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.cpp
  ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
  ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
)
target_link_libraries(OpenAIApiExtendedMockTests PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    nlohmann_json::nlohmann_json
)

# AI Agent Orchestrator tests
add_executable(AIAgentOrchestratorTest 
    AIAgentOrchestratorTest.cpp
    ${CMAKE_SOURCE_DIR}/src/AIAgentOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/AIAgentOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/WorkspaceManager.cpp
    ${CMAKE_SOURCE_DIR}/src/WorkspaceManager.h
    ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.cpp
    ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.h
    ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.cpp
    ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.h
    ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
    ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
    ${CMAKE_SOURCE_DIR}/src/UIModel.h
)
target_link_libraries(AIAgentOrchestratorTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    cpr::cpr
)

# Other test files for individual running
add_executable(FutureFeatureTests FutureFeatureTests.cpp)
target_link_libraries(FutureFeatureTests PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(UndoRedoTest UndoRedoTest.cpp)
target_link_libraries(UndoRedoTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(ExitTest ExitTest.cpp)
target_link_libraries(ExitTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

add_executable(ManualUndoRedoTest ManualUndoRedoTest.cpp)
target_link_libraries(ManualUndoRedoTest PRIVATE EditorLib GTest::gtest GTest::gtest_main)

# Error Reporter Test with File Logging
add_executable(ErrorReporterTest ErrorReporterTest.cpp)
target_link_libraries(ErrorReporterTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

# Add sample critical test
add_executable(SampleCriticalTest sample_critical_test.cpp)
target_link_libraries(SampleCriticalTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

# Large file performance tests
add_executable(LargeFilePerformanceTest large_file_performance_test.cpp)
target_link_libraries(LargeFilePerformanceTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

# Extreme large file performance tests (>10MB)
add_executable(ExtremeLargeFileTest extreme_large_file_test.cpp test_file_utilities.cpp)
target_link_libraries(ExtremeLargeFileTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

# Automated versions of manual tests
add_executable(AutomatedUndoRedoTest AutomatedUndoRedoTest.cpp)
target_link_libraries(AutomatedUndoRedoTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

add_executable(AutomatedSearchTest AutomatedSearchTest.cpp)
target_link_libraries(AutomatedSearchTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

add_executable(AutomatedSyntaxHighlightingTest AutomatedSyntaxHighlightingTest.cpp)
target_link_libraries(AutomatedSyntaxHighlightingTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

add_executable(AutomatedConcurrencyTest AutomatedConcurrencyTest.cpp)
target_link_libraries(AutomatedConcurrencyTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
)

# Editor Dependency Injection Test
add_executable(EditorDependencyInjectionTest EditorDependencyInjectionTest.cpp)
target_link_libraries(EditorDependencyInjectionTest PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
)

# Main test executor with all tests
add_executable(RunAllTests
    RunAllTests.cpp
    MockUtils.cpp
    ComprehensiveFunctionalityTest.cpp
    PerformanceBenchmark.cpp
    editor_file_io_test.cpp
    editor_facade_test.cpp
    cpp_highlighter_multiline_test.cpp
    fuzz_testing.cpp
    memory_leak_test.cpp
    syntax_highlighting_test.cpp
    simple_syntax_test.cpp
    simplified_syntax_test.cpp
    command_join_lines_test.cpp
    command_insert_text_test.cpp
    command_delete_char_test.cpp
    command_backspace_test.cpp
    command_forward_delete_test.cpp
    command_replace_test.cpp
    command_new_line_test.cpp
    command_delete_line_test.cpp
    command_clipboard_operations_test.cpp
    command_find_replace_test.cpp
    command_compound_test.cpp
    syntax_highlighting_manager_test.cpp
    TypeTextCommandTest.cpp
    editor_commands_test.cpp
    TextBufferTest.cpp
    EditorApiExtensionsTest.cpp
    AIAgentOrchestratorTest.cpp
    OpenAISchemaValidationTest.cpp
    OpenAIApiErrorHandlingTest.cpp
    OpenAIApiExtendedMockTests.cpp
    ErrorReporterTest.cpp
    DependencyInjectionTest.cpp
    AutomatedUndoRedoTest.cpp
    AutomatedSearchTest.cpp
    AutomatedSyntaxHighlightingTest.cpp
    AutomatedConcurrencyTest.cpp
    EditorDependencyInjectionTest.cpp
    ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.cpp
    ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client.h
    ${CMAKE_SOURCE_DIR}/src/IOpenAI_API_Client.h
    ${CMAKE_SOURCE_DIR}/src/OpenAI_API_Client_types.h
    ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.cpp
    ${CMAKE_SOURCE_DIR}/src/MockOpenAI_API_Client.h
    ${CMAKE_SOURCE_DIR}/src/AIAgentOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/AIAgentOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/WorkspaceManager.cpp
    ${CMAKE_SOURCE_DIR}/src/WorkspaceManager.h
)

# Define the RUN_ALL_TESTS_INCLUDE macro for the RunAllTests target to avoid duplicate main functions
target_compile_definitions(RunAllTests PRIVATE RUN_ALL_TESTS_INCLUDE)

# Add GMock to RunAllTests
target_link_libraries(RunAllTests PRIVATE 
    EditorLib 
    GTest::gtest 
    GTest::gtest_main 
    GTest::gmock
    GTest::gmock_main
    nlohmann_json::nlohmann_json
    cpr::cpr
)

# Add test discovery via CTest
include(GoogleTest)
gtest_discover_tests(RunAllTests)
gtest_discover_tests(OpenAIApiEndpointsTest)
gtest_discover_tests(OpenAISchemaValidationTest)
gtest_discover_tests(OpenAIApiErrorHandlingTest)
gtest_discover_tests(OpenAIApiRetryStatisticsTest)
gtest_discover_tests(OpenAIApiExtendedMockTests)
gtest_discover_tests(AIAgentOrchestratorTest)
gtest_discover_tests(ErrorReporterTest)
gtest_discover_tests(LargeFilePerformanceTest)
gtest_discover_tests(ExtremeLargeFileTest)
gtest_discover_tests(AutomatedUndoRedoTest)
gtest_discover_tests(AutomatedSearchTest)
gtest_discover_tests(AutomatedSyntaxHighlightingTest)
gtest_discover_tests(AutomatedConcurrencyTest)
gtest_discover_tests(EditorDependencyInjectionTest)

message(STATUS "Configured test executables for TextEditor") 