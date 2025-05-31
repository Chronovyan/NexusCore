cmake_minimum_required(VERSION 3.14) # Increased for FetchContent and modern CMake practices
project(TextEditor VERSION 1.0 LANGUAGES CXX)

# Enable ccache if available for faster builds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer not to use compiler-specific extensions

# Instead of using FetchContent, use local repositories
# ImGui path
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
# GLFW path
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
# JSON path
set(JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/json)
# CPR path
set(CPR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/cpr)
# GoogleTest path
set(GTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest)

# --- GLFW (required for ImGui) ---
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(${GLFW_DIR})

# --- CPR (C++ Requests) for HTTP ---
set(CPR_BUILD_TESTS OFF CACHE INTERNAL "Build CPR tests")
set(CPR_BUILD_TESTS_SSL OFF CACHE INTERNAL "Build CPR SSL tests")
set(CPR_ENABLE_CERTIFICATE_OPTIMIZATION OFF CACHE INTERNAL "Enable certificate optimization (CPR)")
set(CPR_FORCE_USE_SYSTEM_CURL OFF CACHE INTERNAL "Use system curl instead of built-in")
add_subdirectory(${CPR_DIR})

# Disable unsafe buffer usage warning for CPR (third-party library)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    get_target_property(CPR_INCLUDE_DIRS cpr INCLUDE_DIRECTORIES)
    target_compile_options(cpr PRIVATE -Wno-unsafe-buffer-usage)
endif()

# --- nlohmann/json for JSON parsing ---
add_subdirectory(${JSON_DIR})

# --- GoogleTest Configuration ---
# Configure GoogleTest - always include it regardless of BUILD_TESTS
set(GTEST_HAS_現地化 OFF CACHE INTERNAL "Disable gtest localization")
set(INSTALL_GTEST OFF CACHE INTERNAL "Disable installing gtest")
# Depending on GTest version, these might also be useful:
set(BUILD_GMOCK ON CACHE INTERNAL "Ensure GMock is built") # Explicitly ensure gmock is built
set(BUILD_GTEST ON CACHE INTERNAL "Ensure GTest is built") # Explicitly ensure gtest is built

# Force GoogleTest to use the same runtime library as the rest of the project
set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib")

add_subdirectory(${GTEST_DIR})

# Create ImGui library with GLFW and OpenGL3 backends
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create a separate static library for ImGui
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC 
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw)

# Find OpenGL for ImGui backend
find_package(OpenGL REQUIRED)
target_link_libraries(imgui_lib PRIVATE OpenGL::GL)

# --- Editor Library (EditorLib) ---
# Source files for the static library (all .cpp from src/ except main.cpp)
set(EDITOR_LIB_SOURCES
    src/Editor.cpp
    src/TextBuffer.cpp
    src/EditorCommands.cpp
    src/ModernEditorCommands.cpp
    src/SyntaxHighlighter.cpp
    src/SyntaxHighlightingManager.cpp
    src/EditorError.cpp
    # DI Framework files
    src/di/Injector.cpp
    src/di/ModuleManager.cpp
    src/Application.cpp
    # Headers that are part of the library's interface or implementation details
    src/Editor.h
    src/TextBuffer.h
    src/Command.h
    src/CommandManager.h
    src/EditorCommands.h
    src/SyntaxHighlighter.h
    src/SyntaxHighlightingManager.h
    src/EditorError.h
    # DI Framework headers
    src/interfaces/IEditor.hpp
    src/interfaces/ITextBuffer.hpp
    src/interfaces/ICommandManager.hpp
    src/interfaces/ISyntaxHighlightingManager.hpp
    src/interfaces/IApplication.hpp
    src/di/Injector.hpp
    src/di/ModuleManager.hpp
    src/di/ApplicationModule.hpp
    src/di/TextBufferFactory.hpp
    src/di/CommandManagerFactory.hpp
    src/di/EditorFactory.hpp
    src/Application.h
)

add_library(EditorLib STATIC ${EDITOR_LIB_SOURCES})

# Add precompiled header support
target_precompile_headers(EditorLib PRIVATE
  "src/TextEditorPCH.h"
)

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

# --- AI-First TextEditor Executable ---
add_executable(AITextEditor
    src/AITextEditorApp.cpp
    src/UIModel.h
    src/OpenAI_API_Client.cpp
    src/OpenAI_API_Client.h
    src/IOpenAI_API_Client.h
    src/OpenAI_API_Client_types.h
    src/MockOpenAI_API_Client.cpp
    src/MockOpenAI_API_Client.h
    src/AIAgentOrchestrator.cpp
    src/AIAgentOrchestrator.h
    src/WorkspaceManager.cpp
    src/WorkspaceManager.h
)
# Link against imgui_lib instead of compiling ImGui sources directly
target_link_libraries(AITextEditor PRIVATE 
    EditorLib 
    imgui_lib 
    cpr::cpr
    nlohmann_json::nlohmann_json
    OpenGL::GL
)

# --- OpenAI API Client Test Executable ---
add_executable(OpenAIClientTest
    src/OpenAIClientTest.cpp
    src/OpenAI_API_Client.cpp
    src/OpenAI_API_Client.h
    src/IOpenAI_API_Client.h
    src/OpenAI_API_Client_types.h
    src/EditorError.cpp
    src/EditorErrorGlobals.cpp
    src/MockOpenAI_API_Client.cpp
    src/MockOpenAI_API_Client.h
)
target_link_libraries(OpenAIClientTest PRIVATE cpr::cpr nlohmann_json::nlohmann_json)

# --- DI Framework Test Executable ---
add_executable(DIFrameworkTest
    tests/DependencyInjectionTest.cpp
)
target_link_libraries(DIFrameworkTest PRIVATE 
    EditorLib
    gtest
    gtest_main
)

# --- DI Test Executable ---
add_executable(di_test
    src/di/di_test.cpp
)
target_link_libraries(di_test PRIVATE 
    EditorLib
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# Link against pthreads if on a POSIX system
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    # Link pthreads to EditorLib if threading is used within the library code itself.
    # Or link to TextEditor if main.cpp or other platform-specific main uses threads directly.
    # Assuming for now that if threading is used, it's within EditorLib's scope.
    target_link_libraries(EditorLib PUBLIC Threads::Threads) # Changed to PUBLIC if headers expose it
endif()

# Optional: Print a message
message(STATUS "Configured TextEditor project with EditorLib, ImGui, GoogleTest, and DI Framework.")

# Enable unity builds for faster compilation
set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 10) # Adjust based on your project size

# Option to build tests (OFF by default for faster builds)
option(BUILD_TESTS "Build the tests" OFF)

# Set discovery mode to PRE_TEST to avoid _NOT_BUILT issues
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

# Only build additional tests if the option is enabled
if(BUILD_TESTS)
  # Enable testing
  enable_testing()
  # The add_subdirectory(tests) command will now find and process tests/CMakeLists.txt
  # which should define its own targets (e.g., runTests) and link them against GTest::gtest etc.
  add_subdirectory(tests) 
endif()

# Add the DI Framework test to the test suite regardless of BUILD_TESTS
enable_testing()
add_test(NAME DIFrameworkTest COMMAND DIFrameworkTest)

# The standalone DI test project is included conditionally
if(NOT TARGET di)
  add_subdirectory(src/di)
endif() 