cmake_minimum_required(VERSION 3.14)
project(TextBufferTest VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the executable
add_executable(SimpleTextBufferTest 
    SimpleTextBufferTest.cpp
)

# If using MSVC, set appropriate compile options
if(MSVC)
    target_compile_options(SimpleTextBufferTest PRIVATE /W4)
else()
    # GCC/Clang specific compile options
    target_compile_options(SimpleTextBufferTest PRIVATE -Wall -Wextra -pedantic)
endif()

# Add thread library
find_package(Threads REQUIRED)
target_link_libraries(SimpleTextBufferTest PRIVATE Threads::Threads)

# Set up include directories
target_include_directories(SimpleTextBufferTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Output directory
set_target_properties(SimpleTextBufferTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add custom target to run tests
add_custom_target(RunSimpleTextBufferTest
    COMMAND $<TARGET_FILE:SimpleTextBufferTest>
    DEPENDS SimpleTextBufferTest
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    COMMENT "Running SimpleTextBufferTest"
) 