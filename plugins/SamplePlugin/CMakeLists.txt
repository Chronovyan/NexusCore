cmake_minimum_required(VERSION 3.10)
project(SamplePlugin VERSION 1.0.0)

# C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the shared library
add_library(SamplePlugin SHARED
    SamplePlugin.cpp
    SamplePlugin.h
)

# Include directories
target_include_directories(SamplePlugin PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# Link against any required libraries
# (In this simple case, we don't need to link against anything specific)

# Set output directory for the plugin
set_target_properties(SamplePlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Define symbol export on Windows
if(WIN32)
    target_compile_definitions(SamplePlugin PRIVATE PLUGIN_EXPORTS)
endif()

# Install targets
install(TARGETS SamplePlugin
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
) 