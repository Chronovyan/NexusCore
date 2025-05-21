cmake_minimum_required(VERSION 3.10)

# DI Framework Library
add_library(di INTERFACE)
target_include_directories(di INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# DI Example executable
add_executable(di_example
    example_main.cpp
)

target_link_libraries(di_example PRIVATE di)
target_compile_features(di_example PRIVATE cxx_std_17)

# Tests could be added here in the future 