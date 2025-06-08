cmake_minimum_required(VERSION 3.15)

# Enable testing
enable_testing()

# Set a flag to prevent the parent project from including its own GoogleTest
set(INCLUDE_GOOGLETEST OFF CACHE BOOL "" FORCE)

# Add the parent directory as a subdirectory to access the EditorLib target directly
# Use a unique build directory name to avoid conflicts
set(PARENT_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/parent_build")
file(MAKE_DIRECTORY ${PARENT_BUILD_DIR})

# Temporarily disable building tests in the parent project
set(BUILD_TESTING_SAVED ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Add the parent directory
add_subdirectory("${CMAKE_SOURCE_DIR}/../.." ${PARENT_BUILD_DIR} EXCLUDE_FROM_ALL)

# Restore the original BUILD_TESTING value
set(BUILD_TESTING ${BUILD_TESTING_SAVED} CACHE BOOL "" FORCE)

# Include GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add test executables
add_executable(
    test_rpgstate
    test_rpgstate.cpp
    ${CMAKE_SOURCE_DIR}/RPGState.cpp
)

# Add RPGGame test executable
add_executable(
    test_rpggame
    test_rpggame.cpp
    ${CMAKE_SOURCE_DIR}/RPGGame.cpp
)

# Set a flag to prevent the parent project from including its own GoogleTest
set(INCLUDE_GOOGLETEST OFF CACHE BOOL "" FORCE)

# Add the parent directory as a subdirectory to access the EditorLib target directly
# Use a unique build directory name to avoid conflicts
set(PARENT_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/parent_build")
file(MAKE_DIRECTORY ${PARENT_BUILD_DIR})

# Temporarily disable building tests in the parent project
set(BUILD_TESTING_SAVED ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Add the parent directory
add_subdirectory("${CMAKE_SOURCE_DIR}/../.." ${PARENT_BUILD_DIR} EXCLUDE_FROM_ALL)

# Restore the original BUILD_TESTING value
set(BUILD_TESTING ${BUILD_TESTING_SAVED} CACHE BOOL "" FORCE)

# Set common include directories
foreach(target test_rpgstate test_rpggame)
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/../../src  # For EditorLib headers
    )
    
    # Link with Google Test and other libraries
    target_link_libraries(${target} PRIVATE
        gtest_main
        gmock
        TextEditor::EditorLib  # Link with the main editor library
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
    )
    
    # Set output directory for test executables
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    )
    
    # Copy required DLLs for test executables
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/../dlls"
            COMMENT "Creating DLLs directory for ${target}"
        )
        
        # Copy Google Test DLLs if they exist
        foreach(gtest_lib gtest gtest_main gmock gmock_main)
            if(TARGET ${gtest_lib})
                get_target_property(dll_path ${gtest_lib} IMPORTED_LOCATION)
                if(dll_path AND EXISTS "${dll_path}")
                    add_custom_command(TARGET ${target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy "${dll_path}" "$<TARGET_FILE_DIR:${target}>/../dlls/"
                        COMMENT "Copying ${gtest_lib} DLL to output directory"
                    )
                endif()
            endif()
        endforeach()
    endif()
    
    # Add test with environment setup
    add_test(NAME ${target}
        COMMAND ${target}
    )
    
    # Set test environment to include DLL directory
    if(WIN32)
        set_tests_properties(${target} PROPERTIES
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:${target}>/../dlls;$ENV{PATH}"
        )
    endif()
endforeach()

# Add a test that runs all tests
add_test(NAME run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
)

# Enable test discovery for Visual Studio
if(MSVC)
    include(GoogleTest)
    gtest_discover_tests(test_rpgstate)
    gtest_discover_tests(test_rpggame)
endif()
