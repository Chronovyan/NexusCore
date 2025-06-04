$content = @"
cmake_minimum_required(VERSION 3.14)
project(MinimalOpenGLTest VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Use absolute paths with correct path to project root
set(PROJECT_ROOT_DIR "C:/Users/HydraPony/dev/AI-First TextEditor")
set(GLFW_DIR "`${PROJECT_ROOT_DIR}/external/glfw")
set(GLEW_DIR "`${PROJECT_ROOT_DIR}/external/glew")

# --- Minimal OpenGL Test ---
add_executable(MinimalOpenGLTest main_minimal.cpp)

# Include directories for GLFW and GLEW
target_include_directories(MinimalOpenGLTest PRIVATE 
  "`${GLFW_DIR}/include"
  "`${GLEW_DIR}/include"
)

# Link against OpenGL and prebuilt GLFW and GLEW libraries
target_link_libraries(MinimalOpenGLTest PRIVATE 
  OpenGL::GL
  "`${PROJECT_ROOT_DIR}/build/debug/lib/glfw3.lib"
  "`${GLEW_DIR}/lib/Release/x64/glew32.lib"
)
"@

Set-Content -Path "CMakeLists.txt" -Value $content 