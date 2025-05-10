# CMakeLists.txt for tests

# Define the test executable
add_executable(runTests
    # Add your test source files here, e.g.:
    sample_test.cpp 
    textbuffer_test.cpp
    editor_commands_test.cpp
    syntax_highlighting_test.cpp
    syntax_highlighting_manager_test.cpp
    editor_file_io_test.cpp
    # command_tests.cpp
    # editor_tests.cpp 
)

# Link against EditorLib to access its components for testing
# and GTest for the testing framework.
# EditorLib should make its include directories (src/) available PUBLICLY.
target_link_libraries(runTests PRIVATE EditorLib gtest_main GTest::gmock)

# Explicitly add include directory for editor headers if not automatically inherited
# from EditorLib. EditorLib has `target_include_directories(EditorLib PUBLIC src)`
# in the parent CMakeLists.txt, so this should make headers from src/ available.
# If includes like #include "TextBuffer.h" fail in test files, ensure that the path
# from the perspective of files in tests/ is correct, or add `target_include_directories(runTests PRIVATE ../src)`.
# For now, relying on EditorLib's public include propagation.

# Add Google Mock include directory explicitly
# Assuming googletest_SOURCE_DIR is available from FetchContent in parent scope
if(TARGET GTest::gmock) # Or just check if googletest_SOURCE_DIR is defined
    target_include_directories(runTests PRIVATE
        ${googletest_SOURCE_DIR}/googlemock/include
        # ${googletest_SOURCE_DIR}/googletest/include # This one seems to be already picked up via -isystem
    )
endif()

# To test components from the main TextEditor project (e.g., TextBuffer, Editor commands):
# Option 1: Compile necessary .cpp files from ../src directly into the test executable.
# This is simpler for smaller projects but less clean.
# Example: target_sources(runTests PRIVATE ../src/TextBuffer.cpp ../src/Command.cpp ...) 
# You would need to be careful about not including main.cpp from ../src.

# Option 2 (Preferred for larger projects): Turn the core editor logic into a static or shared library.
# In the root CMakeLists.txt:
#   add_library(EditorLib STATIC ${EDITOR_CORE_SOURCES}) # Define EDITOR_CORE_SOURCES
#   target_link_libraries(TextEditor PRIVATE EditorLib)
# Then in tests/CMakeLists.txt:
#   target_link_libraries(runTests PRIVATE EditorLib)

# For now, let's assume we will add a sample test that doesn't depend on Editor sources,
# or we'll add specific sources later.

# Discover tests (CMake 3.10+)
include(GoogleTest)
gtest_discover_tests(runTests)

message(STATUS "Configured test executable 'runTests' linked against EditorLib.") 