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
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip # Or a specific commit/tag
  # For more robust builds, consider specifying URL_HASH SHA256 <hash_value>
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# --- Testing ---
enable_testing()
add_subdirectory(tests) # This will contain CMakeLists.txt for tests

# Optional: Print a message
message(STATUS "Configured TextEditor project with EditorLib and GoogleTest.") 