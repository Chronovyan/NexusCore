cmake_minimum_required(VERSION 3.21)
project(LlamaTest VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_executable(LlamaTest
    test_llama.cpp
)

# Copy the model file to the build directory
configure_file(
    "${CMAKE_SOURCE_DIR}/../models/llama/stable-code-3b.Q4_K_M.gguf"
    "${CMAKE_CURRENT_BINARY_DIR}/stable-code-3b.Q4_K_M.gguf"
    COPYONLY
)

# Include directories
target_include_directories(LlamaTest PRIVATE 
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../include
)

# Enable C++17 filesystem
if(MSVC)
    # For MSVC, we need to link against the correct runtime library
    target_compile_options(LlamaTest PRIVATE /EHsc /W4 /WX)
    target_compile_definitions(LlamaTest PRIVATE _SILENCE_CXX17_FILESYSTEM_DEPRECATION_WARNING)
    target_link_libraries(LlamaTest PRIVATE
        # No additional libraries needed for MSVC's std::filesystem
    )
else()
    # For other compilers (GCC, Clang)
    target_compile_options(LlamaTest PRIVATE -Wall -Wextra -Werror)
    target_compile_definitions(LlamaTest PRIVATE FMT_HEADER_ONLY=1)
    target_link_libraries(LlamaTest PRIVATE stdc++fs)
endif()

# For C++17 filesystem support
if(MSVC)
    target_compile_features(LlamaTest PRIVATE cxx_std_17)
else()
    target_compile_features(LlamaTest PRIVATE cxx_std_17)
    target_link_libraries(LlamaTest PRIVATE stdc++fs)
endif()
