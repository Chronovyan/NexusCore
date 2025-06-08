cmake_minimum_required(VERSION 3.10)
project(SimpleEditorDemo)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# Include directories
include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libs/imgui
    ${CMAKE_SOURCE_DIR}/src
)

# ImGui source files
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp
)

# Add ImGui as a library
add_library(imgui STATIC ${IMGUI_SOURCES})

# Add the executable
add_executable(SimpleEditorDemo
    src/SimpleEditorDemo.cpp
    src/SimpleEditorWindow.cpp
)

# Link libraries
target_link_libraries(SimpleEditorDemo
    imgui
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    glfw
)

# Copy GLEW DLL to output directory on Windows
if(WIN32)
    add_custom_command(TARGET SimpleEditorDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GLEW_ROOT}/bin/Release/x64/glew32.dll
            $<TARGET_FILE_DIR:SimpleEditorDemo>
    )
endif() 