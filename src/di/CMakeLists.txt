cmake_minimum_required(VERSION 3.10)

# Project declaration for the standalone build
project(DIFramework VERSION 1.0.0 LANGUAGES CXX)

# Add the DI library
add_library(di
    Injector.hpp
    ModuleManager.hpp
    ModuleManager.cpp
    CoreModule.hpp
    DIPatterns.hpp
    LifetimeManager.hpp
    AppDebugLog.h
)

# Set include directories for the library
target_include_directories(di PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set C++17 for the library
target_compile_features(di PUBLIC cxx_std_17)

# Create a simple DI test executable with a unique name
add_executable(standalone_di_test
    di_test.cpp
)

# Create a lifetime manager example executable
add_executable(lifetime_example
    LifetimeExample.cpp
)

# Link the test with the DI library
target_link_libraries(standalone_di_test PRIVATE di)

# Link the lifetime example with the DI library
target_link_libraries(lifetime_example PRIVATE di)

# Set C++17 for the executables
target_compile_features(standalone_di_test PUBLIC cxx_std_17)
target_compile_features(lifetime_example PUBLIC cxx_std_17)

# Tests could be added in the future 