cmake_minimum_required(VERSION 3.10)
project(TextBufferTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(TextBufferTest 
    test_textbuffer.cpp
    ${CMAKE_SOURCE_DIR}/../src/TextBuffer.cpp
)

# Include directories
target_include_directories(TextBufferTest PRIVATE 
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable testing
enable_testing()
add_test(NAME TextBufferTest COMMAND TextBufferTest)
