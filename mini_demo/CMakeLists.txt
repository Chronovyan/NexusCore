cmake_minimum_required(VERSION 3.10)
project(SimpleEditorDemo)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define paths to external libraries and include directories
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../external")
set(IMGUI_DIR "${EXTERNAL_DIR}/imgui")

# Include directories
include_directories(
    ${EXTERNAL_DIR}/glew/include
    ${EXTERNAL_DIR}/glfw/include
    ${IMGUI_DIR}
    ${CMAKE_SOURCE_DIR}
)

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add the executable
add_executable(SimpleEditorDemo
    SimpleEditorDemo.cpp
    SimpleEditorWindow.cpp
    ${IMGUI_SOURCES}
)

# Link against OpenGL (this should work on all platforms)
if(WIN32)
    target_link_libraries(SimpleEditorDemo
        opengl32
    )
else()
    target_link_libraries(SimpleEditorDemo
        GL
    )
endif()

# Manual approach for Windows
if(WIN32)
    # Use the existing built libraries in the main project
    target_link_libraries(SimpleEditorDemo
        ${CMAKE_SOURCE_DIR}/../lib/glfw3.lib
        ${CMAKE_SOURCE_DIR}/../lib/glew32.lib
    )
endif() 