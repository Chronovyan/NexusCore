cmake_minimum_required(VERSION 3.14) # Increased for FetchContent and modern CMake practices
project(TextEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer not to use compiler-specific extensions

include(FetchContent)

# --- Editor Library (EditorLib) ---
# Source files for the static library (all .cpp from src/ except main.cpp)
set(EDITOR_LIB_SOURCES
    src/Editor.cpp
    src/TextBuffer.cpp
    src/EditorCommands.cpp
    src/SyntaxHighlighter.cpp
    src/SyntaxHighlightingManager.cpp
    src/EditorError.cpp
    # Headers that are part of the library's interface or implementation details
    src/Editor.h
    src/TextBuffer.h
    src/Command.h
    src/CommandManager.h
    src/EditorCommands.h
    src/SyntaxHighlighter.h
    src/SyntaxHighlightingManager.h
    src/EditorError.h
)

add_library(EditorLib STATIC ${EDITOR_LIB_SOURCES})

# Public include directory for EditorLib, so targets linking against it can find headers.
# This also applies to the library itself for its own internal includes.
target_include_directories(EditorLib PUBLIC src)

# Explicitly set C++17 standard for the compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(EditorLib PRIVATE -std=c++17)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(EditorLib PRIVATE -std=c++17)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(EditorLib PRIVATE /std:c++17)
endif()

# --- Main Editor Executable (TextEditor) ---
# Now only includes main.cpp and links against EditorLib
add_executable(TextEditor src/main.cpp)

target_link_libraries(TextEditor PRIVATE EditorLib)

# No need for target_include_directories(TextEditor PUBLIC src) anymore for TextEditor itself,
# as EditorLib provides its headers via its PUBLIC include directory.
# However, main.cpp might still need to find headers from src, so if EditorLib's PUBLIC isn't enough,
# we might need it. But usually, linking the library that has PUBLIC include dirs is sufficient.


# Link against pthreads if on a POSIX system
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    # Link pthreads to EditorLib if threading is used within the library code itself.
    # Or link to TextEditor if main.cpp or other platform-specific main uses threads directly.
    # Assuming for now that if threading is used, it's within EditorLib's scope.
    target_link_libraries(EditorLib PUBLIC Threads::Threads) # Changed to PUBLIC if headers expose it
endif()

# --- GoogleTest Integration ---
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG    release-1.12.1 # Or a version compatible with your main project
)
# For GTest versions >= 1.10, set these to avoid building tests/samples if not needed
# For older versions, these might not be available or might be named differently.
set(GTEST_HAS_現地化 OFF CACHE INTERNAL "Disable gtest localization")
set(INSTALL_GTEST OFF CACHE INTERNAL "Disable installing gtest")
# Depending on GTest version, these might also be useful:
set(BUILD_GMOCK ON CACHE INTERNAL "Ensure GMock is built") # Explicitly ensure gmock is built
set(BUILD_GTEST ON CACHE INTERNAL "Ensure GTest is built") # Explicitly ensure gtest is built
# set(gtest_build_tests OFF CACHE INTERNAL "Disable building gtest's own tests")
# set(gtest_build_samples OFF CACHE INTERNAL "Disable building gtest's samples")
# set(gmock_build_tests OFF CACHE INTERNAL "Disable building gmock's own tests")

FetchContent_MakeAvailable(googletest)

# --- Testing ---
enable_testing()
# The add_subdirectory(tests) command will now find and process tests/CMakeLists.txt
# which should define its own targets (e.g., runTests) and link them against GTest::gtest etc.
add_subdirectory(tests) 

# Optional: Print a message
message(STATUS "Configured TextEditor project with EditorLib and GoogleTest.")

# The section for runMcve and gmock_mcve.cpp has been removed.

# The following complex find_package and linking logic for runMcve is removed
# as FetchContent_MakeAvailable should make GTest targets directly usable.
# // ... existing code ... (from original find_package(GTest CONFIG) down to its endif)
# // ... existing code ... (from original linking logic for runMcve down to its endif)
# // ... existing code ... (original message about MCVE configuration)

# Set discovery mode to PRE_TEST to avoid _NOT_BUILT issues
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST) 