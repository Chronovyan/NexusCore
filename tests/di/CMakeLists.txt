cmake_minimum_required(VERSION 3.10)

# Add the test executable
add_executable(di_tests 
    DependencyInjectionTests.cpp
)

# Link against the DI library and Google Test
target_link_libraries(di_tests PRIVATE
    di
    gtest
    gtest_main
    gmock
)

# Add the include directories
target_include_directories(di_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Use C++17
target_compile_features(di_tests PRIVATE cxx_std_17)

# Add the test
add_test(NAME DITests COMMAND di_tests) 